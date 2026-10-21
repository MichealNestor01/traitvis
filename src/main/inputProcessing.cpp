#include "inputProcessing.hpp"

#include <GLFW/glfw3.h>

#include "state.hpp"

#include <iostream>

// space bar command only called once per press
// facilitated by spaceLock
bool spaceLock = false;

void processInput(GLFWwindow* window) {
    auto* state = static_cast<RenderState*>(glfwGetWindowUserPointer(window));
#ifdef __EMSCRIPTEN__
    // Escape leaves pointer lock. Closing the window would try to close the tab.
    if (state && glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        state->releaseCam(window);
#else
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
#endif

    if (state) {
#ifndef __EMSCRIPTEN__
        // On the web this runs from the key callback instead, because pointer lock
        // is rejected outside the key event.
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS and not spaceLock) {
            state->toggleCam(window);
            spaceLock = true;
        }
#endif
        if (spaceLock and glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
            spaceLock = false;

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

#ifdef __EMSCRIPTEN__
void onWebKey(GLFWwindow* window, int key, int action) {
    if (action != GLFW_PRESS) return;
    auto* state = static_cast<RenderState*>(glfwGetWindowUserPointer(window));
    if (!state) return;
    if (key == GLFW_KEY_ESCAPE) {
        state->releaseCam(window);
        return;
    }
    if (key == GLFW_KEY_SPACE && !spaceLock) {
        state->toggleCam(window);
        spaceLock = true;
    }
}
#endif