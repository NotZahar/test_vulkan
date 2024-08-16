#pragma once

#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>

#include <vulkan/vulkan.hpp>

#include "../utility/types.hpp"
#include "../utility/structures.hpp"
#include "../scene/scene.hpp"

namespace tv {
    class Renderer {
    public:
        TV_NCM(Renderer)

        static Renderer& instance() noexcept;
        static void setup(Renderer& renderer, GLFWwindow* window) noexcept;
        void render(Scene* scene) noexcept;

    private:
        Renderer() noexcept;

        ~Renderer();

        void init(GLFWwindow* window) noexcept;

        [[nodiscard]] vk::Instance createInstance() const noexcept;
        void createSurface(GLFWwindow* window, vk::Instance& vInstance, vk::SurfaceKHR& vSurface) const noexcept;
        [[nodiscard]] vk::DebugUtilsMessengerEXT createDebugMessenger(vk::Instance& vInstance) const noexcept;
        [[nodiscard]] vk::PhysicalDevice chooseDevice(const vk::Instance& vInstance) const noexcept;
        [[nodiscard]] vk::Device createLogicalDevice(vk::PhysicalDevice& vPhysicalDevice, vk::SurfaceKHR& vSurface) const noexcept;
        [[nodiscard]] std::vector<vk::Queue> getQueues(const vk::PhysicalDevice& vPhysicalDevice, vk::Device& vDevice, vk::SurfaceKHR& vSurface) const noexcept;
        [[nodiscard]] structures::VSwapChainBundle createSwapchain(GLFWwindow* window, vk::Device& vDevice, vk::PhysicalDevice& vPhysicalDevice, vk::SurfaceKHR& vSurface, std::size_t& vMaxFramesInFlight) const noexcept;
        void resetSwapchain() noexcept;
        [[nodiscard]] structures::VGraphicsPipelineBundle createPipeline(vk::Device& vDevice, structures::VSwapChainBundle& vSwapchainBundle) const noexcept;
        void finalSetup(vk::Device& vDevice, vk::PhysicalDevice& vPhysicalDevice, vk::SurfaceKHR vSurface, structures::VGraphicsPipelineBundle& vGraphicsPipelineBundle, structures::VSwapChainBundle& vSwapChainBundle, vk::CommandPool& vCommandPool, vk::CommandBuffer vMainCommandBuffer) const noexcept;
        void recreateSwapchain() noexcept;

        void printAdditionalInfo(const uint32_t vulkanVersion, const std::vector<const char*>& glfwExtensions) const noexcept;
        [[nodiscard]] bool deviceIsSuitable(const vk::PhysicalDevice& vDevice) const noexcept;
        [[nodiscard]] structures::VQueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice& vPhysicalDevice, vk::SurfaceKHR& vSurface) const noexcept;
        [[nodiscard]] bool extensionsSupported(const std::vector<const char*>& vulkanExtensions) const noexcept;
        [[nodiscard]] bool layersSupported(const std::vector<const char*>& vulkanLayers) const noexcept;
        [[nodiscard]] structures::VSwapChainDetails querySwapchainDetails(const vk::PhysicalDevice& vDevice, vk::SurfaceKHR& vSurface) const noexcept;
        [[nodiscard]] vk::SurfaceFormatKHR chooseSwapchainSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& vFormats) const noexcept;
        [[nodiscard]] vk::PresentModeKHR chooseSwapchainPresentMode(const std::vector<vk::PresentModeKHR>& vPresentMods) const noexcept;
        [[nodiscard]] vk::Extent2D chooseSwapchainExtent(GLFWwindow* window, const vk::SurfaceCapabilitiesKHR& vCapabilities) const noexcept;
        [[nodiscard]] vk::ShaderModule createShaderModule(const std::string& filePath, vk::Device& vDevice) const noexcept;
        [[nodiscard]] vk::PipelineLayout createPipelineLayout(vk::Device& vDevice) const noexcept;
        [[nodiscard]] vk::RenderPass createRenderpass(vk::Device& vDevice, vk::Format vSwapchainImageFormat) const noexcept;
        [[nodiscard]] structures::VGraphicsPipelineBundle createGraphicsPipeline(structures::VGraphicsPipelineInBundle& vPipelineInBundle) const noexcept;
        void createFramebuffers(vk::Device& vDevice, structures::VGraphicsPipelineBundle& vGraphicsPipelineBundle, structures::VSwapChainBundle& vSwapChainBundle) const noexcept;
        [[nodiscard]] vk::CommandPool createCommandPool(vk::Device& vDevice, vk::PhysicalDevice& vPhysicalDevice, vk::SurfaceKHR& vSurface) const noexcept;
        void createFrameCommandBuffers(structures::VCommandBufferInput& vInputChunk) const noexcept;
        [[nodiscard]] vk::CommandBuffer createCommandBuffer(structures::VCommandBufferInput& vInputChunk) const noexcept;
        [[nodiscard]] vk::Semaphore createSemaphore(vk::Device& vDevice) const noexcept;
        [[nodiscard]] vk::Fence createFence(vk::Device& vDevice) const noexcept;
        void recordDrawCommands(vk::CommandBuffer& vCommandBuffer, uint32_t imageIndex, structures::VGraphicsPipelineBundle& vGraphicsPipelineBundle, structures::VSwapChainBundle& vSwapChainBundle, Scene* scene) const noexcept;
        void createFrameSyncObjects(vk::Device& vDevice, structures::VSwapChainBundle& vSwapChainBundle) const noexcept;

        GLFWwindow* _window;
        vk::Instance _vInstance;
        vk::PhysicalDevice _vPhysicalDevice;
        vk::Device _vDevice;
        vk::Queue _vGraphicsQueue;
        vk::Queue _vPresentQueue;
        structures::VSwapChainBundle _vSwapChainBundle;
        vk::DebugUtilsMessengerEXT _vDebugMessenger;
        vk::DispatchLoaderDynamic _vDispatchLoaderDynamic;
        vk::SurfaceKHR _vSurface;
        structures::VGraphicsPipelineBundle _vGraphicsPipelineBundle;
        vk::CommandPool _vCommandPool;
        vk::CommandBuffer _vMainCommandBuffer;
        std::size_t _vMaxFramesInFlight;
        std::size_t _vFrameNumber;
    };
}
