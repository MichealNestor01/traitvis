#include "shaderSource.hpp"

#include <cstdio>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {

struct FileDeleter {
    void operator()(std::FILE* file) const noexcept {
        if (file != nullptr) std::fclose(file);
    }
};

} // namespace

std::string readShaderSource(const std::string& path) {
    std::ostringstream msg;

    std::unique_ptr<std::FILE, FileDeleter> input(std::fopen(path.c_str(), "rb"));
    if (!input) {
        msg << "readShaderSource(): unable to open file \"" << path << "\"";
        throw std::runtime_error(msg.str());
    }

    std::fseek(input.get(), 0, SEEK_END);
    const std::size_t length = static_cast<std::size_t>(std::ftell(input.get()));
    std::fseek(input.get(), 0, SEEK_SET);

    std::string contents(length, '\0');
    for (std::size_t totalRead = 0; totalRead != length;) {
        const auto numRead = std::fread(contents.data() + totalRead, 1, length - totalRead, input.get());
        if (numRead == 0) {
            if (const auto err = std::ferror(input.get())) {
                msg << "readShaderSource(): error while reading from \"" << path << "\": " << err
                    << " (" << totalRead << " bytes read, " << length << " total)";
                throw std::runtime_error(msg.str());
            }
            if (std::feof(input.get())) {
                msg << "readShaderSource(): unexpected EOF in \"" << path << "\": ("
                    << totalRead << " bytes read, " << length << " total)";
                throw std::runtime_error(msg.str());
            }
        }
        totalRead += numRead;
    }

    return contents;
}
