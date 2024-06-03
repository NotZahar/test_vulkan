#pragma once

#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>

#include <vulkan/vulkan.hpp>

#include "../utility/types.hpp"
#include "../utility/structures.hpp"

namespace tv {
    class Renderer {
    public:
        TV_NCM(Renderer)

        static Renderer& instance() noexcept;

        void processEvents() noexcept;

    private:
        Renderer() noexcept;

        ~Renderer();

        [[nodiscard]] vk::Instance makeVInstance() const noexcept;
        void createVSurface(vk::Instance& vInstance, vk::SurfaceKHR& vSurface) const noexcept;
        [[nodiscard]] vk::DebugUtilsMessengerEXT makeVDebugMessenger(vk::Instance& vInstance) const noexcept;
        [[nodiscard]] vk::PhysicalDevice chooseVDevice(const vk::Instance& vInstance) const noexcept;
        [[nodiscard]] vk::Device createVLogicalDevice(vk::PhysicalDevice& vPhysicalDevice, vk::SurfaceKHR& vSurface) const noexcept;
        [[nodiscard]] std::vector<vk::Queue> getVQueues(const vk::PhysicalDevice& vPhysicalDevice, vk::Device& vDevice, vk::SurfaceKHR& vSurface) const noexcept;

        void printAdditionalInfo(const uint32_t vulkanVersion, const std::vector<const char*>& glfwExtensions) const noexcept;
        [[nodiscard]] bool deviceIsSuitable(const vk::PhysicalDevice& vDevice) const noexcept;
        [[nodiscard]] structures::VQueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice& vPhysicalDevice, vk::SurfaceKHR& vSurface) const noexcept;
        [[nodiscard]] bool extensionsSupported(const std::vector<const char*>& vulkanExtensions) const noexcept;
        [[nodiscard]] bool layersSupported(const std::vector<const char*>& vulkanLayers) const noexcept;
        [[nodiscard]] structures::VSwapChainDetails querySwapchainDetails(const vk::PhysicalDevice& vDevice, vk::SurfaceKHR& vSurface) const noexcept;
        [[nodiscard]] vk::SurfaceFormatKHR chooseSwapchainSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& vFormats) const noexcept;
        [[nodiscard]] vk::PresentModeKHR chooseSwapchainPresentMode(const std::vector<vk::PresentModeKHR>& vPresentMods) const noexcept;
        [[nodiscard]] vk::Extent2D chooseSwapchainExtent(const vk::SurfaceCapabilitiesKHR& vCapabilities) const noexcept;
        [[nodiscard]] structures::VSwapChainBundle createSwapchain(vk::Device& vDevice, vk::PhysicalDevice& vPhysicalDevice, vk::SurfaceKHR& vSurface) const noexcept;

        void initWindow() noexcept;

        GLFWwindow* _mainWindow;
        vk::Instance _vInstance;
        vk::PhysicalDevice _vPhysicalDevice;
        vk::Device _vDevice;
        vk::Queue _vGraphicsQueue;
        vk::Queue _vPresentQueue;
        structures::VSwapChainBundle _vSwapChainBundle;
        vk::DebugUtilsMessengerEXT _vDebugMessenger;
        vk::DispatchLoaderDynamic _vDispatchLoaderDynamic;
        vk::SurfaceKHR _vSurface;
    };
}
