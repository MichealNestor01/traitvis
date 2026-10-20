#include "pointLight.hpp"

#include <gtest/gtest.h>

#include <glm.hpp>

TEST(LightFrustumTests, CloserToTheLightIsSmallerDepth) {
    const LightFrustum frustum = fitPointLight(glm::vec3(2, 2, 10), glm::vec3(0), glm::vec3(4));
    const float nearLight = lightDepth(frustum, glm::vec3(2, 2, 4));
    const float farAlongRay = lightDepth(frustum, glm::vec3(2, 2, 0));
    EXPECT_LT(nearLight, farAlongRay);
}

TEST(LightFrustumTests, AabbCornersLieInsideTheBox) {
    const LightFrustum frustum = fitPointLight(glm::vec3(2, 2, 10), glm::vec3(0), glm::vec3(4));
    for (float x : {0.f, 4.f}) {
        for (float y : {0.f, 4.f}) {
            for (float z : {0.f, 4.f}) {
                const glm::vec4 clip = frustum.projection * frustum.view * glm::vec4(x, y, z, 1.f);
                const glm::vec3 ndc = glm::vec3(clip) / clip.w;
                EXPECT_GE(ndc.x, -1.f);
                EXPECT_LE(ndc.x, 1.f);
                EXPECT_GE(ndc.y, -1.f);
                EXPECT_LE(ndc.y, 1.f);
                EXPECT_GE(ndc.z, -1.f);
                EXPECT_LE(ndc.z, 1.f);
            }
        }
    }
}

TEST(TransmittanceTests, LayersInFrontMultiplyAndLayersBehindDoNot) {
    const LightLayer layers[] = {{0.2f, 0.5f}, {0.5f, 0.5f}};
    EXPECT_FLOAT_EQ(transmittance(layers, 0.1f, 1.f, 1e-4f), 1.f);
    EXPECT_FLOAT_EQ(transmittance(layers, 0.35f, 1.f, 1e-4f), 0.5f);
    EXPECT_FLOAT_EQ(transmittance(layers, 0.8f, 1.f, 1e-4f), 0.25f);
    EXPECT_FLOAT_EQ(transmittance(layers, 0.2f, 1.f, 1e-4f), 1.f);
}

TEST(TransmittanceTests, OpaqueInFrontBlocks) {
    EXPECT_FLOAT_EQ(transmittance({}, 0.6f, 0.4f, 1e-4f), 0.f);
}
