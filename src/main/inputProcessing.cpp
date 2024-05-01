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
        }

        if (state->enableCam) {
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) state->cam.actionForwards = true;
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_RELEASE) state->cam.actionForwards = false;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) state->cam.actionBackwards = true;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_RELEASE) state->cam.actionBackwards = false;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) state->cam.actionLeft = true;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_RELEASE) state->cam.actionLeft = false;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) state->cam.actionRight = true;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_RELEASE) state->cam.actionRight = false;
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) state->cam.actionUp = true;
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_RELEASE) state->cam.actionUp = false;
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) state->cam.actionDown = true;
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_RELEASE) state->cam.actionDown = false;
        }
    }
}