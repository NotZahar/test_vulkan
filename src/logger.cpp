#include "logger.hpp"

#include <iostream>

namespace tv {
    Logger& Logger::instance() noexcept {
        static Logger instance;
        return instance;
    }

    void Logger::log(std::string_view message) const noexcept {
        std::cout << message;
    }

    void Logger::err(std::string_view message) const noexcept {
        std::cerr << message;
    }
}
