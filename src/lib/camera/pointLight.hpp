#pragma once

#include <glm.hpp>

#include <span>

// One perspective camera at a point light, aimed at the centre of an axis-aligned box.
struct LightFrustum {
    glm::mat4 view;
    glm::mat4 projection;
};

// One peeled layer, nearest to the light first. Depth is the light-space window depth.
struct LightLayer {
    float depth;
    float alpha;
};

// Field of view is the smallest that contains every corner, and never wider than 120°.
// A light sitting inside the box cannot cover the corners behind it with one frustum.
[[nodiscard]] LightFrustum fitPointLight(glm::vec3 position, glm::vec3 minCorner, glm::vec3 maxCorner);

// Window depth after fitPointLight's view-projection. Closer to the light is smaller.
[[nodiscard]] float lightDepth(const LightFrustum& frustum, glm::vec3 world);

// Product of (1 - alpha) for every layer strictly in front of the fragment.
// A fragment behind the opaque surface is fully blocked. The bias stops a surface
// from shadowing itself.
[[nodiscard]] float transmittance(std::span<const LightLayer> layers, float fragmentDepth,
                                  float opaqueDepth, float bias);
