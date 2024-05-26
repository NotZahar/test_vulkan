#pragma once

#include <string>

namespace tv::constants {
    struct config {
        // general
        inline static const std::string WINDOW_TITLE = "Test Vulkan";
        inline static const std::string APP_NAME = "test vulkan";

        // vulkan
        inline static const std::string VULKAN_EXT_DEBUG = "VK_EXT_debug_utils";
        inline static const std::string VULKAN_LAYER_VALIDATION = "VK_LAYER_KHRONOS_validation";
    };
}
