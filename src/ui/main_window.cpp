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

    void MainWindow::processEvents() noexcept {
        while (!glfwWindowShouldClose(_window))
            glfwPollEvents();
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
}

