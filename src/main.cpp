#include "ui/main_window.hpp"
#include "render/renderer.hpp"

int main() {
    auto& mainWindow = tv::ui::MainWindow::instance();
    auto& renderer = tv::Renderer::instance();
    tv::Renderer::setup(renderer, mainWindow.getWindow());

    mainWindow.processEvents(&renderer);

    return 0;
}
