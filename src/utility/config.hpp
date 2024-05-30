#pragma once

#include <string>

namespace tv::constants {
    struct config {
        // general
        inline static const std::string WINDOW_TITLE = "Test Vulkan";
        inline static constexpr int WINDOW_MAIN_WIDTH = 800;
        inline static constexpr int WINDOW_MAIN_HEIGHT = 600;
        inline static const std::string APP_NAME = "test vulkan";

        // vulkan
        inline static const std::string VULKAN_EXT_DEBUG = "VK_EXT_debug_utils";
        inline static const std::string VULKAN_LAYER_VALIDATION = "VK_LAYER_KHRONOS_validation";
    };
}
