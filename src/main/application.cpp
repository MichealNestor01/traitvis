#include "application.hpp"

#include <GLFW/glfw3.h>
#include <glm.hpp>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <iostream>
#include <span>
#include <stdexcept>

#include "callbacks.hpp"
#include "inputProcessing.hpp"
#include "../lib/meshTools/cube.hpp"

namespace {

void bindDepthTexture(unsigned int texture) {
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
}

void drawLevelSet(ShaderProgram& shader, FeatureLevelSet& levelSet, const GpuMesh& cubeMesh, const glm::vec3& activeColour, const glm::vec3& inactiveColour) {
    if (!levelSet.mesh) levelSet.mesh = uploadMesh(levelSet.surface);
    const GpuMesh& mesh = *levelSet.mesh;
    shader.setUniformb("invertNormal", levelSet.invertNormals);
    shader.setUniform3f("objectColor", levelSet.colour);
    shader.setUniformf("objectTransparency", levelSet.transparency);
    shader.setUniformb("useInstanceModel", false);
    glBindVertexArray(mesh.vao.get());
    glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);

    if (!levelSet.showActiveInactivePixels) return;
    shader.setUniformb("useInstanceModel", true);
    glBindVertexArray(cubeMesh.vao.get());
    if (mesh.activeInstances) {
        shader.setUniform3f("objectColor", activeColour);
        bindInstanceBuffer(mesh.activeInstances.get());
        glDrawArraysInstanced(GL_TRIANGLES, 0, cubeMesh.vertexCount, static_cast<GLsizei>(levelSet.surface.activeVertices.size()));
    }
    if (mesh.inactiveInstances) {
        shader.setUniform3f("objectColor", inactiveColour);
        bindInstanceBuffer(mesh.inactiveInstances.get());
        glDrawArraysInstanced(GL_TRIANGLES, 0, cubeMesh.vertexCount, static_cast<GLsizei>(levelSet.surface.inactiveVertices.size()));
    }
    shader.setUniformb("useInstanceModel", false);
}

void errorCallback(int error, const char* description) {
    std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
}

}

bool DepthPeelBuffers::ensure(int newWidth, int newHeight, int peelCount) {
    if (newWidth <= 0 || newHeight <= 0) return false;
    if (peelCount < 1) peelCount = 1;
    if (peelCount > kMaxDepthPeelCap) peelCount = kMaxDepthPeelCap;

    const bool first = fbo == 0;
    const bool sizeChanged = !first && (width != newWidth || height != newHeight);
    if (!first && !sizeChanged && static_cast<int>(colour.size()) == peelCount) return true;

    if (first) {
        glGenFramebuffers(1, &fbo);
        glGenTextures(2, depth);
        glGenTextures(1, &dummyDepth);
        glGenQueries(1, &query);
        glBindTexture(GL_TEXTURE_2D, dummyDepth);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 1, 1, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
        bindDepthTexture(dummyDepth);

        const float triangle[] = {-1.f, -1.f, 3.f, -1.f, -1.f, 3.f};
        glGenVertexArrays(1, &compositeVao);
        glGenBuffers(1, &compositeVbo);
        glBindVertexArray(compositeVao);
        glBindBuffer(GL_ARRAY_BUFFER, compositeVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(triangle), triangle, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
        glBindVertexArray(0);
    }

    if (first || sizeChanged) {
        for (unsigned int texture : depth) {
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, newWidth, newHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
            bindDepthTexture(texture);
        }
    }

    int have = static_cast<int>(colour.size());
    if (have > peelCount) {
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(have - peelCount, colour.data() + peelCount);
        colour.resize(static_cast<std::size_t>(peelCount));
        have = peelCount;
    }
    if (have < peelCount) {
        colour.resize(static_cast<std::size_t>(peelCount));
        glGenTextures(peelCount - have, colour.data() + have);
    }
    const int uploadFrom = (first || sizeChanged) ? 0 : have;
    for (int index = uploadFrom; index < peelCount; ++index) {
        glBindTexture(GL_TEXTURE_2D, colour[static_cast<std::size_t>(index)]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, newWidth, newHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colour[0], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth[0], 0);
    const bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (!complete) {
        if (!reportedIncomplete) {
            std::cerr << "Depth peel framebuffer is incomplete\n";
            reportedIncomplete = true;
        }
        return false;
    }

    width = newWidth;
    height = newHeight;
    return true;
}

void DepthPeelBuffers::destroy() {
    if (fbo != 0) glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(2, depth);
    if (!colour.empty()) glDeleteTextures(static_cast<GLsizei>(colour.size()), colour.data());
    if (dummyDepth != 0) glDeleteTextures(1, &dummyDepth);
    if (query != 0) glDeleteQueries(1, &query);
    if (compositeVbo != 0) glDeleteBuffers(1, &compositeVbo);
    if (compositeVao != 0) glDeleteVertexArrays(1, &compositeVao);
    fbo = 0;
    depth[0] = depth[1] = 0;
    colour.clear();
    width = 0;
    height = 0;
    dummyDepth = 0;
    query = 0;
    compositeVbo = 0;
    compositeVao = 0;
}

bool Application::init() {
    glfwSetErrorCallback(errorCallback);

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    glfwReady = true;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    const int windowWidth = 800;
    const int windowHeight = 600;
    window = glfwCreateWindow(windowWidth, windowHeight, "TraitVis", nullptr, nullptr);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        shutdown();
        return false;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        shutdown();
        return false;
    }
    glReady = true;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
    imguiReady = true;
    ImFont* font = io.Fonts->AddFontFromFileTTF("assets/fonts/OpenSans-VariableFont_wdth,wght.ttf", 24.0f);
    if (font != nullptr) io.FontDefault = font;

    glfwSwapInterval(1);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glViewport(0, 0, windowWidth, windowHeight);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glEnable(GL_DEPTH_TEST);
    glClearColor(1.f, 1.f, 1.f, 0.f);

    try {
        lighting.emplace(std::vector<ShaderProgram::ShaderSource>{
            {GL_VERTEX_SHADER, "assets/litSurface.vert"},
            {GL_FRAGMENT_SHADER, "assets/litSurface.frag"}
        });
        composite.emplace(std::vector<ShaderProgram::ShaderSource>{
            {GL_VERTEX_SHADER, "assets/compositeLayer.vert"},
            {GL_FRAGMENT_SHADER, "assets/compositeLayer.frag"}
        });
        distanceField.emplace();
        session.gpuDistanceField = [this](std::span<const TraitPoint> points, const MultiField& field) {
            return distanceField->compute(points, field);
        };
    } catch (const std::runtime_error& error) {
        std::cerr << "Fatal: " << error.what() << std::endl;
        shutdown();
        return false;
    }

    rendering.emplace(RenderState{
        {glm::vec3(560.f, -200.f, 560.f), glm::vec3(0.f, 0.f, -1.f), glm::vec3(0.f, 1.f, 0.f)},
        windowWidth,
        windowHeight
    });
    gui.emplace(session, *rendering);
    glfwSetWindowUserPointer(window, &*rendering);
    cube.emplace(uploadMesh(std::span<const float>(cubeVerticesWithNormals)));

    glUseProgram(lighting->program.get());
    lighting->setUniformf("objectTransparency", 1.f);
    lighting->setUniformb("useInstanceModel", false);
    lighting->setUniformb("hasPeelDepth", false);
    lighting->setUniformi("peelDepth", 0);
    glUseProgram(composite->program.get());
    composite->setUniformi("layer", 0);
    return true;
}

void Application::frame() {
    RenderState& state = *rendering;
    state.updateTime();
    glfwPollEvents();

    ImGuiIO& io = ImGui::GetIO();
    if (not io.WantCaptureKeyboard) processInput(window);

    double xpos = 0.0;
    double ypos = 0.0;
    glfwGetCursorPos(window, &xpos, &ypos);
    if (not io.WantCaptureMouse and state.enableCam) state.cam.updateDirection(xpos, ypos);
    else state.cam.updateMousePos(xpos, ypos);
    state.cam.updateCamera(state.deltaTime);

    glDisable(GL_SCISSOR_TEST);
    glViewport(state.windowWidth * (1 - state.renderWidthPercentage), 0, state.windowWidth * state.renderWidthPercentage, state.windowHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 proj = state.getProjectionMatrix();
    glm::mat4 view = state.cam.getViewMatrix();

#ifndef __EMSCRIPTEN__
    if (state.drawWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif

    if (state.backFaceCulling) glEnable(GL_CULL_FACE);
    else glDisable(GL_CULL_FACE);

    ShaderProgram& lightingShader = *lighting;
    glUseProgram(lightingShader.program.get());
    lightingShader.setUniform3f("ambientLightColour", state.ambientLightColour);
    lightingShader.setUniformf("ambientLightStrength", state.ambientLightStrength);
    lightingShader.setUniform3f("diffuseLightColour", state.diffuseLightColour);
    lightingShader.setUniformf("diffuseLightStrength", state.diffuseLightStrength);
    lightingShader.setUniform3f("diffuseLightDirection", glm::normalize(state.diffuseLightDirection));
    lightingShader.setUniformMat4f("projection", &proj);
    lightingShader.setUniformMat4f("view", &view);
    lightingShader.setUniformMat4f("model", &defaultModel);
    lightingShader.setUniformb("hasPeelDepth", false);
    glm::vec3 clipNormal(0.f);
    if (state.clipAxis == 0) clipNormal.x = 1.f;
    else if (state.clipAxis == 2) clipNormal.z = 1.f;
    else clipNormal.y = 1.f;
    lightingShader.setUniformb("clipEnabled", state.clipEnabled);
    lightingShader.setUniform3f("clipNormal", clipNormal);
    lightingShader.setUniformf("clipDistance", state.clipPosition);
    glActiveTexture(GL_TEXTURE0);
    if (peels.dummyDepth != 0) glBindTexture(GL_TEXTURE_2D, peels.dummyDepth);

    glDepthMask(GL_TRUE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (auto& levelSet : session.levelSets) {
        if (levelSet.active && levelSet.transparency >= 1.f)
            drawLevelSet(lightingShader, levelSet, *cube, colourGreen, colourRed);
    }

    int fbWidth = 0;
    int fbHeight = 0;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    bool anyTransparent = false;
    for (const auto& levelSet : session.levelSets)
        if (levelSet.active && levelSet.transparency > 0.f && levelSet.transparency < 1.f) anyTransparent = true;

    int peelCount = 0;
    if (anyTransparent && peels.ensure(fbWidth, fbHeight, state.maxDepthPeels)) {
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        const glm::vec4 sceneClear = glm::vec4(1.f, 1.f, 1.f, 0.f);
        glClearColor(0.f, 0.f, 0.f, 0.f);

        for (int peel = 0; peel < state.maxDepthPeels; ++peel) {
            const unsigned int writeDepth = peels.depth[peel % 2];
            glBindFramebuffer(GL_FRAMEBUFFER, peels.fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, peels.colour[peel], 0);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, writeDepth, 0);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, peels.fbo);
            glBlitFramebuffer(0, 0, fbWidth, fbHeight, 0, 0, fbWidth, fbHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
            glBindFramebuffer(GL_FRAMEBUFFER, peels.fbo);
            glClear(GL_COLOR_BUFFER_BIT);

            glActiveTexture(GL_TEXTURE0);
            lightingShader.setUniformb("hasPeelDepth", peel > 0);
            glBindTexture(GL_TEXTURE_2D, peel > 0 ? peels.depth[(peel + 1) % 2] : peels.dummyDepth);

            glBeginQuery(GL_ANY_SAMPLES_PASSED, peels.query);
            for (auto& levelSet : session.levelSets) {
                if (levelSet.active && levelSet.transparency > 0.f && levelSet.transparency < 1.f)
                    drawLevelSet(lightingShader, levelSet, *cube, colourGreen, colourRed);
            }
            glEndQuery(GL_ANY_SAMPLES_PASSED);
            unsigned int passed = 0;
            glGetQueryObjectuiv(peels.query, GL_QUERY_RESULT, &passed);
            if (passed == 0) break;
            peelCount = peel + 1;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(sceneClear.r, sceneClear.g, sceneClear.b, sceneClear.a);
        glActiveTexture(GL_TEXTURE0);
        if (peelCount > 0) {
#ifndef __EMSCRIPTEN__
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glUseProgram(composite->program.get());
            glBindVertexArray(peels.compositeVao);
            for (int peel = peelCount - 1; peel >= 0; --peel) {
                glBindTexture(GL_TEXTURE_2D, peels.colour[peel]);
                glDrawArrays(GL_TRIANGLES, 0, 3);
            }
            glEnable(GL_DEPTH_TEST);
#ifndef __EMSCRIPTEN__
            if (state.drawWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
#endif
        }
        if (state.backFaceCulling) glEnable(GL_CULL_FACE);
        else glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glDepthMask(GL_TRUE);
        glUseProgram(lightingShader.program.get());
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    gui->render();
    gui->renderLightingControls();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
}

void Application::shutdown() {
    if (shutDown) return;
    shutDown = true;

    // GPU ids have to be deleted while the context is still current.
    if (glReady) {
        if (imguiReady) {
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
            imguiReady = false;
        }
        peels.destroy();
        cube.reset();
        session.clearHistory();
        session.levelSets.clear();
        session.gpuDistanceField = nullptr;
        distanceField.reset();
        lighting.reset();
        composite.reset();
        glReady = false;
    }
    gui.reset();
    if (window != nullptr) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    if (glfwReady) {
        glfwTerminate();
        glfwReady = false;
    }
}

bool Application::shouldClose() const {
    return window == nullptr || glfwWindowShouldClose(window);
}
