#include "render/renderer.hpp"

int main() {
    auto& renderer = tv::Renderer::instance();
    renderer.processEvents();

    return 0;
}
