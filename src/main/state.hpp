#pragma once

#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <memory>
#include <optional>
#include <set>
#include <utility>
#include <vector>

#include "../lib/camera/camera.hpp"
#include "../lib/parsingData/datasetReader.hpp"
#include "../lib/multiField/multiField.hpp"
#include "../lib/marchingCubes/marchingCubes.hpp"
#include "../lib/levelSets/datasetSession.hpp"
#include "../lib/levelSets/levelSetJob.hpp"

struct ProgramState {
    // camera object
    Camera cam;
    // window state
    int windowWidth, windowHeight;
    float renderWidthPercentage = 1.f;
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
    DatasetSession session;
    std::optional<DatasetReadJob> activeLoad;
    std::optional<LevelSetJob> activeLevelSet;


    void generateLevelSet(float euclideanDistance, glm::vec3 colour, std::string id) {
        const std::shared_ptr<MultiField> field = session.activeDataset();
        if (activeLevelSet || !field || !field->ok()) return;
        activeLevelSet.emplace(session.attributeSpacePointsBuffer, std::shared_ptr<const MultiField>(field), euclideanDistance, colour, std::move(id));
        activeLevelSet->start();
    }

    void addAttributeSpacePointToBuffer(TraitPoint point) {
        session.attributeSpacePointsBuffer.push_back(point);
    }

    void beginDatasetLoad(std::string path) {
        if (activeLoad) return;
        activeLoad.emplace(std::move(path));
        activeLoad->start();
    }

    // Steps a load that has no worker, then adds a finished dataset and makes it active.
    // Returns true when the active dataset changed, so the interface can rebuild its sliders.
    // A cancel installs nothing.
    bool pollDatasetLoad() {
#if defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)
        if (activeLoad && !activeLoad->runsOnWorker() && !activeLoad->finished())
            activeLoad->stepOnMainThread();
#endif
        if (!activeLoad || !activeLoad->finished()) return false;
        std::optional<MultiField> loaded = activeLoad->take();
        activeLoad.reset();
        if (!loaded) return false;
        session.addDataset(std::make_shared<MultiField>(std::move(*loaded)));
        return true;
    }

    void pollLevelSet() {
#if defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)
        if (activeLevelSet && !activeLevelSet->runsOnWorker() && !activeLevelSet->finished())
            activeLevelSet->stepOnMainThread(1);
#endif
        if (!activeLevelSet || !activeLevelSet->finished()) return;
        if (std::optional<FeatureLevelSet> made = activeLevelSet->take())
            session.levelSets.push_back(std::move(*made));
        activeLevelSet.reset();
    }

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