#include "pointLight.hpp"

#include <gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

void fillCorners(glm::vec3 minCorner, glm::vec3 maxCorner, glm::vec3 (&corners)[8]) {
    const float xs[2] = {minCorner.x, maxCorner.x};
    const float ys[2] = {minCorner.y, maxCorner.y};
    const float zs[2] = {minCorner.z, maxCorner.z};
    int index = 0;
    for (float z : zs)
        for (float y : ys)
            for (float x : xs)
                corners[index++] = glm::vec3(x, y, z);
}

}

LightFrustum fitPointLight(glm::vec3 position, glm::vec3 minCorner, glm::vec3 maxCorner) {
    const glm::vec3 centre = (minCorner + maxCorner) * 0.5f;
    glm::vec3 forward = centre - position;
    if (glm::dot(forward, forward) < 1e-8f) forward = glm::vec3(0.f, 0.f, -1.f);
    else forward = glm::normalize(forward);
    const glm::vec3 up = std::fabs(forward.y) > 0.99f ? glm::vec3(1.f, 0.f, 0.f) : glm::vec3(0.f, 1.f, 0.f);
    const glm::mat4 view = glm::lookAt(position, position + forward, up);

    glm::vec3 corners[8];
    fillCorners(minCorner, maxCorner, corners);

    float closest = std::numeric_limits<float>::infinity();
    float farthest = 0.f;
    float maxTan = 0.f;
    bool everyCornerInFront = true;
    for (const glm::vec3& corner : corners) {
        const float distance = glm::length(corner - position);
        closest = std::min(closest, distance);
        farthest = std::max(farthest, distance);
        const glm::vec4 viewed = view * glm::vec4(corner, 1.f);
        const float ahead = -viewed.z;
        if (ahead <= 1e-4f) {
            everyCornerInFront = false;
            continue;
        }
        const float tanHalf = std::max(std::fabs(viewed.x), std::fabs(viewed.y)) / ahead;
        maxTan = std::max(maxTan, tanHalf);
    }

    // A small pad keeps a corner that lands on the clip edge inside the box.
    constexpr float kMaxFovDegrees = 120.f;
    float fovDegrees = kMaxFovDegrees;
    if (everyCornerInFront) {
        fovDegrees = glm::degrees(2.f * std::atan(maxTan * 1.01f));
        if (!(fovDegrees > 1.f)) fovDegrees = 1.f;
        if (fovDegrees > kMaxFovDegrees) fovDegrees = kMaxFovDegrees;
    }

    if (!std::isfinite(closest)) closest = 1.f;
    // Near is a fraction of the distance to the closest corner, so every corner is beyond it.
    const float nearPlane = std::max(closest * 0.1f, 0.05f);
    const float farPlane = std::max(farthest * 1.01f, nearPlane + 1.f);
    return {view, glm::perspective(glm::radians(fovDegrees), 1.f, nearPlane, farPlane)};
}

float lightDepth(const LightFrustum& frustum, glm::vec3 world) {
    const glm::vec4 clip = frustum.projection * frustum.view * glm::vec4(world, 1.f);
    const float ndcZ = clip.z / clip.w;
    return ndcZ * 0.5f + 0.5f;
}

float transmittance(std::span<const LightLayer> layers, float fragmentDepth, float opaqueDepth, float bias) {
    if (fragmentDepth > opaqueDepth + bias) return 0.f;
    float passing = 1.f;
    for (const LightLayer& layer : layers) {
        if (layer.depth + bias >= fragmentDepth) break;
        passing *= (1.f - layer.alpha);
    }
    return passing;
}
