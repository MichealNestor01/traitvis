#include <GLFW/glfw3.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <iostream>

#include "callbacks.hpp"
#include "state.hpp"


void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    if (auto* state = static_cast<ProgramState*>(glfwGetWindowUserPointer(window))) {
        state->setWidthHeight(width, height);
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (auto* state = static_cast<ProgramState*>(glfwGetWindowUserPointer(window))) {
        if (state->enableCam) state->cam.updateDirection(xpos, ypos);
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (auto* state = static_cast<ProgramState*>(glfwGetWindowUserPointer(window))) {
        if (state->enableCam) state->setFov(yoffset);
    }
}