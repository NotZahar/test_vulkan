#pragma once

#include <filesystem>

namespace tv::constants {
    struct path {
        inline static const std::filesystem::path BUILD_PATH = std::filesystem::current_path() / "..";
        inline static const std::filesystem::path SHADERS_PATH = BUILD_PATH / "shaders";
        inline static const std::filesystem::path TRIANGLE_VERTEX_PATH = SHADERS_PATH / "triangle.vert.spv";
        inline static const std::filesystem::path TRIANGLE_FRAGMENT_PATH = SHADERS_PATH / "triangle.frag.spv";
    };
}
