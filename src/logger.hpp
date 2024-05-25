#pragma once

#include <string_view>

#include "utility/types.hpp"

namespace tv {
    class Logger {
    public:
        TV_NCM(Logger)

        static Logger& instance() noexcept;

        void log(std::string_view message) const noexcept;
        void err(std::string_view message) const noexcept;

    private:
        constexpr Logger() noexcept = default;

        ~Logger() = default;
    };
}
