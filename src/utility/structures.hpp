#pragma once

#include <optional>
#include <cstdint>

namespace tv::structures {
    struct VQueueFamilyIndices {
        bool isComplete() {
            return graphicsFamily && presentFamily;
        }

        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
    };
}
