#pragma once

#include <vector>

#include <glm.hpp>

namespace tv {
    class Scene {
    public:
        Scene() noexcept;

        ~Scene() = default;

        const std::vector<glm::vec3>& getPositions() const noexcept;

    private:
        std::vector<glm::vec3> _trianglePositions;
    };
}
