#pragma once

namespace tv::constants {
    struct config {
        // general
        inline static constexpr char APP_NAME[] = "test vulkan";
        inline static constexpr char WINDOW_TITLE[] = "Test Vulkan";
        inline static constexpr int WINDOW_MAIN_WIDTH = 800;
        inline static constexpr int WINDOW_MAIN_HEIGHT = 600;

        // vulkan
        inline static constexpr char VULKAN_EXT_DEBUG[] = "VK_EXT_debug_utils";
        inline static constexpr char VULKAN_LAYER_VALIDATION[] = "VK_LAYER_KHRONOS_validation";
    };
}
