#include "app.hpp"

#include <memory>

#include "ui/main_window.hpp"
#include "render/renderer.hpp"
#include "scene/scene.hpp"

namespace tv {
    App::App()
    {
        auto& mainWindow = tv::ui::MainWindow::instance();
        auto& renderer = tv::Renderer::instance();
        std::unique_ptr<Scene> scene = std::make_unique<Scene>();
        tv::Renderer::setup(renderer, mainWindow.getWindow());

        mainWindow.processEvents(renderer, scene.get());
    }

    App& App::instance() noexcept {
        static App instance;
        return instance;
    }
}
