#pragma once

#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>

#include "../utility/types.hpp"

namespace tv::ui {
    class MainWindow {
    public:
        TV_NCM(MainWindow)

        static MainWindow& instance() noexcept;

        GLFWwindow* getWindow() noexcept;
        void processEvents() noexcept;

    private:
        MainWindow() noexcept;

        ~MainWindow();

        GLFWwindow* _window;
    };
}
