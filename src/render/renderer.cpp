#include "renderer.hpp"

#include <format>
#include <algorithm>
#include <set>
#include <string>
#include <limits>
#include <mutex>
#include <cassert>
#include <cstring>

#include "../logger.hpp"
#include "../services/file_service.hpp"
#include "../utility/messages.hpp"
#include "../utility/config.hpp"
#include "../utility/paths.hpp"
#include "../shaders/models/triangle.hpp"

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
        : _window{ nullptr },
          _vInstance{ nullptr },
          _vPhysicalDevice{ nullptr },
          _vDevice{ nullptr },
          _vGraphicsQueue{ nullptr },
          _vPresentQueue{ nullptr },
          _vDebugMessenger{ nullptr }
    {}

    Renderer::~Renderer() {
        _vDevice.waitIdle();

        _vDevice.destroyCommandPool(_vCommandPool);

        _vDevice.destroyPipeline(_vGraphicsPipelineBundle.pipeline);
        _vDevice.destroyPipelineLayout(_vGraphicsPipelineBundle.layout);
        _vDevice.destroyRenderPass(_vGraphicsPipelineBundle.renderpass);

        resetSwapchain();
        _vDevice.destroy();

        _vInstance.destroySurfaceKHR(_vSurface);
#if(TV_DEBUG_MODE)
        _vInstance.destroyDebugUtilsMessengerEXT(_vDebugMessenger, nullptr, _vDispatchLoaderDynamic);
#endif
        _vInstance.destroy();
    }

    void Renderer::setup(Renderer& renderer, GLFWwindow* window) noexcept {
        static std::once_flag setupFlag;
        std::call_once(setupFlag, [&renderer, window]() {
            renderer.init(window);
        });
    }

    void Renderer::render(Scene* scene) noexcept {
        auto waitResult = _vDevice.waitForFences(1, &_vSwapChainBundle.frames[_vFrameNumber].inFlight, VK_TRUE, UINT64_MAX);
        if (waitResult != vk::Result::eSuccess)
            return;

        auto resetResult = _vDevice.resetFences(1, &_vSwapChainBundle.frames[_vFrameNumber].inFlight);
        if (resetResult != vk::Result::eSuccess)
            return;

        vk::ResultValue acquireResult = _vDevice.acquireNextImageKHR(_vSwapChainBundle.swapChain, UINT64_MAX, _vSwapChainBundle.frames[_vFrameNumber].imageAvailable, nullptr);
        if (acquireResult.result == vk::Result::eErrorOutOfDateKHR) {
            recreateSwapchain();
            return;
        }

        uint32_t imageIndex = acquireResult.value;
        vk::CommandBuffer commandBuffer = _vSwapChainBundle.frames[_vFrameNumber].commandBuffer;

        commandBuffer.reset();

        recordDrawCommands(commandBuffer, imageIndex, _vGraphicsPipelineBundle, _vSwapChainBundle, scene);

        vk::SubmitInfo submitInfo{};

        vk::Semaphore waitSemaphores[] = { _vSwapChainBundle.frames[_vFrameNumber].imageAvailable };
        vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vk::Semaphore signalSemaphores[] = { _vSwapChainBundle.frames[_vFrameNumber].renderFinished };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        try {
            _vGraphicsQueue.submit(submitInfo, _vSwapChainBundle.frames[_vFrameNumber].inFlight);
        } catch (const vk::SystemError& err) {
#if(TV_DEBUG_MODE)
            Logger::instance().log(std::format("{}\n", err.what()));
#endif
            return;
        }

        vk::PresentInfoKHR presentInfo{};
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        vk::SwapchainKHR swapChains[] = { _vSwapChainBundle.swapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;

        presentInfo.pImageIndices = &imageIndex;

        vk::Result presentResult;
        try {
            presentResult = _vPresentQueue.presentKHR(presentInfo);
        } catch ([[maybe_unused]] const vk::OutOfDateKHRError& err) {
#if(TV_DEBUG_MODE)
            Logger::instance().log(std::format("{}\n", err.what()));
#endif
            presentResult = vk::Result::eErrorOutOfDateKHR;
        }

        if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR) {
            recreateSwapchain();
            return;
        }

        _vFrameNumber = (_vFrameNumber + 1) % _vMaxFramesInFlight;
    }

    void Renderer::init(GLFWwindow* window) noexcept {
        assert(window);
        _window = window;

        _vInstance = createInstance();
        _vDispatchLoaderDynamic.init(_vInstance, vkGetInstanceProcAddr);
        _vDebugMessenger = createDebugMessenger(_vInstance);

        createSurface(_window, _vInstance, _vSurface);

        _vPhysicalDevice = chooseDevice(_vInstance);
        _vDevice = createLogicalDevice(_vPhysicalDevice, _vSurface);

        auto vQueues = getQueues(_vPhysicalDevice, _vDevice, _vSurface);
        assert(vQueues.size() == 2);
        _vGraphicsQueue = vQueues[0];
        _vPresentQueue = vQueues[1];

        _vSwapChainBundle = createSwapchain(_window, _vDevice, _vPhysicalDevice, _vSurface, _vMaxFramesInFlight);
        _vGraphicsPipelineBundle = createPipeline(_vDevice, _vSwapChainBundle);

        _vFrameNumber = 0;

        finalSetup(_vDevice, _vPhysicalDevice, _vSurface, _vGraphicsPipelineBundle, _vSwapChainBundle, _vCommandPool, _vMainCommandBuffer);
    }

    Renderer& Renderer::instance() noexcept {
        static Renderer instance;
        return instance;
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

    vk::Extent2D Renderer::chooseSwapchainExtent(GLFWwindow* window, const vk::SurfaceCapabilitiesKHR& vCapabilities) const noexcept {
        if (vCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
            return vCapabilities.currentExtent;

        int width;
        int height;
        glfwGetFramebufferSize(window, &width, &height);

        vk::Extent2D extent{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
        extent.width = std::clamp(extent.width, vCapabilities.minImageExtent.width, vCapabilities.maxImageExtent.width);
        extent.height = std::clamp(extent.height, vCapabilities.minImageExtent.height, vCapabilities.maxImageExtent.height);

        return extent;
    }

    vk::ShaderModule Renderer::createShaderModule(const std::string& filePath, vk::Device& vDevice) const noexcept {
        std::vector<char> sourceCode = service::FileService::read(filePath);
        assert(!sourceCode.empty());
        vk::ShaderModuleCreateInfo moduleInfo{};
        moduleInfo.flags = vk::ShaderModuleCreateFlags();
        moduleInfo.codeSize = sourceCode.size();
        moduleInfo.pCode = reinterpret_cast<const uint32_t*>(sourceCode.data());

        try {
            return vDevice.createShaderModule(moduleInfo);
        } catch ([[maybe_unused]] const vk::SystemError& err) {
#if(TV_DEBUG_MODE)
            Logger::instance().err(std::format("{}: {}\n", constants::messages::VULKAN_SHADER_MODULE_CREATION_FAILED, err.what()));
#endif
        }

        return {};
    }

    vk::PipelineLayout Renderer::createPipelineLayout(vk::Device& vDevice) const noexcept {
        vk::PipelineLayoutCreateInfo layoutInfo;
        layoutInfo.flags = vk::PipelineLayoutCreateFlags();
        layoutInfo.setLayoutCount = 0;

        vk::PushConstantRange pushConstantRange;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(shader::model::Triangle);
        pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex;
        layoutInfo.pPushConstantRanges = &pushConstantRange;
        layoutInfo.pushConstantRangeCount = 1;

        try {
            return vDevice.createPipelineLayout(layoutInfo);
        } catch ([[maybe_unused]] const vk::SystemError& err) {
#if(TV_DEBUG_MODE)
            Logger::instance().err(std::format("{}: {}\n", constants::messages::VULKAN_PIPELINE_LAYOUT_CREATION_FAILED, err.what()));
#endif
        }

        return {};
    }

    vk::RenderPass Renderer::createRenderpass(vk::Device& vDevice, vk::Format vSwapchainImageFormat) const noexcept {
        vk::AttachmentDescription colorAttachment{};
        colorAttachment.flags = vk::AttachmentDescriptionFlags();
        colorAttachment.format = vSwapchainImageFormat;
        colorAttachment.samples = vk::SampleCountFlagBits::e1;
        colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
        colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
        colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
        colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

        vk::AttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::SubpassDescription subpass{};
        subpass.flags = vk::SubpassDescriptionFlags();
        subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        vk::RenderPassCreateInfo renderpassInfo{};
        renderpassInfo.flags = vk::RenderPassCreateFlags();
        renderpassInfo.attachmentCount = 1;
        renderpassInfo.pAttachments = &colorAttachment;
        renderpassInfo.subpassCount = 1;
        renderpassInfo.pSubpasses = &subpass;

        try {
            return vDevice.createRenderPass(renderpassInfo);
        } catch ([[maybe_unused]] const vk::SystemError& err) {
#if(TV_DEBUG_MODE)
            Logger::instance().err(std::format("{}: {}\n", constants::messages::VULKAN_RENDERPASS_CREATION_FAILED, err.what()));
#endif
        }

        return {};
    }

    structures::VGraphicsPipelineBundle Renderer::createGraphicsPipeline(structures::VGraphicsPipelineInBundle& vPipelineInBundle) const noexcept {
        vk::GraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.flags = vk::PipelineCreateFlags();

        std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;

        vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.flags = vk::PipelineVertexInputStateCreateFlags();
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        pipelineInfo.pVertexInputState = &vertexInputInfo;

        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
        inputAssemblyInfo.flags = vk::PipelineInputAssemblyStateCreateFlags();
        inputAssemblyInfo.topology = vk::PrimitiveTopology::eTriangleList;
        pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;

        vk::ShaderModule vertexShader = createShaderModule(vPipelineInBundle.vertexFilepath, vPipelineInBundle.device);
        vk::PipelineShaderStageCreateInfo vertexShaderInfo{};
        vertexShaderInfo.flags = vk::PipelineShaderStageCreateFlags();
        vertexShaderInfo.stage = vk::ShaderStageFlagBits::eVertex;
        vertexShaderInfo.module = vertexShader;
        vertexShaderInfo.pName = constants::config::VULKAN_SHADER_ENTRY_POINT_NAME;
        shaderStages.push_back(vertexShaderInfo);

        vk::Viewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(vPipelineInBundle.swapchainExtent.width);
        viewport.height = static_cast<float>(vPipelineInBundle.swapchainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vk::Rect2D scissor{};
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent = vPipelineInBundle.swapchainExtent;

        vk::PipelineViewportStateCreateInfo viewportState = {};
        viewportState.flags = vk::PipelineViewportStateCreateFlags();
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;
        pipelineInfo.pViewportState = &viewportState;

        vk::PipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.flags = vk::PipelineRasterizationStateCreateFlags();
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = vk::PolygonMode::eFill;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = vk::CullModeFlagBits::eBack;
        rasterizer.frontFace = vk::FrontFace::eClockwise;
        rasterizer.depthBiasEnable = VK_FALSE;
        pipelineInfo.pRasterizationState = &rasterizer;

        vk::ShaderModule fragmentShader = createShaderModule(vPipelineInBundle.fragmentFilepath, vPipelineInBundle.device);
        vk::PipelineShaderStageCreateInfo fragmentShaderInfo{};
        fragmentShaderInfo.flags = vk::PipelineShaderStageCreateFlags();
        fragmentShaderInfo.stage = vk::ShaderStageFlagBits::eFragment;
        fragmentShaderInfo.module = fragmentShader;
        fragmentShaderInfo.pName = constants::config::VULKAN_SHADER_ENTRY_POINT_NAME;
        shaderStages.push_back(fragmentShaderInfo);

        pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineInfo.pStages = shaderStages.data();

        vk::PipelineMultisampleStateCreateInfo multisampling{};
        multisampling.flags = vk::PipelineMultisampleStateCreateFlags();
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
        pipelineInfo.pMultisampleState = &multisampling;

        vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        colorBlendAttachment.blendEnable = VK_FALSE;

        vk::PipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.flags = vk::PipelineColorBlendStateCreateFlags();
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = vk::LogicOp::eCopy;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;
        pipelineInfo.pColorBlendState = &colorBlending;

        vk::PipelineLayout pipelineLayout = createPipelineLayout(vPipelineInBundle.device);
        pipelineInfo.layout = pipelineLayout;

        vk::RenderPass renderpass = createRenderpass(vPipelineInBundle.device, vPipelineInBundle.swapchainImageFormat);
        pipelineInfo.renderPass = renderpass;
        pipelineInfo.subpass = 0;

        pipelineInfo.basePipelineHandle = nullptr;

#if(TV_DEBUG_MODE)
        Logger::instance().log(std::format("{}\n", constants::messages::VULKAN_GRAPHICS_PIPELINE_CREATION_STARTED));
#endif
        vk::Pipeline graphicsPipeline;
        try {
            graphicsPipeline = (vPipelineInBundle.device.createGraphicsPipeline(nullptr, pipelineInfo)).value;
        } catch ([[maybe_unused]] const vk::SystemError& err) {
#if(TV_DEBUG_MODE)
            Logger::instance().err(std::format("{}: {}\n", constants::messages::VULKAN_PIPELINE_CREATION_FAILED, err.what()));
#endif
            return {};
        }

        structures::VGraphicsPipelineBundle pipelineBundle;
        pipelineBundle.layout = pipelineLayout;
        pipelineBundle.renderpass = renderpass;
        pipelineBundle.pipeline = graphicsPipeline;

        vPipelineInBundle.device.destroyShaderModule(vertexShader);
        vPipelineInBundle.device.destroyShaderModule(fragmentShader);

        return pipelineBundle;
    }

    void Renderer::createFramebuffers(vk::Device& vDevice, structures::VGraphicsPipelineBundle& vGraphicsPipelineBundle, structures::VSwapChainBundle& vSwapChainBundle) const noexcept {
        structures::VFramebufferInput frameBufferInput;
        frameBufferInput.device = vDevice;
        frameBufferInput.renderpass = vGraphicsPipelineBundle.renderpass;
        frameBufferInput.swapchainExtent = vSwapChainBundle.extent;

        for (auto& frame : vSwapChainBundle.frames) {
            std::vector<vk::ImageView> attachments = { frame.imageView };

            vk::FramebufferCreateInfo framebufferInfo;
            framebufferInfo.flags = vk::FramebufferCreateFlags();
            framebufferInfo.renderPass = frameBufferInput.renderpass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = frameBufferInput.swapchainExtent.width;
            framebufferInfo.height = frameBufferInput.swapchainExtent.height;
            framebufferInfo.layers = 1;

            try {
                frame.framebuffer = frameBufferInput.device.createFramebuffer(framebufferInfo);
#if(TV_DEBUG_MODE)
                Logger::instance().log(std::format("{}\n", constants::messages::VULKAN_FRAMEBUFFER_CREATED));
#endif
            } catch ([[maybe_unused]] const vk::SystemError& err) {
#if(TV_DEBUG_MODE)
                Logger::instance().err(std::format("{}: {}\n", constants::messages::VULKAN_FRAMEBUFFER_CREATION_FAILED, err.what()));
#endif
            }
        }
    }

    vk::CommandPool Renderer::createCommandPool(vk::Device& vDevice, vk::PhysicalDevice& vPhysicalDevice, vk::SurfaceKHR& vSurface) const noexcept {
#if(TV_DEBUG_MODE)
        Logger::instance().log(std::format("{}\n", constants::messages::VULKAN_COMMAND_POOL_CREATION_STARTED));
#endif
        structures::VQueueFamilyIndices queueFamilyIndices = findQueueFamilies(vPhysicalDevice, vSurface);

        vk::CommandPoolCreateInfo poolInfo;
        poolInfo.flags = vk::CommandPoolCreateFlags() | vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        try {
            return vDevice.createCommandPool(poolInfo);
        } catch ([[maybe_unused]] const vk::SystemError& err) {
#if(TV_DEBUG_MODE)
            Logger::instance().err(std::format("{}: {}\n", constants::messages::VULKAN_COMMAND_POOL_CREATION_FAILED, err.what()));
#endif
        }

        return nullptr;
    }

    void Renderer::createFrameCommandBuffers(structures::VCommandBufferInput& vInputChunk) const noexcept {
        vk::CommandBufferAllocateInfo allocInfo{};
        allocInfo.commandPool = vInputChunk.commandPool;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = 1;

        for (std::size_t i = 0; i < vInputChunk.frames.size(); ++i) {
            try {
                vInputChunk.frames[i].commandBuffer = vInputChunk.device.allocateCommandBuffers(allocInfo)[0];
            } catch ([[maybe_unused]] const vk::SystemError& err) {
#if(TV_DEBUG_MODE)
                Logger::instance().err(std::format("{}: {}\n", constants::messages::VULKAN_COMMAND_BUFFER_ALLOCATION_FAILED, err.what()));
#endif
                return;
            }
        }
    }

    [[nodiscard]] vk::CommandBuffer Renderer::createCommandBuffer(structures::VCommandBufferInput& vInputChunk) const noexcept {
        vk::CommandBufferAllocateInfo allocInfo{};
        allocInfo.commandPool = vInputChunk.commandPool;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = 1;

        try {
            return vInputChunk.device.allocateCommandBuffers(allocInfo)[0];
        } catch ([[maybe_unused]] const vk::SystemError& err) {
#if(TV_DEBUG_MODE)
            Logger::instance().err(std::format("{}: {}\n", constants::messages::VULKAN_MAIN_COMMAND_BUFFER_ALLOCATION_FAILED, err.what()));
#endif
        }

        return nullptr;
    }

    vk::Semaphore Renderer::createSemaphore(vk::Device& vDevice) const noexcept {
        vk::SemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.flags = vk::SemaphoreCreateFlags();

        try {
            return vDevice.createSemaphore(semaphoreInfo);
        } catch (const vk::SystemError&) {
            return nullptr;
        }
    }

    vk::Fence Renderer::createFence(vk::Device& vDevice) const noexcept {
        vk::FenceCreateInfo fenceInfo{};
        fenceInfo.flags = vk::FenceCreateFlags() | vk::FenceCreateFlagBits::eSignaled;

        try {
            return vDevice.createFence(fenceInfo);
        } catch (const vk::SystemError&) {
            return nullptr;
        }
    }

    structures::VSwapChainBundle Renderer::createSwapchain(GLFWwindow* window, vk::Device& vDevice, vk::PhysicalDevice& vPhysicalDevice, vk::SurfaceKHR& vSurface, std::size_t& vMaxFramesInFlight) const noexcept {
#if(TV_DEBUG_MODE)
        Logger::instance().log(std::format("{}\n", constants::messages::VULKAN_SWAPCHAIN_CREATION_STARTED));
#endif
        structures::VSwapChainDetails details = querySwapchainDetails(vPhysicalDevice, vSurface);
        vk::SurfaceFormatKHR format = chooseSwapchainSurfaceFormat(details.formats);
        vk::PresentModeKHR presentMode = chooseSwapchainPresentMode(details.presentMods);
        vk::Extent2D extent = chooseSwapchainExtent(window, details.capabilities);
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
            Logger::instance().err(std::format("{}: {}\n", constants::messages::VULKAN_SWAPCHAIN_CREATION_FAILED, err.what()));
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

            bundle.frames.emplace_back(structures::VSwapChainFrame{
                images[i],
                vDevice.createImageView(imageViewCreateInfo),
                nullptr,
                nullptr,
                {},
                {},
                {}
            });
        }

        bundle.format = format.format;
        bundle.extent = extent;

        vMaxFramesInFlight = bundle.frames.size();

        return bundle;
    }

    void Renderer::resetSwapchain() noexcept {
        std::ranges::for_each(_vSwapChainBundle.frames, [this](structures::VSwapChainFrame& frame) {
            _vDevice.destroyImageView(frame.imageView);
            _vDevice.destroyFramebuffer(frame.framebuffer);

            _vDevice.destroyFence(frame.inFlight);
            _vDevice.destroySemaphore(frame.imageAvailable);
            _vDevice.destroySemaphore(frame.renderFinished);
        });

        _vDevice.destroySwapchainKHR(_vSwapChainBundle.swapChain);
    }

    structures::VGraphicsPipelineBundle Renderer::createPipeline(vk::Device& vDevice, structures::VSwapChainBundle& vSwapchainBundle) const noexcept {
        structures::VGraphicsPipelineInBundle pipelineInBundle{};
        pipelineInBundle.device = vDevice;
        pipelineInBundle.vertexFilepath = constants::path::TRIANGLE_VERTEX_PATH.string();
        pipelineInBundle.fragmentFilepath = constants::path::TRIANGLE_FRAGMENT_PATH.string();
        pipelineInBundle.swapchainExtent = vSwapchainBundle.extent;
        pipelineInBundle.swapchainImageFormat = vSwapchainBundle.format;

        structures::VGraphicsPipelineBundle pipelineBundle = createGraphicsPipeline(pipelineInBundle);
        return pipelineBundle;
    }

    void Renderer::finalSetup(vk::Device& vDevice, vk::PhysicalDevice& vPhysicalDevice, vk::SurfaceKHR vSurface, structures::VGraphicsPipelineBundle& vGraphicsPipelineBundle, structures::VSwapChainBundle& vSwapChainBundle, vk::CommandPool& vCommandPool, vk::CommandBuffer vMainCommandBuffer) const noexcept {
        createFramebuffers(vDevice, vGraphicsPipelineBundle, vSwapChainBundle);

        vCommandPool = createCommandPool(vDevice, vPhysicalDevice, vSurface);

        structures::VCommandBufferInput commandBufferInput = { vDevice, vCommandPool, vSwapChainBundle.frames };
        vMainCommandBuffer = createCommandBuffer(commandBufferInput);
        createFrameCommandBuffers(commandBufferInput);

        createFrameSyncObjects(vDevice, vSwapChainBundle);
    }

    void Renderer::recreateSwapchain() noexcept {
        int windowWidth = 0;
        int windowHeight = 0;
        while (windowWidth == 0 || windowHeight == 0) {
            glfwGetFramebufferSize(_window, &windowWidth, &windowHeight);
            glfwWaitEvents();
        }

        _vDevice.waitIdle();

        resetSwapchain();

        _vSwapChainBundle = createSwapchain(_window, _vDevice, _vPhysicalDevice, _vSurface, _vMaxFramesInFlight);
        createFramebuffers(_vDevice, _vGraphicsPipelineBundle, _vSwapChainBundle);
        createFrameSyncObjects(_vDevice, _vSwapChainBundle);

        structures::VCommandBufferInput commandBufferInput = { _vDevice, _vCommandPool, _vSwapChainBundle.frames };
        createFrameCommandBuffers(commandBufferInput);
    }

    void Renderer::recordDrawCommands(vk::CommandBuffer &vCommandBuffer, uint32_t imageIndex, structures::VGraphicsPipelineBundle& vGraphicsPipelineBundle, structures::VSwapChainBundle& vSwapChainBundle, Scene* scene) const noexcept {
        vk::CommandBufferBeginInfo beginInfo{};

        try {
            vCommandBuffer.begin(beginInfo);
        } catch ([[maybe_unused]] const vk::SystemError& err) {
#if(TV_DEBUG_MODE)
            Logger::instance().err(std::format("{}\n", err.what()));
#endif
            return;
        }

        vk::RenderPassBeginInfo renderPassInfo{};
        renderPassInfo.renderPass = vGraphicsPipelineBundle.renderpass;
        renderPassInfo.framebuffer = vSwapChainBundle.frames[imageIndex].framebuffer;
        renderPassInfo.renderArea.offset.x = 0;
        renderPassInfo.renderArea.offset.y = 0;
        renderPassInfo.renderArea.extent = vSwapChainBundle.extent;

        vk::ClearValue clearColor = { std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f} };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vCommandBuffer.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);
        vCommandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vGraphicsPipelineBundle.pipeline);

        for (const auto& position : scene->getPositions()) {
            auto model = glm::translate(glm::mat4(1.0f), position);
            shader::model::Triangle triangle;
            triangle.model = model;
            vCommandBuffer.pushConstants(vGraphicsPipelineBundle.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(triangle), &triangle);
            vCommandBuffer.draw(3, 1, 0, 0);
        }

        vCommandBuffer.endRenderPass();

        try {
            vCommandBuffer.end();
        } catch ([[maybe_unused]] const vk::SystemError& err) {
#if(TV_DEBUG_MODE)
            Logger::instance().err(std::format("{}\n", err.what()));
#endif
            return;
        }
    }

    void Renderer::createFrameSyncObjects(vk::Device& vDevice, structures::VSwapChainBundle& vSwapChainBundle) const noexcept {
        for (auto& frame : vSwapChainBundle.frames) {
            frame.inFlight = createFence(vDevice);
            frame.imageAvailable = createSemaphore(vDevice);
            frame.renderFinished = createSemaphore(vDevice);
        }
    }

    structures::VSwapChainDetails Renderer::querySwapchainDetails(const vk::PhysicalDevice &vDevice, vk::SurfaceKHR &vSurface) const noexcept {
        structures::VSwapChainDetails details;
        details.capabilities = vDevice.getSurfaceCapabilitiesKHR(vSurface);
        details.formats = vDevice.getSurfaceFormatsKHR(vSurface);
        details.presentMods = vDevice.getSurfacePresentModesKHR(vSurface);

        return details;
    }

    vk::Instance Renderer::createInstance() const noexcept {
        assert(glfwVulkanSupported());
        auto& logger = Logger::instance();

        uint32_t vulkanVersion{ 0 };
        vkEnumerateInstanceVersion(&vulkanVersion);

        uint32_t vulkanExtensionCount = 0;
        auto rawGlfwExtensions = glfwGetRequiredInstanceExtensions(&vulkanExtensionCount);
        std::vector<const char*> vulkanExtensions{ rawGlfwExtensions, rawGlfwExtensions + vulkanExtensionCount };
        std::vector<const char*> vulkanLayers{};

#if(TV_DEBUG_MODE)
        vulkanExtensions.emplace_back(constants::config::VULKAN_EXT_DEBUG);
        vulkanLayers.emplace_back(constants::config::VULKAN_LAYER_VALIDATION);
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
            constants::config::APP_NAME,
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

    void Renderer::createSurface(GLFWwindow* window, vk::Instance& vInstance, vk::SurfaceKHR& vSurface) const noexcept {
        VkSurfaceKHR surface{};
        assert(window != nullptr);
        if (glfwCreateWindowSurface(vInstance, window, nullptr, &surface) != VK_SUCCESS) {
#if(TV_DEBUG_MODE)
            Logger::instance().err(std::format("{}\n", constants::messages::VULKAN_SURFACE_CREATION_FAILED));
#endif
        }

        vSurface = surface;
    }

    vk::DebugUtilsMessengerEXT Renderer::createDebugMessenger([[maybe_unused]] vk::Instance& vInstance) const noexcept {
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

    vk::PhysicalDevice Renderer::chooseDevice(const vk::Instance& vInstance) const noexcept {
        const std::vector<vk::PhysicalDevice> availableDevices = vInstance.enumeratePhysicalDevices();
        if (availableDevices.empty()) {
            Logger::instance().err(std::format("{}\n", constants::messages::VULKAN_NO_AVAILABLE_DEVICE));
            return nullptr;
        }

        for (const vk::PhysicalDevice& device : availableDevices) {
#if(TV_DEBUG_MODE)
            Logger::instance().log(std::format("{}: {}\n", constants::messages::VULKAN_DEVICE_NAME, device.getProperties().deviceName.data()));
#endif
            if (deviceIsSuitable(device))
                return device;
        }

        return nullptr;
    }

    vk::Device Renderer::createLogicalDevice(vk::PhysicalDevice& vPhysicalDevice, vk::SurfaceKHR& vSurface) const noexcept {
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
        for (const uint32_t index : uniqueFamilyIndices) {
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
        enabledLayers.emplace_back(constants::config::VULKAN_LAYER_VALIDATION);
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

    std::vector<vk::Queue> Renderer::getQueues(const vk::PhysicalDevice& vPhysicalDevice, vk::Device& vDevice, vk::SurfaceKHR& vSurface) const noexcept {
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

