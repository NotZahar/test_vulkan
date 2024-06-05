#include "renderer.hpp"

#include <format>
#include <algorithm>
#include <set>
#include <string>
#include <limits>
#include <cassert>
#include <cstring>

#include "../logger.hpp"
#include "../utility/messages.hpp"
#include "../utility/config.hpp"

namespace tv {
    namespace {
#if(TV_DEBUG_MODE)
        VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT /* messageSeverity */,
            VkDebugUtilsMessageTypeFlagsEXT /* messageType */,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* /* pUserData */ ) {
            Logger::instance().err(std::format("{}\n", pCallbackData->pMessage));
            return VK_FALSE;
        }
#endif
    }

    Renderer::Renderer() noexcept
        : _mainWindow{ nullptr },
          _vInstance{ nullptr },
          _vPhysicalDevice{ nullptr },
          _vDevice{ nullptr },
          _vGraphicsQueue{ nullptr },
          _vPresentQueue{ nullptr },
          _vDebugMessenger{ nullptr }
    {
        glfwInit();
        initWindow();

        _vInstance = makeVInstance();
        _vDispatchLoaderDynamic.init(_vInstance, vkGetInstanceProcAddr);
        _vDebugMessenger = makeVDebugMessenger(_vInstance);

        createVSurface(_vInstance, _vSurface);

        _vPhysicalDevice = chooseVDevice(_vInstance);
        _vDevice = createVLogicalDevice(_vPhysicalDevice, _vSurface);

        auto vQueues = getVQueues(_vPhysicalDevice, _vDevice, _vSurface);
        assert(vQueues.size() == 2);
        _vGraphicsQueue = vQueues[0];
        _vPresentQueue = vQueues[1];

        _vSwapChainBundle = createSwapchain(_vDevice, _vPhysicalDevice, _vSurface);
    }

    Renderer::~Renderer() {
        std::ranges::for_each(_vSwapChainBundle.frames, [this](structures::VSwapChainFrame& frame) {
            _vDevice.destroyImageView(frame.imageView);
        });

        _vDevice.destroySwapchainKHR(_vSwapChainBundle.swapChain);
        _vDevice.destroy();

        _vInstance.destroySurfaceKHR(_vSurface);
#if(TV_DEBUG_MODE)
        _vInstance.destroyDebugUtilsMessengerEXT(_vDebugMessenger, nullptr, _vDispatchLoaderDynamic);
#endif
        _vInstance.destroy();

        glfwDestroyWindow(_mainWindow);
        glfwTerminate();
    }

    void Renderer::initWindow() noexcept {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        _mainWindow = glfwCreateWindow(
            constants::config::WINDOW_MAIN_WIDTH,
            constants::config::WINDOW_MAIN_HEIGHT,
            constants::config::WINDOW_TITLE.c_str(),
            nullptr,
            nullptr
        );
    }

    Renderer& Renderer::instance() noexcept {
        static Renderer instance;
        return instance;
    }

    void Renderer::processEvents() noexcept {
        while (!glfwWindowShouldClose(_mainWindow))
            glfwPollEvents();
    }

    void Renderer::printAdditionalInfo(const uint32_t vulkanVersion, const std::vector<const char*>& vulkanExtensions) const noexcept {
        const std::string vulkanVersionMessage = std::format(
            "{}: {}.{}.{}\n",
            constants::messages::VULKAN_API_VERSION,
            VK_API_VERSION_MAJOR(vulkanVersion),
            VK_API_VERSION_MINOR(vulkanVersion),
            VK_API_VERSION_PATCH(vulkanVersion)
        );

        std::string requestedExtensionsMessage = std::format("{}:\n", constants::messages::VULKAN_REQUESTED_EXTENSIONS);
        for (auto name : vulkanExtensions)
            requestedExtensionsMessage += std::format("    {}\n", name);

        auto& logger = Logger::instance();
        logger.log(vulkanVersionMessage);
        logger.log(requestedExtensionsMessage);
    }

    bool Renderer::deviceIsSuitable(const vk::PhysicalDevice& vDevice) const noexcept {
        const std::vector<const char*> requestedExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        std::set<std::string> requiredExtensions{ requestedExtensions.cbegin(), requestedExtensions.cend() };
        const auto deviceExtensions = vDevice.enumerateDeviceExtensionProperties();
        for (const vk::ExtensionProperties& deviceExtension : deviceExtensions)
            requiredExtensions.erase(deviceExtension.extensionName);
        return requiredExtensions.empty();
    }

    structures::VQueueFamilyIndices Renderer::findQueueFamilies(const vk::PhysicalDevice& vPhysicalDevice, vk::SurfaceKHR& vSurface) const noexcept {
        structures::VQueueFamilyIndices indices;
        const auto queueFamilies = vPhysicalDevice.getQueueFamilyProperties();
#if(TV_DEBUG_MODE)
        Logger::instance().log(std::format("    {}: {}\n", constants::messages::VULKAN_DEVICE_QUEUE_FAMILIES, queueFamilies.size()));
#endif
        for (int i = 0; const vk::QueueFamilyProperties& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
                indices.graphicsFamily = i;

            if (vPhysicalDevice.getSurfaceSupportKHR(i, vSurface))
                indices.presentFamily = i;

            if (indices.isComplete())
                break;

            ++i;
        }

        return indices;
    }

    bool Renderer::extensionsSupported(const std::vector<const char*>& vulkanExtensions) const noexcept {
        auto& logger = Logger::instance();
        const std::vector<vk::ExtensionProperties> supportedExtensions = vk::enumerateInstanceExtensionProperties();

#if(TV_DEBUG_MODE)
        std::string supportedExtensionsMessage = std::format("{}:\n", constants::messages::VULKAN_EXTENSIONS);
        for (const vk::ExtensionProperties& supportedExtension : supportedExtensions)
            supportedExtensionsMessage += std::format("    {}\n", supportedExtension.extensionName.data());
        logger.log(supportedExtensionsMessage);
#endif
        for (auto vExtension : vulkanExtensions) {
            if (std::ranges::any_of(supportedExtensions, [vExtension](const vk::ExtensionProperties& supportedExtension) {
                    return std::strcmp(vExtension, supportedExtension.extensionName) == 0;
                })) {
#if(TV_DEBUG_MODE)
                logger.log(std::format("{}: {}\n", constants::messages::VULKAN_EXTENSION_SUPPORTED, vExtension));
#endif
                continue;
            }

            logger.err(std::format("{}: {}\n", constants::messages::VULKAN_EXTENSION_NOT_SUPPORTED, vExtension));

            return false;
        }

        return true;
    }

    bool Renderer::layersSupported(const std::vector<const char*>& vulkanLayers) const noexcept {
        auto& logger = Logger::instance();
        const std::vector<vk::LayerProperties> supportedLayers = vk::enumerateInstanceLayerProperties();

#if(TV_DEBUG_MODE)
        std::string supportedLayersMessage = std::format("{}:\n", constants::messages::VULKAN_LAYERS);
        for (const vk::LayerProperties& supportedLayer : supportedLayers)
            supportedLayersMessage += std::format("    {}\n", supportedLayer.layerName.data());
        logger.log(supportedLayersMessage);
#endif
        for (auto vLayer : vulkanLayers) {
            if (std::ranges::any_of(supportedLayers, [vLayer](const vk::LayerProperties& supportedLayer) {
                    return std::strcmp(vLayer, supportedLayer.layerName) == 0;
                })) {
#if(TV_DEBUG_MODE)
                logger.log(std::format("{}: {}\n", constants::messages::VULKAN_LAYER_SUPPORTED, vLayer));
#endif
                continue;
            }

            logger.err(std::format("{}: {}\n", constants::messages::VULKAN_LAYER_NOT_SUPPORTED, vLayer));

            return false;
        }

        return true;
    }

    vk::SurfaceFormatKHR Renderer::chooseSwapchainSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &vFormats) const noexcept {
        if (auto it = std::ranges::find_if(vFormats,
                [](const auto& format) {
                    return format.format == vk::Format::eB8G8R8A8Unorm
                        && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
                }
            );
            it != vFormats.end()) {
            return *it;
        }

        assert(vFormats.size() >= 1);
        return vFormats[0];
    }

    vk::PresentModeKHR Renderer::chooseSwapchainPresentMode(const std::vector<vk::PresentModeKHR> &vPresentMods) const noexcept {
        if (auto it = std::ranges::find_if(vPresentMods,
                [](const auto& presentMode) {
                    return presentMode == vk::PresentModeKHR::eMailbox;
                }
            );
            it != vPresentMods.end()) {
            return *it;
        }

        return vk::PresentModeKHR::eFifo;
    }

    vk::Extent2D Renderer::chooseSwapchainExtent(const vk::SurfaceCapabilitiesKHR& vCapabilities) const noexcept {
        if (vCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
            return vCapabilities.currentExtent;

        int width;
        int height;
        glfwGetFramebufferSize(_mainWindow, &width, &height);

        vk::Extent2D extent{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
        extent.width = std::clamp(extent.width, vCapabilities.minImageExtent.width, vCapabilities.maxImageExtent.width);
        extent.height = std::clamp(extent.height, vCapabilities.minImageExtent.height, vCapabilities.maxImageExtent.height);

        return extent;
    }

    structures::VSwapChainBundle Renderer::createSwapchain(vk::Device& vDevice, vk::PhysicalDevice& vPhysicalDevice, vk::SurfaceKHR& vSurface) const noexcept {
        auto& logger = Logger::instance();
#if(TV_DEBUG_MODE)
        logger.log(std::format("{}\n", constants::messages::VULKAN_SWAPCHAIN_CREATION_STARTED));
#endif
        structures::VSwapChainDetails details = querySwapchainDetails(vPhysicalDevice, vSurface);
        vk::SurfaceFormatKHR format = chooseSwapchainSurfaceFormat(details.formats);
        vk::PresentModeKHR presentMode = chooseSwapchainPresentMode(details.presentMods);
        vk::Extent2D extent = chooseSwapchainExtent(details.capabilities);
        uint32_t imageCount = std::min(
            details.capabilities.maxImageCount,
            details.capabilities.minImageCount + 1
        );

        constexpr uint32_t imageArrayLayers{ 1 };
        vk::SwapchainCreateInfoKHR createInfo{
            vk::SwapchainCreateFlagsKHR(),
            vSurface,
            imageCount,
            format.format,
            format.colorSpace,
            extent,
            imageArrayLayers,
            vk::ImageUsageFlagBits::eColorAttachment
        };

        structures::VQueueFamilyIndices indices = findQueueFamilies(vPhysicalDevice, vSurface);
        assert(indices.graphicsFamily.has_value() && indices.presentFamily.has_value());
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        constexpr uint32_t queueFamilyIndexCount{ 2 };
        if (indices.graphicsFamily.value() != indices.presentFamily.value()) {
            createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
            createInfo.queueFamilyIndexCount = queueFamilyIndexCount;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        }

        createInfo.preTransform = details.capabilities.currentTransform;
        createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        createInfo.oldSwapchain = nullptr;

        structures::VSwapChainBundle bundle;
        try {
            bundle.swapChain = vDevice.createSwapchainKHR(createInfo);
        } catch (const vk::SystemError& err) {
            assert(false);
            logger.err(std::format("{}\n", constants::messages::VULKAN_SWAPCHAIN_CREATION_FAILED));
            return bundle;
        }

        const std::vector<vk::Image> images = vDevice.getSwapchainImagesKHR(bundle.swapChain);
        bundle.frames.reserve(images.size());
        for (size_t i = 0; i < images.size(); ++i) {
            vk::ImageViewCreateInfo imageViewCreateInfo{};
            imageViewCreateInfo.image = images[i];
            imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
            imageViewCreateInfo.format = format.format;
            imageViewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
            imageViewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
            imageViewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
            imageViewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
            imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
            imageViewCreateInfo.subresourceRange.levelCount = 1;
            imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
            imageViewCreateInfo.subresourceRange.layerCount = 1;
            imageViewCreateInfo.format = format.format;

            bundle.frames.emplace_back(structures::VSwapChainFrame{ images[i], vDevice.createImageView(imageViewCreateInfo) });
        }

        bundle.format = format.format;
        bundle.extent = extent;

        return bundle;
    }

    structures::VSwapChainDetails Renderer::querySwapchainDetails(const vk::PhysicalDevice &vDevice, vk::SurfaceKHR &vSurface) const noexcept {
        structures::VSwapChainDetails details;
        details.capabilities = vDevice.getSurfaceCapabilitiesKHR(vSurface);
        details.formats = vDevice.getSurfaceFormatsKHR(vSurface);
        details.presentMods = vDevice.getSurfacePresentModesKHR(vSurface);

        return details;
    }

    vk::Instance Renderer::makeVInstance() const noexcept {
        assert(glfwVulkanSupported());
        auto& logger = Logger::instance();

        uint32_t vulkanVersion{ 0 };
        vkEnumerateInstanceVersion(&vulkanVersion);

        uint32_t vulkanExtensionCount = 0;
        auto rawGlfwExtensions = glfwGetRequiredInstanceExtensions(&vulkanExtensionCount);
        std::vector<const char*> vulkanExtensions{ rawGlfwExtensions, rawGlfwExtensions + vulkanExtensionCount };
        std::vector<const char*> vulkanLayers{};

#if(TV_DEBUG_MODE)
        vulkanExtensions.emplace_back(constants::config::VULKAN_EXT_DEBUG.c_str());
        vulkanLayers.emplace_back(constants::config::VULKAN_LAYER_VALIDATION.c_str());
        ++vulkanExtensionCount;

        printAdditionalInfo(vulkanVersion, vulkanExtensions);
#endif
        if(!extensionsSupported(vulkanExtensions)) {
            logger.err(constants::messages::VULKAN_SOME_EXTENSIONS_NOT_SUPPORTED);
            return nullptr;
        }

        if (!layersSupported(vulkanLayers)) {
            logger.err(constants::messages::VULKAN_SOME_LAYERS_NOT_SUPPORTED);
            return nullptr;
        }

        vk::ApplicationInfo appInfo{
            constants::config::APP_NAME.c_str(),
            vulkanVersion,
            nullptr,
            vulkanVersion,
            vulkanVersion
        };

        vk::InstanceCreateInfo createInfo{
            vk::InstanceCreateFlags(),
            &appInfo,
            static_cast<uint32_t>(vulkanLayers.size()),
            vulkanLayers.data(),
            static_cast<uint32_t>(vulkanExtensions.size()),
            vulkanExtensions.data()
        };

        try {
            return vk::createInstance(createInfo);
        } catch (const vk::SystemError& err) {
            logger.err(std::format("{}: {}\n",
                constants::messages::VULKAN_INSTANCE_CREATION_FAILED,
                err.what()));
            return nullptr;
        }
    }

    void Renderer::createVSurface(vk::Instance& vInstance, vk::SurfaceKHR& vSurface) const noexcept {
        VkSurfaceKHR surface{};
        assert(_mainWindow != nullptr);
        if (glfwCreateWindowSurface(vInstance, _mainWindow, nullptr, &surface) != VK_SUCCESS) {
#if(TV_DEBUG_MODE)
            Logger::instance().err(std::format("{}\n", constants::messages::VULKAN_SURFACE_CREATION_FAILED));
#endif
        }

        vSurface = surface;
    }

    vk::DebugUtilsMessengerEXT Renderer::makeVDebugMessenger([[maybe_unused]] vk::Instance& vInstance) const noexcept {
#if(TV_DEBUG_MODE)
        vk::DebugUtilsMessengerCreateInfoEXT createInfo{
            vk::DebugUtilsMessengerCreateFlagsEXT(),
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
                | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
                | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
                | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
                | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            debugCallback,
            nullptr
        };

        return vInstance.createDebugUtilsMessengerEXT(createInfo, nullptr, _vDispatchLoaderDynamic);
#else
        return nullptr;
#endif
    }

    vk::PhysicalDevice Renderer::chooseVDevice(const vk::Instance& vInstance) const noexcept {
        auto& logger = Logger::instance();
        const std::vector<vk::PhysicalDevice> availableDevices = vInstance.enumeratePhysicalDevices();
        if (availableDevices.empty()) {
            logger.err(std::format("{}\n", constants::messages::VULKAN_NO_AVAILABLE_DEVICE));
            return nullptr;
        }

        for (const vk::PhysicalDevice& device : availableDevices) {
#if(TV_DEBUG_MODE)
            logger.log(std::format("{}: {}\n", constants::messages::VULKAN_DEVICE_NAME, device.getProperties().deviceName.data()));
#endif
            if (deviceIsSuitable(device))
                return device;
        }

        return nullptr;
    }

    vk::Device Renderer::createVLogicalDevice(vk::PhysicalDevice& vPhysicalDevice, vk::SurfaceKHR& vSurface) const noexcept {
#if(TV_DEBUG_MODE)
        Logger::instance().log(std::format("{}\n", constants::messages::VULKAN_DEVICE_CREATION_STARTED));
#endif
        structures::VQueueFamilyIndices familyIndices = findQueueFamilies(vPhysicalDevice, vSurface);
        std::vector<uint32_t> uniqueFamilyIndices;

        assert(familyIndices.graphicsFamily.has_value() && familyIndices.presentFamily.has_value());
        uniqueFamilyIndices.emplace_back(familyIndices.graphicsFamily.value());
        if (familyIndices.graphicsFamily.value() != familyIndices.presentFamily.value())
            uniqueFamilyIndices.emplace_back(familyIndices.presentFamily.value());

        float queuePriority{ 1 };
        constexpr uint32_t queueCount{ 1 };

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfo;
        for (uint32_t index : uniqueFamilyIndices) {
            queueCreateInfo.emplace_back(vk::DeviceQueueCreateInfo{
                vk::DeviceQueueCreateFlags(),
                index,
                queueCount,
                &queuePriority
            });
        }

        std::vector<const char*> deviceExtensions{
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        vk::PhysicalDeviceFeatures deviceFeatures{};
        std::vector<const char*> enabledLayers;
#if(TV_DEBUG_MODE)
        enabledLayers.emplace_back(constants::config::VULKAN_LAYER_VALIDATION.c_str());
#endif
        vk::DeviceCreateInfo deviceInfo{
            vk::DeviceCreateFlags(),
            static_cast<uint32_t>(queueCreateInfo.size()),
            queueCreateInfo.data(),
            static_cast<uint32_t>(enabledLayers.size()),
            enabledLayers.data(),
            static_cast<uint32_t>(deviceExtensions.size()),
            deviceExtensions.data(),
            &deviceFeatures
        };

        try {
            return vPhysicalDevice.createDevice(deviceInfo);
        } catch (const vk::SystemError& err) {
            Logger::instance().err(std::format("{}: {}\n", constants::messages::VULKAN_DEVICE_CREATION_FAILED, err.what()));
        }

        return nullptr;
    }

    std::vector<vk::Queue> Renderer::getVQueues(const vk::PhysicalDevice& vPhysicalDevice, vk::Device& vDevice, vk::SurfaceKHR& vSurface) const noexcept {
#if(TV_DEBUG_MODE)
        Logger::instance().log(std::format("{}\n", constants::messages::VULKAN_GETTING_QUEUE_STARTED));
#endif
        structures::VQueueFamilyIndices indices = findQueueFamilies(vPhysicalDevice, vSurface);
        constexpr uint32_t queueIndex{ 0 };

        assert(indices.graphicsFamily.has_value() && indices.presentFamily.has_value());
        return {
            vDevice.getQueue(indices.graphicsFamily.value(), queueIndex),
            vDevice.getQueue(indices.presentFamily.value(), queueIndex)
        };
    }
}

