#include "renderer.hpp"

#include <format>
#include <algorithm>
#include <cassert>
#include <cstring>

#include "../logger.hpp"
#include "../utility/messages.hpp"
#include "../utility/config.hpp"

namespace tv {
    namespace {
        VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT /* messageSeverity */,
            VkDebugUtilsMessageTypeFlagsEXT /* messageType */,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* /* pUserData*/ ) {
            Logger::instance().err(std::format("{}\n", pCallbackData->pMessage));
            return VK_FALSE;
        }
    }

    Renderer::Renderer() noexcept
        : _mainWindow{ nullptr },
          _vInstance{ nullptr },
          _vDebugMessenger{ nullptr }
    {
        glfwInit();
        _vInstance = makeVInstance();
        _vDispatchLoaderDynamic.init(_vInstance, vkGetInstanceProcAddr);
        _vDebugMessenger = makeVDebugMessenger();
        initWindow();
    }

    Renderer::~Renderer() {
        glfwDestroyWindow(_mainWindow);
        _vInstance.destroyDebugUtilsMessengerEXT(_vDebugMessenger, nullptr, _vDispatchLoaderDynamic);
        _vInstance.destroy();
        glfwTerminate();
    }

    void Renderer::initWindow() noexcept {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        _mainWindow = glfwCreateWindow(
            _windowWidth,
            _windowHeight,
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

    void Renderer::printAdditionalInfo(const uint32_t vulkanVersion, const std::vector<const char*>& vulkanExtensions) noexcept {
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

    bool Renderer::vExtensionsSupported(const std::vector<const char*>& vulkanExtensions) noexcept {
        auto& logger = Logger::instance();
        const std::vector<vk::ExtensionProperties> supportedExtensions = vk::enumerateInstanceExtensionProperties();

#if(DEBUG_MODE)
        std::string supportedExtensionsMessage = std::format("{}:\n", constants::messages::VULKAN_EXTENSIONS);
        for (const vk::ExtensionProperties& supportedExtension : supportedExtensions)
            supportedExtensionsMessage += std::format("    {}\n", supportedExtension.extensionName.data());
        logger.log(supportedExtensionsMessage);
#endif
        for (auto vExtension : vulkanExtensions) {
            if (std::ranges::any_of(supportedExtensions, [vExtension](const vk::ExtensionProperties& supportedExtension) {
                    return std::strcmp(vExtension, supportedExtension.extensionName) == 0;
                })) {
#if(DEBUG_MODE)
                logger.log(std::format("{}: {}\n", constants::messages::VULKAN_EXTENSION_SUPPORTED, vExtension));
#endif
                continue;
            }

            logger.err(std::format("{}: {}\n", constants::messages::VULKAN_EXTENSION_NOT_SUPPORTED, vExtension));
            return false;
        }

        return true;
    }

    bool Renderer::vLayersSupported(const std::vector<const char*>& vulkanLayers) noexcept {
        auto& logger = Logger::instance();
        const std::vector<vk::LayerProperties> supportedLayers = vk::enumerateInstanceLayerProperties();

#if(DEBUG_MODE)
        std::string supportedLayersMessage = std::format("{}:\n", constants::messages::VULKAN_LAYERS);
        for (const vk::LayerProperties& supportedLayer : supportedLayers)
            supportedLayersMessage += std::format("    {}\n", supportedLayer.layerName.data());
        logger.log(supportedLayersMessage);
#endif
        for (auto vLayer : vulkanLayers) {
            if (std::ranges::any_of(supportedLayers, [vLayer](const vk::LayerProperties& supportedLayer) {
                    return std::strcmp(vLayer, supportedLayer.layerName) == 0;
                })) {
#if(DEBUG_MODE)
                logger.log(std::format("{}: {}\n", constants::messages::VULKAN_LAYER_SUPPORTED, vLayer));
#endif
                continue;
            }

            logger.err(std::format("{}: {}\n", constants::messages::VULKAN_LAYER_NOT_SUPPORTED, vLayer));
            return false;
        }

        return true;
    }

    vk::Instance Renderer::makeVInstance() noexcept {
        assert(glfwVulkanSupported());
        auto& logger = Logger::instance();

        uint32_t vulkanVersion{ 0 };
        vkEnumerateInstanceVersion(&vulkanVersion);

        uint32_t vulkanExtensionCount = 0;
        auto rawGlfwExtensions = glfwGetRequiredInstanceExtensions(&vulkanExtensionCount);
        std::vector<const char*> vulkanExtensions{ rawGlfwExtensions, rawGlfwExtensions + vulkanExtensionCount };
        std::vector<const char*> vulkanLayers{};

#if(DEBUG_MODE)
        vulkanExtensions.emplace_back(constants::config::VULKAN_EXT_DEBUG.c_str());
        vulkanLayers.emplace_back(constants::config::VULKAN_LAYER_VALIDATION.c_str());
        ++vulkanExtensionCount;
#endif
        printAdditionalInfo(vulkanVersion, vulkanExtensions);

        if(!vExtensionsSupported(vulkanExtensions)) {
            logger.err(constants::messages::VULKAN_SOME_EXTENSIONS_NOT_SUPPORTED);
            return nullptr;
        }

        if (!vLayersSupported(vulkanLayers)) {
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
            logger.err(std::format("{}: {}",
                constants::messages::VULKAN_INSTANCE_CREATION_FAILED,
                err.what()));
            return nullptr;
        }
    }

    vk::DebugUtilsMessengerEXT Renderer::makeVDebugMessenger() noexcept {
#if(DEBUG_MODE)
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

        return _vInstance.createDebugUtilsMessengerEXT(createInfo, nullptr, _vDispatchLoaderDynamic);
#endif
        return nullptr;
    }
}

