#pragma once

#include <vector>
#include <string>

namespace tv::service {
    class FileService {
    public:
        FileService() = default;

        ~FileService() = default;

        static std::vector<char> read(const std::string& filePath) noexcept;
    };
}
