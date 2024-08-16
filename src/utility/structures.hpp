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

    struct VSwapChainFrame {
        vk::Image image;
        vk::ImageView imageView;
        vk::Framebuffer framebuffer;
        vk::CommandBuffer commandBuffer;
        vk::Semaphore imageAvailable;
        vk::Semaphore renderFinished;
        vk::Fence inFlight;
    };

    struct VSwapChainBundle {
        vk::SwapchainKHR swapChain;
        std::vector<VSwapChainFrame> frames;
        vk::Format format;
        vk::Extent2D extent;
    };

    struct VGraphicsPipelineInBundle {
        vk::Device device;
        std::string vertexFilepath;
        std::string fragmentFilepath;
        vk::Extent2D swapchainExtent;
        vk::Format swapchainImageFormat;
    };

    struct VGraphicsPipelineBundle {
        vk::PipelineLayout layout;
        vk::RenderPass renderpass;
        vk::Pipeline pipeline;
    };

    struct VFramebufferInput {
        vk::Device device;
        vk::RenderPass renderpass;
        vk::Extent2D swapchainExtent;
    };

    struct VCommandBufferInput {
        vk::Device device;
        vk::CommandPool commandPool;
        std::vector<VSwapChainFrame>& frames;
    };
}
