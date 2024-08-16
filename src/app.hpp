#pragma once

#include "utility/types.hpp"

namespace tv {
    class App {
    public:
        TV_NCM(App)

        static App& instance() noexcept;

    private:
        App();

        ~App() = default;
    };
}
