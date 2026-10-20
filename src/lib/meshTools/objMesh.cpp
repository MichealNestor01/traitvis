#include "objMesh.hpp"

#include <algorithm>
#include <charconv>
#include <fstream>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
#include <string_view>
#include <utility>

namespace {

[[nodiscard]] std::optional<float> parseFloat(std::string_view text) {
    float value = 0.f;
    const char* const end = text.data() + text.size();
    const std::from_chars_result parsed = std::from_chars(text.data(), end, value);
    if (parsed.ec != std::errc{} || parsed.ptr != end) return std::nullopt;
    return value;
}

[[nodiscard]] std::optional<int> parseInt(std::string_view text) {
    int value = 0;
    const char* const end = text.data() + text.size();
    const std::from_chars_result parsed = std::from_chars(text.data(), end, value);
    if (parsed.ec != std::errc{} || parsed.ptr != end) return std::nullopt;
    return value;
}

// OBJ indices are 1-based. A negative index counts back from the most recent element.
[[nodiscard]] std::optional<int> resolveIndex(int index, int count) {
    if (index > 0 && index <= count) return index - 1;
    if (index < 0 && count + index >= 0) return count + index;
    return std::nullopt;
}

[[nodiscard]] glm::vec3 polygonNormal(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
    const glm::vec3 crossed = glm::cross(b - a, c - a);
    const float length = glm::length(crossed);
    if (length == 0.f) return {0.f, 0.f, 1.f};
    return crossed / length;
}

struct Corner {
    int vertex = 0;
    std::optional<int> normal;
};

[[nodiscard]] std::optional<Corner> parseCorner(std::string_view token, int vertexCount, int normalCount, std::string& error) {
    if (std::count(token.begin(), token.end(), '/') > 2) {
        error = "OBJ face corner has too many indices.";
        return std::nullopt;
    }
    std::string_view parts[3];
    int partCount = 0;
    std::size_t start = 0;
    while (partCount < 3) {
        const std::size_t slash = token.find('/', start);
        parts[partCount++] = token.substr(start, slash == std::string_view::npos ? std::string_view::npos : slash - start);
        if (slash == std::string_view::npos) break;
        start = slash + 1;
    }
    if (parts[0].empty()) {
        error = "OBJ face is missing a vertex index.";
        return std::nullopt;
    }
    const std::optional<int> vertex = parseInt(parts[0]);
    const std::optional<int> resolvedVertex = vertex ? resolveIndex(*vertex, vertexCount) : std::nullopt;
    if (!resolvedVertex) {
        error = "OBJ face index is past the end of the vertices.";
        return std::nullopt;
    }
    Corner corner{*resolvedVertex, std::nullopt};
    if (partCount == 3 && !parts[2].empty()) {
        const std::optional<int> normal = parseInt(parts[2]);
        const std::optional<int> resolvedNormal = normal ? resolveIndex(*normal, normalCount) : std::nullopt;
        if (!resolvedNormal) {
            error = "OBJ face index is past the end of the normals.";
            return std::nullopt;
        }
        corner.normal = *resolvedNormal;
    }
    return corner;
}

[[nodiscard]] ObjMesh failed(std::string error) {
    return {{}, std::move(error)};
}

}

void writeOBJ(const Surface& surface, std::ostream& out) {
    out << std::setprecision(std::numeric_limits<float>::max_digits10);
    for (const glm::vec3& vertex : surface.vertices)
        out << "v " << vertex.x << ' ' << vertex.y << ' ' << vertex.z << '\n';
    for (const glm::vec3& normal : surface.normals)
        out << "vn " << normal.x << ' ' << normal.y << ' ' << normal.z << '\n';

    const std::size_t paired = std::min(surface.vertices.size(), surface.normals.size());
    for (std::size_t index = 0; index + 2 < paired; index += 3) {
        out << "f";
        for (int corner = 0; corner < 3; ++corner) {
            const std::size_t faceIndex = index + static_cast<std::size_t>(corner) + 1;
            out << ' ' << faceIndex << "//" << faceIndex;
        }
        out << '\n';
    }
}

bool exportOBJ(const Surface& surface, const std::filesystem::path& file) {
    std::ofstream out(file);
    if (!out) return false;
    writeOBJ(surface, out);
    return static_cast<bool>(out);
}

ObjMesh readOBJ(std::istream& in) {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<std::vector<Corner>> faces;

    std::string text;
    while (std::getline(in, text)) {
        if (!text.empty() && text.back() == '\r') text.pop_back();
        std::istringstream line(text);
        std::string word;
        if (!(line >> word) || word[0] == '#') continue;
        if (word == "v" || word == "vn" || word == "vt") {
            std::string xs, ys, zs;
            if (!(line >> xs)) return failed("OBJ value is not a number.");
            const std::optional<float> x = parseFloat(xs);
            if (!x) return failed("OBJ value is not a number.");
            if (word == "vt") continue;
            if (!(line >> ys >> zs)) return failed("OBJ value is not three numbers.");
            const std::optional<float> y = parseFloat(ys);
            const std::optional<float> z = parseFloat(zs);
            if (!y || !z) return failed("OBJ value is not a number.");
            (word == "v" ? positions : normals).push_back({*x, *y, *z});
            continue;
        }
        if (word != "f") continue;

        std::vector<Corner> corners;
        std::string token;
        while (line >> token) {
            if (token[0] == '#') break;
            std::optional<Corner> corner = parseCorner(token, static_cast<int>(positions.size()), static_cast<int>(normals.size()), text);
            if (!corner) return failed(std::move(text));
            corners.push_back(*corner);
        }
        if (corners.size() < 3) return failed("OBJ face has fewer than three corners.");
        faces.push_back(std::move(corners));
    }

    ObjMesh mesh;
    for (const std::vector<Corner>& face : faces) {
        const glm::vec3 faceNormal = polygonNormal(
            positions[face[0].vertex], positions[face[1].vertex], positions[face[2].vertex]);
        for (std::size_t triangle = 1; triangle + 1 < face.size(); ++triangle) {
            const Corner fan[3] = {face[0], face[triangle], face[triangle + 1]};
            for (const Corner& corner : fan) {
                mesh.surface.vertices.push_back(positions[corner.vertex]);
                mesh.surface.normals.push_back(corner.normal ? normals[*corner.normal] : faceNormal);
            }
        }
    }
    return mesh;
}

ObjMesh importOBJ(const std::filesystem::path& file) {
    std::ifstream in(file);
    if (!in) return failed("Failed to read " + file.string());
    return readOBJ(in);
}
