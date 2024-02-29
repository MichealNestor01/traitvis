#pragma once

#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include "../lib/camera/camera.hpp"
#include "../lib/parsingData/datasetReader.hpp"
#include "../lib/multiField/multiField.hpp"
#include "../lib/marchingCubes/marchingCubes.hpp"

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
    float farPlane = 500.f;
    // frametime variables
    float deltaTime = 0.f;
    float lastFrame = 0.f;
    // dataset variables
    bool loadedDataset = false;
    MultiField dataset;
    // levelset variables
    std::vector<AttributeVertex> attributeSpaceVertices;
    int **grid;
    std::vector<float> levelSetVertices;
    unsigned int levelSetVAO = 0;
    bool levelSetGenerated = false;

    void addAttributeSpaceVertex(AttributeVertex vertex) {
        attributeSpaceVertices.push_back(vertex);
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

    void generateLevelSet(float euclidianDistance) {
        // identify vertices which map to the level set
        // geneerateLevelset...
        
        // render the levelset 
        // clear the grid
        for (int i = 0; i < dataset.xVals * dataset.yVals * dataset.zVals; ++i) grid[i][3] = grid[i][2] == 200;
        Triangles surface = extractTriangles(grid, {dataset.xVals, dataset.yVals, dataset.zVals});
        levelSetVertices = surface.getVertices();
        levelSetGenerated = true;
    }
};