#include "scene.hpp"

namespace tv {
    Scene::Scene() noexcept
    {
        for (int x = -10; x < 10; x += 2)
            for (int y = -10; y < 10; y += 2)
                _trianglePositions.emplace_back(glm::vec3((float)x / 10, (float)y / 10, 0));
    }

    const std::vector<glm::vec3>& Scene::getPositions() const noexcept {
        return _trianglePositions;
    }
}
