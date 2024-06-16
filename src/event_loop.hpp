#pragma once

#include "utility/types.hpp"

namespace tv {
    class EventLoop {
    public:
        TV_NCM(EventLoop)

        static EventLoop& instance() noexcept;

    private:
        constexpr EventLoop() noexcept = default;

        ~EventLoop() = default;
    };
}
