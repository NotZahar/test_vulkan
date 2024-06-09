#pragma once

namespace tv::constants {
    struct messages {
        // vulkan
        inline static const char VULKAN_API_VERSION[] = "Vulkan API version";
        inline static const char VULKAN_EXTENSIONS[] = "Supported extensions";
        inline static const char VULKAN_LAYERS[] = "Supported layers";
        inline static const char VULKAN_REQUESTED_EXTENSIONS[] = "Requested extensions";
        inline static const char VULKAN_EXTENSION_SUPPORTED[] = "Extension supported";
        inline static const char VULKAN_LAYER_SUPPORTED[] = "Layer supported";
        inline static const char VULKAN_DEVICE_NAME[] = "Device name";
        inline static const char VULKAN_DEVICE_QUEUE_FAMILIES[] = "Available queue families";
        inline static const char VULKAN_DEVICE_CREATION_STARTED[] = "Logical device creation started";
        inline static const char VULKAN_SWAPCHAIN_CREATION_STARTED[] = "Swapchain creation started";
        inline static const char VULKAN_GETTING_QUEUE_STARTED[] = "Getting queue started";

        // errors
        inline static const char VULKAN_INSTANCE_CREATION_FAILED[] = "Failed to create Vulkan instance";
        inline static const char VULKAN_SOME_EXTENSIONS_NOT_SUPPORTED[] = "Some extensions not supported";
        inline static const char VULKAN_SOME_LAYERS_NOT_SUPPORTED[] = "Some layers not supported";
        inline static const char VULKAN_EXTENSION_NOT_SUPPORTED[] = "Extension not supported";
        inline static const char VULKAN_LAYER_NOT_SUPPORTED[] = "Layer not supported";
        inline static const char VULKAN_NO_AVAILABLE_DEVICE[] = "Failed to find supported device";
        inline static const char VULKAN_DEVICE_CREATION_FAILED[] = "Device creation failed";
        inline static const char VULKAN_SURFACE_CREATION_FAILED[] = "Surface creation failed";
        inline static const char VULKAN_SWAPCHAIN_CREATION_FAILED[] = "Swapchain creation failed";
    };
}
