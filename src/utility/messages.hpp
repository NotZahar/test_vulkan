#pragma once

#include <string>

namespace tv::constants {
    struct messages {
        // vulkan
        inline static const std::string VULKAN_API_VERSION = "Vulkan API version";
        inline static const std::string VULKAN_EXTENSIONS = "Supported extensions";
        inline static const std::string VULKAN_LAYERS = "Supported layers";
        inline static const std::string VULKAN_REQUESTED_EXTENSIONS = "Requested extensions";
        inline static const std::string VULKAN_EXTENSION_SUPPORTED = "Extension supported";
        inline static const std::string VULKAN_LAYER_SUPPORTED = "Layer supported";
        inline static const std::string VULKAN_DEVICE_NAME = "Device name";

        // errors
        inline static const std::string VULKAN_INSTANCE_CREATION_FAILED = "Failed to create Vulkan instance";
        inline static const std::string VULKAN_SOME_EXTENSIONS_NOT_SUPPORTED = "Some extensions not supported";
        inline static const std::string VULKAN_SOME_LAYERS_NOT_SUPPORTED = "Some layers not supported";
        inline static const std::string VULKAN_EXTENSION_NOT_SUPPORTED = "Extension not supported";
        inline static const std::string VULKAN_LAYER_NOT_SUPPORTED = "Layer not supported";
    };
}
