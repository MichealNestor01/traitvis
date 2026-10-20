#include "pngImage.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <limits>
#include <vector>

namespace {

void appendByte(std::vector<unsigned char>& out, unsigned char value) {
    out.push_back(value);
}

void appendU32(std::vector<unsigned char>& out, std::uint32_t value) {
    appendByte(out, static_cast<unsigned char>((value >> 24) & 0xFF));
    appendByte(out, static_cast<unsigned char>((value >> 16) & 0xFF));
    appendByte(out, static_cast<unsigned char>((value >> 8) & 0xFF));
    appendByte(out, static_cast<unsigned char>(value & 0xFF));
}

std::uint32_t crc32(std::span<const unsigned char> data) {
    std::uint32_t crc = 0xFFFFFFFFu;
    for (const unsigned char byte : data) {
        crc ^= byte;
        for (int bit = 0; bit < 8; ++bit) {
            const std::uint32_t mask = -(crc & 1u);
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    return crc ^ 0xFFFFFFFFu;
}

std::uint32_t adler32(std::span<const unsigned char> data) {
    std::uint32_t s1 = 1;
    std::uint32_t s2 = 0;
    for (const unsigned char byte : data) {
        s1 = (s1 + byte) % 65521u;
        s2 = (s2 + s1) % 65521u;
    }
    return (s2 << 16) | s1;
}

void appendChunk(std::vector<unsigned char>& out, const char type[4], std::span<const unsigned char> data) {
    appendU32(out, static_cast<std::uint32_t>(data.size()));
    const std::size_t start = out.size();
    out.insert(out.end(), type, type + 4);
    out.insert(out.end(), data.begin(), data.end());
    appendU32(out, crc32(std::span<const unsigned char>(out.data() + start, out.size() - start)));
}

// zlib wrapper around raw deflate blocks. Each block is stored uncompressed, which
// is enough for a screenshot and keeps the file a normal PNG.
void appendZlibStored(std::vector<unsigned char>& out, std::span<const unsigned char> raw) {
    appendByte(out, 0x78);
    appendByte(out, 0x01);
    std::size_t offset = 0;
    do {
        const std::size_t count = std::min(raw.size() - offset, std::size_t{65535});
        const bool last = offset + count == raw.size();
        appendByte(out, last ? 0x01 : 0x00);
        const auto length = static_cast<std::uint16_t>(count);
        appendByte(out, static_cast<unsigned char>(length & 0xFF));
        appendByte(out, static_cast<unsigned char>((length >> 8) & 0xFF));
        const auto inverted = static_cast<std::uint16_t>(~length);
        appendByte(out, static_cast<unsigned char>(inverted & 0xFF));
        appendByte(out, static_cast<unsigned char>((inverted >> 8) & 0xFF));
        out.insert(out.end(), raw.begin() + static_cast<std::ptrdiff_t>(offset), raw.begin() + static_cast<std::ptrdiff_t>(offset + count));
        offset += count;
    } while (offset < raw.size());
    appendU32(out, adler32(raw));
}

}

bool writePng(const std::string& path, int width, int height, std::span<const unsigned char> rgb) {
    if (width <= 0 || height <= 0) return false;
    const auto columns = static_cast<std::size_t>(width);
    const auto rows = static_cast<std::size_t>(height);
    if (columns > std::numeric_limits<std::size_t>::max() / 3) return false;
    const std::size_t rowBytes = columns * 3;
    if (rows > std::numeric_limits<std::size_t>::max() / rowBytes) return false;
    if (rgb.size() != rows * rowBytes) return false;

    // Each scanline starts with filter 0 (the bytes are already the pixel values).
    std::vector<unsigned char> filtered(rows * (rowBytes + 1));
    for (std::size_t y = 0; y < rows; ++y) {
        filtered[y * (rowBytes + 1)] = 0;
        const auto src = rgb.begin() + static_cast<std::ptrdiff_t>(y * rowBytes);
        std::copy(src, src + static_cast<std::ptrdiff_t>(rowBytes), filtered.begin() + static_cast<std::ptrdiff_t>(y * (rowBytes + 1) + 1));
    }

    std::vector<unsigned char> file;
    const unsigned char signature[] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    file.insert(file.end(), signature, signature + sizeof(signature));

    unsigned char header[13] = {};
    header[0] = static_cast<unsigned char>((static_cast<std::uint32_t>(width) >> 24) & 0xFF);
    header[1] = static_cast<unsigned char>((static_cast<std::uint32_t>(width) >> 16) & 0xFF);
    header[2] = static_cast<unsigned char>((static_cast<std::uint32_t>(width) >> 8) & 0xFF);
    header[3] = static_cast<unsigned char>(static_cast<std::uint32_t>(width) & 0xFF);
    header[4] = static_cast<unsigned char>((static_cast<std::uint32_t>(height) >> 24) & 0xFF);
    header[5] = static_cast<unsigned char>((static_cast<std::uint32_t>(height) >> 16) & 0xFF);
    header[6] = static_cast<unsigned char>((static_cast<std::uint32_t>(height) >> 8) & 0xFF);
    header[7] = static_cast<unsigned char>(static_cast<std::uint32_t>(height) & 0xFF);
    header[8] = 8;
    header[9] = 2;
    appendChunk(file, "IHDR", header);

    std::vector<unsigned char> compressed;
    appendZlibStored(compressed, filtered);
    appendChunk(file, "IDAT", compressed);
    appendChunk(file, "IEND", {});

    std::ofstream output(path, std::ios::binary);
    if (!output) return false;
    output.write(reinterpret_cast<const char*>(file.data()), static_cast<std::streamsize>(file.size()));
    return output.good();
}
