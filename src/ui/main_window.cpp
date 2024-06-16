#include "main_window.hpp"

#include <sstream>

#include "../utility/config.hpp"

namespace tv::ui {
    MainWindow& MainWindow::instance() noexcept {
        static MainWindow instance;
        return instance;
    }

    GLFWwindow *MainWindow::getWindow() noexcept {
        return _window;
    }

    void MainWindow::processEvents(Renderer* renderer) noexcept {
        while (!glfwWindowShouldClose(_window)) {
            glfwPollEvents();
            renderer->render();
            calculateFrameRate();
        }
    }

    MainWindow::MainWindow() noexcept {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

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

    void MainWindow::calculateFrameRate() noexcept {
        _currentTime = glfwGetTime();
        double delta = _currentTime - _lastTime;

        if (delta >= 1) {
            int framerate{ std::max(1, int(_numFrames / delta)) };
            std::stringstream title;
            title << constants::config::WINDOW_TITLE << " " << framerate << " fps";
            glfwSetWindowTitle(_window, title.str().c_str());
            _lastTime = _currentTime;
            _numFrames = -1;
            _frameTime = float(1000.0 / framerate);
        }

        ++_numFrames;
    }
}

