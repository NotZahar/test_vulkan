#pragma once

#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>

#include "../render/renderer.hpp"
#include "../utility/types.hpp"
#include "../scene/scene.hpp"

namespace tv::ui {
    class MainWindow {
    public:
        TV_NCM(MainWindow)

        static MainWindow& instance() noexcept;

        GLFWwindow* getWindow() noexcept;
        void processEvents(Renderer& renderer, Scene* scene) noexcept;

    private:
        MainWindow() noexcept;

        ~MainWindow();

        void calculateFrameRate() noexcept;

        GLFWwindow* _window;
        double _lastTime;
        double _currentTime;
        int _numFrames;
        float _frameTime;
    };
}
