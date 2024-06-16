#pragma once

namespace tv::constants {
    struct messages {
        // vulkan
        inline static constexpr char VULKAN_API_VERSION[] = "Vulkan API version";
        inline static constexpr char VULKAN_EXTENSIONS[] = "Supported extensions";
        inline static constexpr char VULKAN_LAYERS[] = "Supported layers";
        inline static constexpr char VULKAN_REQUESTED_EXTENSIONS[] = "Requested extensions";
        inline static constexpr char VULKAN_EXTENSION_SUPPORTED[] = "Extension supported";
        inline static constexpr char VULKAN_LAYER_SUPPORTED[] = "Layer supported";
        inline static constexpr char VULKAN_DEVICE_NAME[] = "Device name";
        inline static constexpr char VULKAN_DEVICE_QUEUE_FAMILIES[] = "Available queue families";
        inline static constexpr char VULKAN_DEVICE_CREATION_STARTED[] = "Logical device creation started";
        inline static constexpr char VULKAN_SWAPCHAIN_CREATION_STARTED[] = "Swapchain creation started";
        inline static constexpr char VULKAN_GETTING_QUEUE_STARTED[] = "Getting queue started";
        inline static constexpr char VULKAN_GRAPHICS_PIPELINE_CREATION_STARTED[] = "Graphics pipeline creation started";
        inline static constexpr char VULKAN_FRAMEBUFFER_CREATED[] = "Framebuffer created";
        inline static constexpr char VULKAN_COMMAND_POOL_CREATION_STARTED[] = "Command pool creation started";

        // errors
        inline static constexpr char VULKAN_INSTANCE_CREATION_FAILED[] = "Failed to create Vulkan instance";
        inline static constexpr char VULKAN_SOME_EXTENSIONS_NOT_SUPPORTED[] = "Some extensions not supported";
        inline static constexpr char VULKAN_SOME_LAYERS_NOT_SUPPORTED[] = "Some layers not supported";
        inline static constexpr char VULKAN_EXTENSION_NOT_SUPPORTED[] = "Extension not supported";
        inline static constexpr char VULKAN_LAYER_NOT_SUPPORTED[] = "Layer not supported";
        inline static constexpr char VULKAN_NO_AVAILABLE_DEVICE[] = "Failed to find supported device";
        inline static constexpr char VULKAN_DEVICE_CREATION_FAILED[] = "Device creation failed";
        inline static constexpr char VULKAN_SURFACE_CREATION_FAILED[] = "Surface creation failed";
        inline static constexpr char VULKAN_SWAPCHAIN_CREATION_FAILED[] = "Swapchain creation failed";
        inline static constexpr char VULKAN_SHADER_MODULE_CREATION_FAILED[] = "Failed to create shader module";
        inline static constexpr char VULKAN_PIPELINE_LAYOUT_CREATION_FAILED[] = "Failed to create pipeline layout";
        inline static constexpr char VULKAN_RENDERPASS_CREATION_FAILED[] = "Failed to create renderpass";
        inline static constexpr char VULKAN_PIPELINE_CREATION_FAILED[] = "Pipeline creation failed";
        inline static constexpr char VULKAN_FRAMEBUFFER_CREATION_FAILED[] = "Failed to create framebuffer";
        inline static constexpr char VULKAN_COMMAND_POOL_CREATION_FAILED[] = "Failed to create command pool";
        inline static constexpr char VULKAN_COMMAND_BUFFER_ALLOCATION_FAILED[] = "Failed to allocate command buffer";
        inline static constexpr char VULKAN_MAIN_COMMAND_BUFFER_ALLOCATION_FAILED[] = "Failed to allocate main command buffer";

        inline static constexpr char FILE_DONT_EXIST[] = "File does not exist";
    };
}
