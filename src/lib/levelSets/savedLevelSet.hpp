#pragma once

#include "featureLevelSet.hpp"

#include <iosfwd>
#include <span>
#include <string>

// A level set saved with the distance field that built it. The surface is not stored:
// opening the file extracts it again, and the distance slider can extract another.
// The bytes are little-endian. Magic "TVLS", then version 1, the name, colour,
// transparency, isovalue, the close-at-missing-data flag, the grid size, and the samples.
struct SavedLevelSet {
    std::string id;
    glm::vec3 colour{1.f, 0.f, 0.f};
    float transparency = 1.f;
    float isoValue = 0.f;
    bool closeAtMissingData = true;
    ScalarField field;
    std::string error;

    [[nodiscard]] bool ok() const { return error.empty(); }
};

// False when no distance field was kept, or the stream cannot take the bytes.
[[nodiscard]] bool writeSavedLevelSet(const FeatureLevelSet& levelSet, std::ostream& out);
[[nodiscard]] SavedLevelSet readSavedLevelSet(std::span<const unsigned char> bytes);
