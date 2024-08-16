#include "main_window.hpp"

#include "../utility/config.hpp"

namespace tv::ui {
    MainWindow& MainWindow::instance() noexcept {
        static MainWindow instance;
        return instance;
    }

    GLFWwindow *MainWindow::getWindow() noexcept {
        return _window;
    }

    void MainWindow::processEvents(Renderer& renderer, Scene* scene) noexcept {
        while (!glfwWindowShouldClose(_window)) {
            glfwPollEvents();
            renderer.render(scene);
            drawFrameRate();
        }
    }

    MainWindow::MainWindow() noexcept {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        _window = glfwCreateWindow(
            constants::config::WINDOW_MAIN_WIDTH,
            constants::config::WINDOW_MAIN_HEIGHT,
            constants::config::WINDOW_TITLE,
            nullptr,
            nullptr
        );
    }

    MainWindow::~MainWindow() {
        glfwDestroyWindow(_window);
        glfwTerminate();
    }

    void MainWindow::drawFrameRate() noexcept {
        static int numberOfFrames = 0;
        static double lastTime = 0;
        const double currentTime = glfwGetTime();
        const double delta = currentTime - lastTime;

        if (delta >= 1) {
            assert(delta != 0);
            const int frameRate = std::max(1, numberOfFrames / (int)delta);
            glfwSetWindowTitle(_window, std::format("{} in {} fps", constants::config::WINDOW_TITLE, frameRate).c_str());
            lastTime = currentTime;
            numberOfFrames = -1;
        }

        ++numberOfFrames;
    }
}

