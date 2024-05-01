#include "inputProcessing.hpp"

#include <GLFW/glfw3.h>

#include "state.hpp"

#include <iostream>

// space bar command only called once per press
// facilitated by spaceLock
bool spaceLock = false;

void processInput(GLFWwindow* window) {
    // program termination
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (auto* state = static_cast<ProgramState*>(glfwGetWindowUserPointer(window))) {
        // dissable/enable mouse
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS and not spaceLock) {
            state->toggleCam(window);
            spaceLock = true;
        } else if (spaceLock and glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
            spaceLock = false;
            state->cam.firstMouseMovement = true;
        }

        // camera movement controls
        if (state->enableCam) {
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                state->cam.moveForward(state->deltaTime);
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                state->cam.moveBackward(state->deltaTime);
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                state->cam.moveLeft(state->deltaTime);
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                state->cam.moveRight(state->deltaTime);
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
                state->cam.moveUp(state->deltaTime);
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
                state->cam.moveDown(state->deltaTime);
        }
    }
}