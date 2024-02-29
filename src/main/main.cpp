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

int main() {
    // initialise glfw and ensure opengl version is 4.6
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // create the window
    int windowWidth = 800;
    int windowHeight = 600;
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Marching Cubes", NULL, NULL);
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

    // set mouse input mode + callbacks
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);  
    //glfwSetCursorPosCallback(window, mouse_callback);  
    glfwSetScrollCallback(window, scroll_callback);

    // tell opengl how big the window should be 
    glViewport(0, 0, windowWidth, windowHeight);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    // gl config
    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_CULL_FACE);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    // create shader programs
    ShaderProgram lightingShader({
        {GL_VERTEX_SHADER, "assets/colours.vert"},
        {GL_FRAGMENT_SHADER, "assets/colours.frag"}
    });
    ShaderProgram lightCubeShader({
        {GL_VERTEX_SHADER, "assets/light_cube.vert"},
        {GL_FRAGMENT_SHADER, "assets/light_cube.frag"}
    });
    
    // global lighting variables
    glm::vec3 lightPos(0.5f, 7.f, 10.f);
    glm::vec3 lightColour(1.f, 1.f, 1.f);
    // global colour variables
    glm::vec3 colourCoral(1.f, 0.5f, 0.31f);
    glm::vec3 colourRed(1.f, 0.f, 0.f);
    glm::vec3 colourGreen(0.f, 1.f, 0.f);
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


    // create coordinate grid
    GridLayout layout = {10, 10, 10};
    std::vector<glm::vec3> activeVertices = {
        glm::vec3(2, 1, 1),
        glm::vec3(2, 1, 2),
        glm::vec3(2, 2, 2),
        glm::vec3(2, 3, 2),
        glm::vec3(2, 3, 3)
    };
    int ** coordinateGrid = createGrid(layout, activeVertices);

    // create surface based on active vertices in the grid
    // if a surface exists createa a VAO for it.
    Triangles surface = extractTriangles(coordinateGrid, layout);
    unsigned int surfaceVAO;
    if (surface.vertices.size()) {
        std::vector<float> surfaceVertices = surface.getVertices();
        surfaceVAO = createVAO(surfaceVertices.data(), surfaceVertices.size()*sizeof(float));
    }
    
    // create cube vao
    unsigned int cubeVAO = createVAO(cubeVerticesWithNormals, sizeof(cubeVerticesWithNormals));
    
    // create model matrices to place small cubes at each point on the coordinate grid 
    std::vector<glm::mat4> pixels;
    for (int pixel = 0; pixel < layout.total; ++pixel) 
        pixels.push_back(glm::scale(glm::translate(glm::mat4(1.f), glm::vec3({coordinateGrid[pixel][0], coordinateGrid[pixel][1], coordinateGrid[pixel][2]})), glm::vec3(0.1f)));

    // create model matrix for the point light
    glm::mat4 lightCubeModel = glm::scale(glm::translate(glm::mat4(1.f), lightPos), glm::vec3(0.2f));

    // setup static uniforms for the lighting shader that don't change
    glUseProgram(lightingShader.program);
    lightingShader.setUniform3f("lightColor", lightColour);
    lightingShader.setUniform3f("lightPos", lightPos);

    // setup static uniforms for the light cube shader
    glUseProgram(lightCubeShader.program);
    lightCubeShader.setUniformMat4f("model", &lightCubeModel);

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

        // draw ui 
        renderGUI(state);

        glViewport(state.windowWidth * (1-state.renderWidthPercentage), 0, state.windowWidth  * state.renderWidthPercentage, state.windowHeight);

        // get update projection and view matrix
        glm::mat4 proj = state.getProjectionMatrix();
        glm::mat4 view = state.cam.getViewMatrix();
        // update light position
        //lightAngle += 0.02f; // Adjust speed as needed
        //lightPos.x = rotationCenter.x + cos(lightAngle) * lightRadius;
        //lightPos.y = rotationCenter.y + sin(lightAngle/2) * lightRadius * 0.5f;
        //lightPos.z = rotationCenter.z + sin(lightAngle) * lightRadius;
        //lightCubeModel = glm::scale(glm::translate(glm::mat4(1.f), lightPos), glm::vec3(0.2f));

        // rendering 
        glClear(GL_COLOR_BUFFER_BIT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // switch to general lighting shader
        glUseProgram(lightingShader.program);
        lightingShader.setUniform3f("viewPos", state.cam.getPos());
        lightingShader.setUniformMat4f("projection", &proj);
        lightingShader.setUniformMat4f("view", &view);
        lightingShader.setUniform3f("lightPos", lightPos);

        // render the pixels
        for (int pixel = 0; pixel < pixels.size(); ++pixel) {
            if (coordinateGrid[pixel][3] == 0) lightingShader.setUniform3f("objectColor", colourRed);
            else lightingShader.setUniform3f("objectColor", colourGreen);
            lightingShader.setUniformMat4f("model", &pixels[pixel]);
            glBindVertexArray(cubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6*2*3);
        }

        // draw surface 
        if (surface.vertices.size()) {
            lightingShader.setUniform3f("objectColor", colourGreen);
            lightingShader.setUniformMat4f("model", &defaultModel);
            glBindVertexArray(surfaceVAO);
            glDrawArrays(GL_TRIANGLES, 0, surface.vertices.size());
        }

        // swap to simple light cube shader
        glUseProgram(lightCubeShader.program);
        lightCubeShader.setUniformMat4f("model", &lightCubeModel);
        lightCubeShader.setUniformMat4f("projection", &proj);
        lightCubeShader.setUniformMat4f("view", &view);

        // render the light cube
        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6*2*3);

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        // check and call events and swap the buffers
        glfwSwapBuffers(window);
    }

    // cleanup
    glDeleteVertexArrays(1, &cubeVAO);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}