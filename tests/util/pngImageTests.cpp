#include "pngImage.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <vector>

namespace {

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

std::uint32_t readU32(std::span<const unsigned char> bytes, std::size_t offset) {
    return (std::uint32_t{bytes[offset]} << 24) | (std::uint32_t{bytes[offset + 1]} << 16)
        | (std::uint32_t{bytes[offset + 2]} << 8) | std::uint32_t{bytes[offset + 3]};
}

std::vector<unsigned char> readFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return std::vector<unsigned char>(std::istreambuf_iterator<char>(input), {});
}

std::vector<unsigned char> inflateStored(std::span<const unsigned char> zlib) {
    if (zlib.size() < 6 || zlib[0] != 0x78 || zlib[1] != 0x01) {
        ADD_FAILURE() << "zlib header";
        return {};
    }
    std::vector<unsigned char> raw;
    std::size_t cursor = 2;
    bool last = false;
    while (!last) {
        if (cursor + 5 > zlib.size()) {
            ADD_FAILURE() << "truncated block";
            return {};
        }
        const unsigned char header = zlib[cursor++];
        last = (header & 0x01) != 0;
        if ((header & 0x06) != 0) {
            ADD_FAILURE() << "block is not stored";
            return {};
        }
        const auto length = static_cast<std::uint16_t>(zlib[cursor] | (zlib[cursor + 1] << 8));
        const auto inverted = static_cast<std::uint16_t>(zlib[cursor + 2] | (zlib[cursor + 3] << 8));
        if (inverted != static_cast<std::uint16_t>(~length)) {
            ADD_FAILURE() << "block length";
            return {};
        }
        cursor += 4;
        if (cursor + length > zlib.size()) {
            ADD_FAILURE() << "truncated payload";
            return {};
        }
        raw.insert(raw.end(), zlib.begin() + static_cast<std::ptrdiff_t>(cursor), zlib.begin() + static_cast<std::ptrdiff_t>(cursor + length));
        cursor += length;
    }
    if (cursor + 4 != zlib.size() || readU32(zlib, cursor) != adler32(raw)) {
        ADD_FAILURE() << "adler32";
        return {};
    }
    return raw;
}

std::vector<unsigned char> pixelsOf(const std::filesystem::path& path, int& width, int& height) {
    const std::vector<unsigned char> file = readFile(path);
    const unsigned char signature[] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    if (file.size() < sizeof(signature) || !std::equal(signature, signature + sizeof(signature), file.begin())) {
        ADD_FAILURE() << "signature";
        return {};
    }

    std::vector<unsigned char> compressed;
    bool sawHeader = false;
    bool sawEnd = false;
    std::size_t cursor = sizeof(signature);
    while (cursor + 12 <= file.size()) {
        const std::uint32_t length = readU32(file, cursor);
        const std::size_t type = cursor + 4;
        if (type + 4 + length + 4 > file.size()) {
            ADD_FAILURE() << "truncated chunk";
            return {};
        }
        const auto chunk = std::span<const unsigned char>(file.data() + type, 4 + length);
        if (readU32(file, type + 4 + length) != crc32(chunk)) {
            ADD_FAILURE() << "chunk crc";
            return {};
        }
        const std::string name(reinterpret_cast<const char*>(file.data() + type), 4);
        const auto data = std::span<const unsigned char>(file.data() + type + 4, length);
        if (name == "IHDR") {
            sawHeader = true;
            if (length != 13) {
                ADD_FAILURE() << "header length";
                return {};
            }
            width = static_cast<int>(readU32(data, 0));
            height = static_cast<int>(readU32(data, 4));
            EXPECT_EQ(data[8], 8);
            EXPECT_EQ(data[9], 2);
            EXPECT_EQ(data[10], 0);
            EXPECT_EQ(data[11], 0);
            EXPECT_EQ(data[12], 0);
        } else if (name == "IDAT") {
            compressed.insert(compressed.end(), data.begin(), data.end());
        } else if (name == "IEND") {
            sawEnd = true;
            EXPECT_EQ(length, 0u);
        }
        cursor = type + 4 + length + 4;
    }
    if (!sawHeader || !sawEnd) {
        ADD_FAILURE() << "missing chunk";
        return {};
    }
    const std::vector<unsigned char> filtered = inflateStored(compressed);
    std::vector<unsigned char> rgb;
    const std::size_t rowBytes = static_cast<std::size_t>(width) * 3;
    if (filtered.size() != static_cast<std::size_t>(height) * (rowBytes + 1)) {
        ADD_FAILURE() << "filtered size";
        return {};
    }
    for (int y = 0; y < height; ++y) {
        const std::size_t row = static_cast<std::size_t>(y) * (rowBytes + 1);
        EXPECT_EQ(filtered[row], 0);
        rgb.insert(rgb.end(), filtered.begin() + static_cast<std::ptrdiff_t>(row + 1), filtered.begin() + static_cast<std::ptrdiff_t>(row + 1 + rowBytes));
    }
    return rgb;
}

}

TEST(PngImageTests, ChecksumsMatchTheKnownAnswers) {
    const unsigned char text[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    EXPECT_EQ(crc32(text), 0xCBF43926u);
    EXPECT_EQ(adler32(text), 0x091E01DEu);
}

TEST(PngImageTests, RejectsASizeThatDoesNotMatchTheBytes) {
    const unsigned char rgb[] = {1, 2, 3, 4, 5, 6};
    EXPECT_FALSE(writePng("unused.png", 0, 1, rgb));
    EXPECT_FALSE(writePng("unused.png", 2, 2, rgb));
}

TEST(PngImageTests, RoundTripsTheTopRowFirst) {
    const unsigned char rgb[] = {
        255, 0, 0,  0, 255, 0,
        0, 0, 255,  255, 255, 255,
    };
    const auto path = std::filesystem::temp_directory_path() / "traitvis-png-two-by-two.png";
    ASSERT_TRUE(writePng(path.string(), 2, 2, rgb));
    int width = 0;
    int height = 0;
    const std::vector<unsigned char> read = pixelsOf(path, width, height);
    std::filesystem::remove(path);
    EXPECT_EQ(width, 2);
    EXPECT_EQ(height, 2);
    ASSERT_EQ(read.size(), sizeof(rgb));
    EXPECT_TRUE(std::equal(rgb, rgb + sizeof(rgb), read.begin()));
}

TEST(PngImageTests, RoundTripsAnImageThatNeedsTwoBlocks) {
    constexpr int width = 200;
    constexpr int height = 120;
    std::vector<unsigned char> rgb(static_cast<std::size_t>(width) * height * 3);
    for (std::size_t i = 0; i < rgb.size(); ++i) rgb[i] = static_cast<unsigned char>(i % 251);
    const auto path = std::filesystem::temp_directory_path() / "traitvis-png-two-blocks.png";
    ASSERT_TRUE(writePng(path.string(), width, height, rgb));
    int readWidth = 0;
    int readHeight = 0;
    const std::vector<unsigned char> read = pixelsOf(path, readWidth, readHeight);
    std::filesystem::remove(path);
    EXPECT_EQ(readWidth, width);
    EXPECT_EQ(readHeight, height);
    EXPECT_EQ(read, rgb);
}
