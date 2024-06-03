#pragma once

#include <optional>
#include <vector>
#include <cstdint>

#include <vulkan/vulkan.hpp>

namespace tv::structures {
    struct VQueueFamilyIndices {
        bool isComplete() {
            return graphicsFamily && presentFamily;
        }

        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
    };

    struct VSwapChainDetails {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentMods;
    };

    struct VSwapChainBundle {
        vk::SwapchainKHR swapChain{ nullptr };
        std::vector<vk::Image> images{ nullptr };
        vk::Format format;
        vk::Extent2D extent;
    };
}
