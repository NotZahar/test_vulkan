#include "file_service.hpp"

#include <fstream>
#include <format>
#include <cassert>

#include "../logger.hpp"
#include "../utility/messages.hpp"

namespace tv::service {
    std::vector<char> FileService::read(const std::string& filePath) noexcept {
        std::ifstream file{ filePath, std::ios::ate | std::ios::binary };
        if (!file.is_open()) {
            Logger::instance().err(std::format("{}\n", constants::messages::FILE_DONT_EXIST));
            return {};
        }

        const std::size_t fileSize = file.tellg();
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        assert(buffer.size() == fileSize);

        file.close();
        return buffer;
    }
}
