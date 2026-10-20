#pragma once

#include "../marchingCubes/marchingCubes.hpp"

#include <filesystem>
#include <iosfwd>
#include <string>

// A triangle mesh read from OBJ. error is empty when the file parsed.
// An imported mesh has positions and normals only: the trait and the dataset are not in the file.
struct ObjMesh {
    Surface surface;
    std::string error;
    [[nodiscard]] bool ok() const { return error.empty(); }
};

void writeOBJ(const Surface& surface, std::ostream& out);
[[nodiscard]] bool exportOBJ(const Surface& surface, const std::filesystem::path& file);
[[nodiscard]] ObjMesh readOBJ(std::istream& in);
[[nodiscard]] ObjMesh importOBJ(const std::filesystem::path& file);
