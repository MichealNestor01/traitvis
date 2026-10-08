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
#include <algorithm>

// my includes
#include "state.hpp"
#include "callbacks.hpp"
#include "inputProcessing.hpp"
#include "../lib/shaderTools/shaderProgram.hpp"
#include "../lib/meshTools/createVao.hpp"
#include "../lib/meshTools/cube.hpp"
#include "../lib/marchingCubes/marchingCubes.hpp"
#include "../lib/gui/gui.hpp"

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
    try {
        lightingShaderOpt.emplace(std::vector<ShaderProgram::ShaderSource>{
            {GL_VERTEX_SHADER, "assets/litSurface.vert"},
            {GL_FRAGMENT_SHADER, "assets/litSurface.frag"}
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

        // sort level sets by render depth
        std::sort(state.levelSets.begin(), state.levelSets.end());
        // render level sets
        if (state.levelSets.size() > 0) {
            lightingShader.setUniformMat4f("model", &defaultModel);
            for (auto & levelSet : state.levelSets) {
                if (levelSet.active) {
                    if (levelSet.VAO == 0) {
                        const std::vector<float> interleaved = levelSet.surface.interleaved();
                        levelSet.VAO = createVAO(interleaved.data(), interleaved.size()*sizeof(float));
                        levelSet.activeInstanceVBO = boundaryCubeInstances(levelSet.surface.activeVertices);
                        levelSet.inactiveInstanceVBO = boundaryCubeInstances(levelSet.surface.inactiveVertices);
                    }
                    lightingShader.setUniformb("invertNormal", levelSet.invertNormals);
                    lightingShader.setUniform3f("objectColor", levelSet.colour);
                    lightingShader.setUniformf("objectTransparency", levelSet.transparency);
                    if (levelSet.transparency < 1.f) glDepthMask(GL_FALSE);
                    else glDepthMask(GL_TRUE);
                    lightingShader.setUniformb("useInstanceModel", false);
                    glBindVertexArray(levelSet.VAO);
                    glDrawArrays(GL_TRIANGLES, 0, levelSet.surface.vertices.size());

                    if (levelSet.showActiveInactivePixels) {
                        lightingShader.setUniformb("useInstanceModel", true);
                        glBindVertexArray(cubeVAO);
                        if (levelSet.activeInstanceVBO != 0) {
                            lightingShader.setUniform3f("objectColor", colourGreen);
                            bindInstanceBuffer(levelSet.activeInstanceVBO);
                            glDrawArraysInstanced(GL_TRIANGLES, 0, 6*2*3, levelSet.surface.activeVertices.size());
                        }
                        if (levelSet.inactiveInstanceVBO != 0) {
                            lightingShader.setUniform3f("objectColor", colourRed);
                            bindInstanceBuffer(levelSet.inactiveInstanceVBO);
                            glDrawArraysInstanced(GL_TRIANGLES, 0, 6*2*3, levelSet.surface.inactiveVertices.size());
                        }
                        lightingShader.setUniformb("useInstanceModel", false);
                    }
                }
            }
        }
        glDepthMask(GL_TRUE);

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
    glfwTerminate();
    return 0;
}