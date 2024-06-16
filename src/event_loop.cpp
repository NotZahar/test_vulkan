#include "event_loop.hpp"

namespace tv {
    EventLoop& EventLoop::instance() noexcept {
        static EventLoop instance;
        return instance;
    }
}
