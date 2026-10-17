#pragma once

#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

#include "../lib/camera/camera.hpp"

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
    glm::vec3 diffuseLightDirection = glm::vec3(-0.44f, -0.82f, -1.f);
    bool drawWireframe = false;
    bool backFaceCulling = false;

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
