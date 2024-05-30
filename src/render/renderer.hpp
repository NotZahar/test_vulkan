#pragma once

#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>

#include <vulkan/vulkan.hpp>

#include "../utility/types.hpp"

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
        [[nodiscard]] vk::DebugUtilsMessengerEXT makeVDebugMessenger() const noexcept;
        [[nodiscard]] vk::PhysicalDevice chooseDevice() const noexcept;
        void printAdditionalInfo(const uint32_t vulkanVersion, const std::vector<const char*>& glfwExtensions) const noexcept;
        bool vExtensionsSupported(const std::vector<const char*>& vulkanExtensions) const noexcept;
        bool vLayersSupported(const std::vector<const char*>& vulkanLayers) const noexcept;

        void initWindow() noexcept;

        GLFWwindow* _mainWindow;
        vk::Instance _vInstance;
        vk::PhysicalDevice _physicalDevice;
        vk::DebugUtilsMessengerEXT _vDebugMessenger;
        vk::DispatchLoaderDynamic _vDispatchLoaderDynamic;
    };
}
