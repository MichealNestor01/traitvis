#pragma once

#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <set>
#include <vector>

#include "../lib/camera/camera.hpp"
#include "../lib/parsingData/datasetReader.hpp"
#include "../lib/multiField/multiField.hpp"
#include "../lib/marchingCubes/marchingCubes.hpp"
#include "../lib/levelSets/featureLevelSet.hpp"

struct ProgramState {
    // camera object
    Camera cam;
    // window state
    int windowWidth, windowHeight;
    float renderWidthPercentage = 0.8f;
    // control toggling
    bool enableCam = false;
    // projection matrix variables
    float fov = 45.f;
    float aspectRatio = (renderWidthPercentage*windowWidth)/windowHeight;
    float nearPlane = 0.1f;
    float farPlane = 3000.f;
    // frametime variables
    float deltaTime = 0.f;
    float lastFrame = 0.f;
    // lighting controls
    glm::vec3 ambientLightColour = glm::vec3(1.f, 1.f, 1.f);
    float ambientLightStrength = 0.28f;
    glm::vec3 diffuseLightColour = glm::vec3(1.f, 1.f, 1.f); 
    float diffuseLightStrength = 0.58f;
    glm::vec3 diffuseLightDirection = glm::vec3(-0.44f, -0.82f, -1.f);
    // other rendering parameters
    bool drawWireframe = false;
    bool backFaceCulling = false;
    // dataset variables
    bool loadedDataset = false;
    MultiField dataset;
    // levelset variables
    std::vector<TraitPoint> attributeSpaceVerticesBuffer;
    std::vector<FeatureLevelSet> levelSets;


    void generateLevelSet(float euclideanDistance, glm::vec3 colour, std::string id) {
        FeatureLevelSet newLevelSet(attributeSpaceVerticesBuffer, dataset, euclideanDistance, colour, id);
        levelSets.push_back(newLevelSet);
    }

    void addAttributeSpaceVertexToBuffer(TraitPoint vertex) {
        attributeSpaceVerticesBuffer.push_back(vertex);
    }

    void loadDataset(std::string path) {
        dataset = readDataset(path);
        loadedDataset = true;
    }

    void toggleCam(GLFWwindow* window) {
        enableCam = not enableCam;
        if (enableCam) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            glfwSetCursorPos(window, windowWidth/2, windowHeight/2);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    void toggleDrawWireframe() {
        if (drawWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        else glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        drawWireframe = not drawWireframe;
    }

    void toggleBackFaceCulling() {
        if (backFaceCulling) glEnable(GL_CULL_FACE);
        else glDisable(GL_CULL_FACE);
        backFaceCulling = not backFaceCulling;
    }

    void updateTime() {
        float now = glfwGetTime();
        deltaTime = now - lastFrame;
        lastFrame = now;
    }

    void setFov(double yoffset) {
        fov -= yoffset;
        if (fov < 1.f) fov = 1.f;
        else if (fov > 70.f) fov = 70.f;
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