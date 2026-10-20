#pragma once

#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

#include "../lib/camera/camera.hpp"

// Front and back of eight shells. The slider cannot ask for more colour textures than this.
constexpr int kMaxDepthPeelCap = 16;

// Camera, window, and lighting. The datasets live in SessionState.
struct RenderState {
    Camera cam;
    int windowWidth, windowHeight;
    float renderWidthPercentage = 1.f;
    bool enableCam = false;
    float fov = 45.f;
    float aspectRatio = (renderWidthPercentage*windowWidth)/windowHeight;
    float nearPlane = 0.1f;
    float farPlane = 3000.f;
    float deltaTime = 0.f;
    float lastFrame = 0.f;
    glm::vec3 ambientLightColour = glm::vec3(1.f, 1.f, 1.f);
    float ambientLightStrength = 0.28f;
    glm::vec3 diffuseLightColour = glm::vec3(1.f, 1.f, 1.f);
    float diffuseLightStrength = 0.58f;
    // One direction for the whole frame. This is the cheap light; the point light is opt-in.
    glm::vec3 diffuseLightDirection = glm::vec3(-0.44f, -0.82f, -1.f);
    bool pointLight = false;
    // Off the Isabel grid (0..500, 0..100, 0..500), on the side the old direction pointed from.
    glm::vec3 lightPosition = glm::vec3(200.f, -400.f, -200.f);
    float lightSize = 20.f;
    float lightRange = 2000.f;
    bool shadowsEnabled = false;
    float shadowFloor = 0.35f;
    bool drawWireframe = false;
    bool backFaceCulling = false;
    // How many transparent layers to peel. The loop still stops when a layer is empty.
    int maxDepthPeels = kMaxDepthPeelCap;
    // One clip plane in grid coordinates. Axis 0, 1, 2 is X, Y, Z.
    // The fragment on the positive side of clipPosition is dropped.
    bool clipEnabled = false;
    int clipAxis = 1;
    float clipPosition = 0.f;

    void toggleCam(GLFWwindow* window) {
        enableCam = not enableCam;
        cam.disableActions();
        if (enableCam) glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        else glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    void updateTime() {
        float now = glfwGetTime();
        deltaTime = now - lastFrame;
        lastFrame = now;
    }

    void setFov(double yoffset) {
        fov -= yoffset;
        if (fov < 1.f) fov = 1.f;
        else if (fov > 90.f) fov = 90.f;
    }

    void setWidthHeight(int width, int height) {
        windowWidth = width;
        windowHeight = height;
        aspectRatio = renderWidthPercentage*(float)width/(float)height;
    }

    void setAspect(double aspect) {
        aspectRatio = aspect;
    }

    glm::mat4 getProjectionMatrix() {
        return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
    }
};
