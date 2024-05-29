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

        vk::Instance makeVInstance() noexcept;
        vk::DebugUtilsMessengerEXT makeVDebugMessenger() noexcept;
        void printAdditionalInfo(const uint32_t vulkanVersion, const std::vector<const char*>& glfwExtensions) noexcept;
        bool vExtensionsSupported(const std::vector<const char*>& vulkanExtensions) noexcept;
        bool vLayersSupported(const std::vector<const char*>& vulkanLayers) noexcept;

        inline static constexpr int _windowWidth = 800;
        inline static constexpr int _windowHeight = 600;

        void initWindow() noexcept;

        GLFWwindow* _mainWindow;
        vk::Instance _vInstance;
        vk::DebugUtilsMessengerEXT _vDebugMessenger;
        vk::DispatchLoaderDynamic _vDispatchLoaderDynamic;
    };
}
