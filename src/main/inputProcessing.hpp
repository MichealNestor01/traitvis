#pragma once

#include <GLFW/glfw3.h>

void processInput(GLFWwindow *window);
#ifdef __EMSCRIPTEN__
// Runs inside the key event, so a pointer-lock request still counts as a user gesture.
void onWebKey(GLFWwindow* window, int key, int action);
#endif