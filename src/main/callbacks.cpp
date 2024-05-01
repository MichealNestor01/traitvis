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