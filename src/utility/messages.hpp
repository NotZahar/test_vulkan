#pragma once

#include <string>

namespace tv::constants {
    struct messages {
        // general
        inline static const std::string WINDOW_TITLE = "Test Vulkan";
        inline static const std::string APP_NAME = "test vulkan";

        // vulkan
        inline static const std::string VULKAN_CONFIG_VARIANT = "Vulkan variant";
        inline static const std::string VULKAN_CONFIG_MAJOR = "Vulkan major";
        inline static const std::string VULKAN_CONFIG_MINOR = "Vulkan minor";
        inline static const std::string VULKAN_CONFIG_PATCH = "Vulkan patch";

        // glfw
        inline static const std::string GLFW_EXTENSIONS = "Requested GLFW extensions";

        // errors
        inline static const std::string VULKAN_INSTANCE_CREATION_FAILED = "Failed to create Vulkan instance";
    };
}
