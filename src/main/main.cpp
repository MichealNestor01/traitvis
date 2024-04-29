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
#include <math.h>
#include <set>
#include <algorithm>

// my includes
#include "state.hpp"
#include "callbacks.hpp"
#include "inputProcessing.hpp"
#include "../lib/shaderTools/shaderProgram.hpp"
#include "../lib/meshTools/createVao.hpp"
#include "../lib/meshTools/cube.hpp"
#include "../lib/marchingCubes/marchingCubes.hpp"
#include "../lib/marchingCubes/grid.hpp"
#include "../lib/gui/gui.hpp"
#include "../lib/marchingCubes/mc_tables.h"

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
    ImFont* font = io.Fonts->AddFontFromFileTTF("assets/OpenSans-VariableFont_wdth,wght.ttf", 24.0f);
    if (font != nullptr) io.FontDefault = font;

    // set mouse input mode + callbacks
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);  
    //glfwSetCursorPosCallback(window, mouse_callback);  
    glfwSetScrollCallback(window, scroll_callback);
    // setup opengl blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // set opengl to use wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    // set cull face
    glEnable(GL_CULL_FACE);
    // tell opengl how big the window should be 
    glViewport(0, 0, windowWidth, windowHeight);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    // gl config
    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_CULL_FACE);
    glClearColor(0.52f, 0.81f, 0.92f, 0.f);

    // create shader programs
    ShaderProgram lightingShader({
        {GL_VERTEX_SHADER, "assets/colours.vert"},
        {GL_FRAGMENT_SHADER, "assets/colours.frag"}
    });
    
    // global lighting variables
    glm::vec3 lightPos(0.5f, -3.f, -3.f);
    glm::vec3 lightColour(1.f, 1.f, 1.f);
    // global colour variables
    glm::vec3 colourCoral(1.f, 0.5f, 0.31f);
    glm::vec3 colourRed(1.f, 0.f, 0.f);
    glm::vec3 colourGreen(0.f, 1.f, 0.f);
    glm::vec3 colourTurquoise(0.02f, 1.f, 0.8f);
    glm::vec3 colourWhite(1.f, 1.f, 1.f);
    // default model matrix (no change)
    glm::mat4 defaultModel(1.f);

    // camera variables 
    glm::vec3 defaultCameraPos = glm::vec3(0.f, 0.f, 3.f);
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

    // test grid
    GridLayout layout = {3, 3, 3}; 
    std::vector<std::vector<float>> grid = {
        // x layer 1
        {0, 0, 0, 1},  {0, 0, 1, 1},  {0, 0, 2, 1}, 
        {0, 1, 0, 1},  {0, 1, 1, 1},  {0, 1, 2, 1}, 
        {0, 2, 0, 1},  {0, 2, 1, 1},  {0, 2, 2, 1}, 
        // x layer 2
        {1, 0, 0, 1},  {1, 0, 1, 1},  {1, 0, 2, 1}, 
        {1, 1, 0, 1},  {1, 1, 1, -1},  {1, 1, 2, 1}, 
        {1, 2, 0, 1},  {1, 2, 1, 1},  {1, 2, 2, 1}, 
        // x layer 3
        {2, 0, 0, 1},  {2, 0, 1, 1},  {2, 0, 2, 1}, 
        {2, 1, 0, 1},  {2, 1, 1, 1},  {2, 1, 2, 1}, 
        {2, 2, 0, 1},  {2, 2, 1, 1},  {2, 2, 2, 1}, 
    };
    float isoValue = 0;

    // code to display specific marching cubes case
    // GridLayout layout = {2, 2, 2};
    // float isoValue = 0; 
    // int caseNum = 6;
    // std::vector<std::vector<float>> grid = {
    //     // Layer 1
    //     {0, 0, 0, static_cast<float>((caseNum & 1) ? 1 : -1)},
    //     {0, 0, 1, static_cast<float>((caseNum & 2) ? 1 : -1)},
    //     {0, 1, 0, static_cast<float>((caseNum & 4) ? 1 : -1)},
    //     {0, 1, 1, static_cast<float>((caseNum & 8) ? 1 : -1)},
    //     // Layer 2
    //     {1, 0, 0, static_cast<float>((caseNum & 16) ? 1 : -1)},
    //     {1, 0, 1, static_cast<float>((caseNum & 32) ? 1 : -1)},
    //     {1, 1, 0, static_cast<float>((caseNum & 64) ? 1 : -1)},
    //     {1, 1, 1, static_cast<float>((caseNum & 128) ? 1 : -1)}
    // };

    //create surface based on active vertices in the grid
    //if a surface exists createa a VAO for it.
    Surface surface = extractSurface(grid, layout, isoValue);
    unsigned int surfaceVAO;
    if (surface.vertices.size()) {
        std::vector<float> surfaceVertices = surface.getVertices();
        surfaceVAO = createVAO(surfaceVertices.data(), surfaceVertices.size()*sizeof(float));
    }
    
    // create cube vao
    unsigned int cubeVAO = createVAO(cubeVerticesWithNormals, sizeof(cubeVerticesWithNormals));
    
    // // create model matrices to place small cubes at each point on the coordinate grid 
    std::vector<glm::mat4> pixels;
    for (int pixel = 0; pixel < layout.total; ++pixel) 
       pixels.push_back(glm::scale(glm::translate(glm::mat4(1.f), glm::vec3({grid[pixel][0], grid[pixel][1], grid[pixel][2]})), glm::vec3(0.1f)));

    // setup static uniforms for the lighting shader that don't change
    glUseProgram(lightingShader.program);
    lightingShader.setUniform3f("lightColor", lightColour);
    lightingShader.setUniform3f("lightPos", lightPos);
    lightingShader.setUniformf("objectTransparency", 1.f);
    // global lighting
    lightingShader.setUniform3f("lightDir", glm::normalize(glm::vec3(-1.f, -1.f, -1.f)));
    lightingShader.setUniform3f("lightDiffuse", glm::vec3(0.2f, 0.2f, 0.2f));
    lightingShader.setUniform3f("sceneAmbient", glm::vec3(0.4f, 0.4f, 0.4f));

    float lightAngle = 0.0f;
    float lightRadius = 7.0f;
    glm::vec3 rotationCenter(2.0f, 2.0f, 2.0f); // Center of rotation

    // mainloop 
    while (!glfwWindowShouldClose(window)) {
        // poll glfw events 
        glfwPollEvents();

        // update frame time
        state.updateTime();

        // control user inputs
        processInput(window);
        if (not io.WantCaptureMouse and state.enableCam) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            state.cam.updateDirection(xpos, ypos);
        }

        glViewport(state.windowWidth * (1-state.renderWidthPercentage), 0, state.windowWidth  * state.renderWidthPercentage, state.windowHeight);

        // get update projection and view matrix
        glm::mat4 proj = state.getProjectionMatrix();
        glm::mat4 view = state.cam.getViewMatrix();
        // rendering 
        glClear(GL_COLOR_BUFFER_BIT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (state.drawWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

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
        // update camera position
        lightingShader.setUniform3f("viewPos", state.cam.getPos());
        lightingShader.setUniformMat4f("projection", &proj);
        lightingShader.setUniformMat4f("view", &view);
        lightingShader.setUniform3f("lightPos", lightPos);

        //render test pixels and test surface
        lightingShader.setUniformf("objectTransparency", 1.f);
        for (int pixel = 0; pixel < pixels.size(); ++pixel) {
            if (grid[pixel][3] > isoValue) lightingShader.setUniform3f("objectColor", colourRed);
            else lightingShader.setUniform3f("objectColor", colourGreen);
            lightingShader.setUniformMat4f("model", &pixels[pixel]);
            glBindVertexArray(cubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6*2*3);
        }
        if (surface.vertices.size()) {
            lightingShader.setUniform3f("objectColor", colourGreen);
            lightingShader.setUniformMat4f("model", &defaultModel);
            glBindVertexArray(surfaceVAO);
            glDrawArrays(GL_TRIANGLES, 0, surface.vertices.size());
        }
        
        // sort level sets by render depth
        std::sort(state.levelSets.begin(), state.levelSets.end());
        // render level sets
        if (state.levelSets.size() > 0) {
            lightingShader.setUniformMat4f("model", &defaultModel);
            for (auto & levelSet : state.levelSets) {
                if (levelSet.active) {
                    if (levelSet.VAO == 0) levelSet.VAO = createVAO(levelSet.surfaceVertices.data(), levelSet.surfaceVertices.size()*sizeof(float));
                    lightingShader.setUniformb("invertNormal", levelSet.invertNormals);
                    lightingShader.setUniform3f("objectColor", levelSet.colour);
                    lightingShader.setUniformf("objectTransparency", levelSet.transparency);
                    if (levelSet.transparency < 1.f) glDepthMask(GL_FALSE);
                    else glDepthMask(GL_TRUE);
                    glBindVertexArray(levelSet.VAO);
                    glDrawArrays(GL_TRIANGLES, 0, levelSet.surfaceVertices.size());
                    
                    if (levelSet.showActiveInactivePixels) {
                        // generate boundary vertices
                        glBindVertexArray(cubeVAO);
                        lightingShader.setUniform3f("objectColor", colourGreen);
                        for (auto & vertex : levelSet.surface.activeVertices) {
                            //std::cout << "Active vertex: " << vertex.x << " " << vertex.y << " " << vertex.z << "\n";
                            glm::mat4 cubeModel = glm::scale(glm::translate(glm::mat4(1.f), vertex), glm::vec3(0.1f));
                            lightingShader.setUniformMat4f("model", &cubeModel);
                            glDrawArrays(GL_TRIANGLES, 0, 6*2*3);
                        }
                        lightingShader.setUniform3f("objectColor", colourRed);
                        for (auto & vertex : levelSet.surface.inactiveVertices) {
                            //std::cout << "Active vertex: " << vertex.x << " " << vertex.y << " " << vertex.z << "\n";
                            glm::mat4 cubeModel = glm::scale(glm::translate(glm::mat4(1.f), vertex), glm::vec3(0.1f));
                            lightingShader.setUniformMat4f("model", &cubeModel);
                            glDrawArrays(GL_TRIANGLES, 0, 6*2*3);
                        }
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

        // check and call events and swap the buffers
        glfwSwapBuffers(window);
    }

    // cleanup
    //glDeleteVertexArrays(1, &cubeVAO);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}