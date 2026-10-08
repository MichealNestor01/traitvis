// libraries
#include <glad/glad.h> 
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>

// my includes
#include "state.hpp"
#include "callbacks.hpp"
#include "inputProcessing.hpp"
#include "../lib/shaderTools/shaderProgram.hpp"
#include "../lib/meshTools/createVao.hpp"
#include "../lib/meshTools/cube.hpp"
#include "../lib/marchingCubes/marchingCubes.hpp"
#include "../lib/gui/gui.hpp"

constexpr int kMaxDepthPeels = 16;

struct DepthPeelBuffers {
    unsigned int fbo = 0;
    unsigned int depth[2] = {};
    unsigned int colour[kMaxDepthPeels] = {};
    unsigned int dummyDepth = 0;
    unsigned int query = 0;
    unsigned int compositeVao = 0;
    unsigned int compositeVbo = 0;
    int width = 0;
    int height = 0;
    bool reportedIncomplete = false;

    bool ensure(int newWidth, int newHeight);
    void destroy();
};

void bindDepthTexture(unsigned int texture) {
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
}

bool DepthPeelBuffers::ensure(int newWidth, int newHeight) {
    if (newWidth <= 0 || newHeight <= 0) return false;
    if (fbo != 0 && width == newWidth && height == newHeight) return true;

    if (fbo == 0) {
        glGenFramebuffers(1, &fbo);
        glGenTextures(2, depth);
        glGenTextures(kMaxDepthPeels, colour);
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

    for (unsigned int texture : depth) {
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, newWidth, newHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
        bindDepthTexture(texture);
    }
    for (unsigned int texture : colour) {
        glBindTexture(GL_TEXTURE_2D, texture);
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
    glDeleteTextures(kMaxDepthPeels, colour);
    if (dummyDepth != 0) glDeleteTextures(1, &dummyDepth);
    if (query != 0) glDeleteQueries(1, &query);
    if (compositeVbo != 0) glDeleteBuffers(1, &compositeVbo);
    if (compositeVao != 0) glDeleteVertexArrays(1, &compositeVao);
    fbo = 0;
}

unsigned int boundaryCubeInstances(const std::set<glm::vec3, Vec3Comparator>& vertices);

void drawLevelSet(ShaderProgram& shader, FeatureLevelSet& levelSet, unsigned int cubeVAO, const glm::vec3& activeColour, const glm::vec3& inactiveColour) {
    if (levelSet.VAO == 0) {
        const std::vector<float> interleaved = levelSet.surface.interleaved();
        levelSet.VAO = createVAO(interleaved.data(), interleaved.size() * sizeof(float));
        levelSet.activeInstanceVBO = boundaryCubeInstances(levelSet.surface.activeVertices);
        levelSet.inactiveInstanceVBO = boundaryCubeInstances(levelSet.surface.inactiveVertices);
    }
    shader.setUniformb("invertNormal", levelSet.invertNormals);
    shader.setUniform3f("objectColor", levelSet.colour);
    shader.setUniformf("objectTransparency", levelSet.transparency);
    shader.setUniformb("useInstanceModel", false);
    glBindVertexArray(levelSet.VAO);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(levelSet.surface.vertices.size()));

    if (!levelSet.showActiveInactivePixels) return;
    shader.setUniformb("useInstanceModel", true);
    glBindVertexArray(cubeVAO);
    if (levelSet.activeInstanceVBO != 0) {
        shader.setUniform3f("objectColor", activeColour);
        bindInstanceBuffer(levelSet.activeInstanceVBO);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6 * 2 * 3, static_cast<GLsizei>(levelSet.surface.activeVertices.size()));
    }
    if (levelSet.inactiveInstanceVBO != 0) {
        shader.setUniform3f("objectColor", inactiveColour);
        bindInstanceBuffer(levelSet.inactiveInstanceVBO);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6 * 2 * 3, static_cast<GLsizei>(levelSet.surface.inactiveVertices.size()));
    }
    shader.setUniformb("useInstanceModel", false);
}

unsigned int boundaryCubeInstances(const std::set<glm::vec3, Vec3Comparator>& vertices) {
    std::vector<glm::mat4> models;
    models.reserve(vertices.size());
    for (const glm::vec3& vertex : vertices)
        models.push_back(glm::scale(glm::translate(glm::mat4(1.f), vertex), glm::vec3(0.1f)));
    return createInstanceBuffer(models.data(), models.size());
}

// Define the error callback function
void error_callback(int error, const char* description) {
    std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
}

int main() {
    glfwSetErrorCallback(error_callback);

    // initialise glfw and ensure opengl version is 4.6
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // create the window
    int windowWidth = 800;
    int windowHeight = 600;
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "TraitVis", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // validate glad is working
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // setup imgui 
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130"); 
    ImFont* font = io.Fonts->AddFontFromFileTTF("assets/fonts/OpenSans-VariableFont_wdth,wght.ttf", 24.0f);
    if (font != nullptr) io.FontDefault = font;

    // set vsync
    glfwSwapInterval(1);
    // set mouse input mode + callbacks
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    // setup opengl blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // set cull face
    glEnable(GL_CULL_FACE);
    // tell opengl how big the window should be
    glViewport(0, 0, windowWidth, windowHeight);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    // gl config
    glEnable(GL_DEPTH_TEST);
    glClearColor(1.f, 1.f, 1.f, 0.f);

    // create shader programs (constructed in place: ShaderProgram owns a GL
    // program id, so it must not be copied or move-assigned)
    std::optional<ShaderProgram> lightingShaderOpt;
    std::optional<ShaderProgram> compositeShaderOpt;
    try {
        lightingShaderOpt.emplace(std::vector<ShaderProgram::ShaderSource>{
            {GL_VERTEX_SHADER, "assets/litSurface.vert"},
            {GL_FRAGMENT_SHADER, "assets/litSurface.frag"}
        });
        compositeShaderOpt.emplace(std::vector<ShaderProgram::ShaderSource>{
            {GL_VERTEX_SHADER, "assets/compositeLayer.vert"},
            {GL_FRAGMENT_SHADER, "assets/compositeLayer.frag"}
        });
    } catch (const std::runtime_error& e) {
        std::cerr << "Fatal: " << e.what() << std::endl;
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    ShaderProgram& lightingShader = lightingShaderOpt.value();
    ShaderProgram& compositeShader = compositeShaderOpt.value();
    
    glm::vec3 colourRed(1.f, 0.f, 0.f);
    glm::vec3 colourGreen(0.f, 1.f, 0.f);
    // default model matrix (no change)
    glm::mat4 defaultModel(1.f);

    // camera variables 
    glm::vec3 defaultCameraPos = glm::vec3(560.f, -200.f, 560.f);
    glm::vec3 defaultCameraFront = glm::vec3(0.f, 0.f, -1.f);
    glm::vec3 defaultCameraUp = glm::vec3(0.f, 1.f, 0.f);
    // setup state
    ProgramState state = {
        {defaultCameraPos, defaultCameraFront, defaultCameraUp},
        windowWidth,
        windowHeight
    };
    glfwSetWindowUserPointer(window, &state);
    // setup gui 
    GUI gui(state);

    unsigned int cubeVAO = createVAO(cubeVerticesWithNormals, sizeof(cubeVerticesWithNormals));

    // setup static uniforms for the lighting shader that don't change
    glUseProgram(lightingShader.program);
    lightingShader.setUniformf("objectTransparency", 1.f);
    lightingShader.setUniformb("useInstanceModel", false);
    lightingShader.setUniformb("hasPeelDepth", false);
    lightingShader.setUniformi("peelDepth", 0);
    glUseProgram(compositeShader.program);
    compositeShader.setUniformi("layer", 0);

    DepthPeelBuffers peels;


    // mainloop 
    while (!glfwWindowShouldClose(window)) {
        state.updateTime();

        // poll glfw events 
        glfwPollEvents();

        // get keyboard inputs 
        if (not io.WantCaptureKeyboard) processInput(window);

        // update camera position attributes
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        if (not io.WantCaptureMouse and state.enableCam) state.cam.updateDirection(xpos, ypos);
        else state.cam.updateMousePos(xpos, ypos);
        state.cam.updateCamera(state.deltaTime);
    
        glDisable(GL_SCISSOR_TEST);
        glViewport(state.windowWidth * (1-state.renderWidthPercentage), 0, state.windowWidth  * state.renderWidthPercentage, state.windowHeight);
        // prepare frame for rendering
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // get update projection and view matrix
        glm::mat4 proj = state.getProjectionMatrix();
        glm::mat4 view = state.cam.getViewMatrix();


#ifndef __EMSCRIPTEN__
        if (state.drawWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif

        if (state.backFaceCulling) glEnable(GL_CULL_FACE);
        else glDisable(GL_CULL_FACE);

        // switch to general lighting shader
        glUseProgram(lightingShader.program);
        // update lighting uniforms
        lightingShader.setUniform3f("ambientLightColour", state.ambientLightColour);
        lightingShader.setUniformf("ambientLightStrength", state.ambientLightStrength);
        lightingShader.setUniform3f("diffuseLightColour", state.diffuseLightColour);
        lightingShader.setUniformf("diffuseLightStrength", state.diffuseLightStrength);
        lightingShader.setUniform3f("diffuseLightDirection", glm::normalize(state.diffuseLightDirection));
        lightingShader.setUniformMat4f("projection", &proj);
        lightingShader.setUniformMat4f("view", &view);
        lightingShader.setUniformMat4f("model", &defaultModel);
        lightingShader.setUniformb("hasPeelDepth", false);
        glActiveTexture(GL_TEXTURE0);
        if (peels.dummyDepth != 0) glBindTexture(GL_TEXTURE_2D, peels.dummyDepth);

        glDepthMask(GL_TRUE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        for (auto& levelSet : state.levelSets) {
            if (levelSet.active && levelSet.transparency >= 1.f)
                drawLevelSet(lightingShader, levelSet, cubeVAO, colourGreen, colourRed);
        }

        int fbWidth = 0;
        int fbHeight = 0;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        bool anyTransparent = false;
        for (const auto& levelSet : state.levelSets)
            if (levelSet.active && levelSet.transparency > 0.f && levelSet.transparency < 1.f) anyTransparent = true;

        int peelCount = 0;
        if (anyTransparent && peels.ensure(fbWidth, fbHeight)) {
            glDisable(GL_CULL_FACE);
            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
            glEnable(GL_DEPTH_TEST);
            const glm::vec4 sceneClear = glm::vec4(1.f, 1.f, 1.f, 0.f);
            glClearColor(0.f, 0.f, 0.f, 0.f);

            for (int peel = 0; peel < kMaxDepthPeels; ++peel) {
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
                for (auto& levelSet : state.levelSets) {
                    if (levelSet.active && levelSet.transparency > 0.f && levelSet.transparency < 1.f)
                        drawLevelSet(lightingShader, levelSet, cubeVAO, colourGreen, colourRed);
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
                glUseProgram(compositeShader.program);
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
            glUseProgram(lightingShader.program);
        }

        // draw ui 
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        gui.render();
        gui.renderLightingControls();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    peels.destroy();
    glfwTerminate();
    return 0;
}