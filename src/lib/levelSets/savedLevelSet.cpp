#include "savedLevelSet.hpp"

#include <cstdint>
#include <cstring>
#include <limits>
#include <ostream>

namespace {

constexpr char kMagic[4] = {'T', 'V', 'L', 'S'};
constexpr std::uint32_t kVersion = 1;
constexpr std::uint32_t kCloseAtMissingData = 1u;
constexpr std::uint32_t kMaxNameBytes = 1024;

void write32(std::ostream& out, std::uint32_t value) {
    const unsigned char bytes[4] = {
        static_cast<unsigned char>(value & 0xffu),
        static_cast<unsigned char>((value >> 8) & 0xffu),
        static_cast<unsigned char>((value >> 16) & 0xffu),
        static_cast<unsigned char>((value >> 24) & 0xffu),
    };
    out.write(reinterpret_cast<const char*>(bytes), 4);
}

void writeFloat(std::ostream& out, float value) {
    static_assert(sizeof(float) == 4);
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, 4);
    write32(out, bits);
}

bool read32(std::span<const unsigned char>& in, std::uint32_t& value) {
    if (in.size() < 4) return false;
    value = std::uint32_t(in[0])
        | (std::uint32_t(in[1]) << 8)
        | (std::uint32_t(in[2]) << 16)
        | (std::uint32_t(in[3]) << 24);
    in = in.subspan(4);
    return true;
}

bool readFloat(std::span<const unsigned char>& in, float& value) {
    std::uint32_t bits = 0;
    if (!read32(in, bits)) return false;
    std::memcpy(&value, &bits, 4);
    return true;
}

SavedLevelSet failed(std::string error) {
    SavedLevelSet saved;
    saved.error = std::move(error);
    return saved;
}

bool gridCount(int x, int y, int z, int& total) {
    if (x <= 0 || y <= 0 || z <= 0) return false;
    const std::int64_t count = std::int64_t(x) * y * z;
    if (count > std::numeric_limits<int>::max()) return false;
    total = static_cast<int>(count);
    return true;
}

}

bool writeSavedLevelSet(const FeatureLevelSet& levelSet, std::ostream& out) {
    if (!levelSet.distanceField) return false;
    const ScalarField& field = *levelSet.distanceField;
    int total = 0;
    if (!gridCount(field.layout.x, field.layout.y, field.layout.z, total)) return false;
    if (static_cast<int>(field.values.size()) != total) return false;
    if (levelSet.id.size() > kMaxNameBytes) return false;

    out.write(kMagic, 4);
    write32(out, kVersion);
    write32(out, static_cast<std::uint32_t>(levelSet.id.size()));
    out.write(levelSet.id.data(), static_cast<std::streamsize>(levelSet.id.size()));
    writeFloat(out, levelSet.colour.x);
    writeFloat(out, levelSet.colour.y);
    writeFloat(out, levelSet.colour.z);
    writeFloat(out, levelSet.transparency);
    writeFloat(out, levelSet.isoValue);
    write32(out, levelSet.closeAtMissingData ? kCloseAtMissingData : 0u);
    write32(out, static_cast<std::uint32_t>(field.layout.x));
    write32(out, static_cast<std::uint32_t>(field.layout.y));
    write32(out, static_cast<std::uint32_t>(field.layout.z));
    for (const float sample : field.values) writeFloat(out, sample);
    return static_cast<bool>(out);
}

SavedLevelSet readSavedLevelSet(std::span<const unsigned char> bytes) {
    if (bytes.size() < 4 || std::memcmp(bytes.data(), kMagic, 4) != 0)
        return failed("That file is not a TraitVis level set.");
    bytes = bytes.subspan(4);

    std::uint32_t version = 0;
    if (!read32(bytes, version)) return failed("That level set file is not valid.");
    if (version != kVersion) return failed("That file is from a newer TraitVis.");

    std::uint32_t nameBytes = 0;
    if (!read32(bytes, nameBytes) || nameBytes > kMaxNameBytes || bytes.size() < nameBytes)
        return failed("That level set file is not valid.");
    SavedLevelSet saved;
    saved.id.assign(reinterpret_cast<const char*>(bytes.data()), nameBytes);
    bytes = bytes.subspan(nameBytes);

    if (!readFloat(bytes, saved.colour.x) || !readFloat(bytes, saved.colour.y) || !readFloat(bytes, saved.colour.z))
        return failed("That level set file is not valid.");
    if (!readFloat(bytes, saved.transparency) || !readFloat(bytes, saved.isoValue))
        return failed("That level set file is not valid.");

    std::uint32_t flags = 0;
    std::uint32_t x = 0;
    std::uint32_t y = 0;
    std::uint32_t z = 0;
    if (!read32(bytes, flags) || !read32(bytes, x) || !read32(bytes, y) || !read32(bytes, z))
        return failed("That level set file is not valid.");
    saved.closeAtMissingData = (flags & kCloseAtMissingData) != 0;

    int total = 0;
    if (x > static_cast<std::uint32_t>(std::numeric_limits<int>::max())
        || y > static_cast<std::uint32_t>(std::numeric_limits<int>::max())
        || z > static_cast<std::uint32_t>(std::numeric_limits<int>::max())
        || !gridCount(static_cast<int>(x), static_cast<int>(y), static_cast<int>(z), total))
        return failed("That level set file is not valid.");

    const std::size_t sampleBytes = static_cast<std::size_t>(total) * sizeof(float);
    if (bytes.size() != sampleBytes) return failed("That level set file is not valid.");

    saved.field.layout = {static_cast<int>(x), static_cast<int>(y), static_cast<int>(z)};
    saved.field.values.resize(static_cast<std::size_t>(total));
    for (float& sample : saved.field.values) {
        if (!readFloat(bytes, sample)) return failed("That level set file is not valid.");
    }
    return saved;
}
