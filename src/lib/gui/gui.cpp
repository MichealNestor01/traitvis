#include <GLFW/glfw3.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "../gui/gui.hpp"
#include "../../main/state.hpp" 

void GUI::render() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // ImGui UI on the left 30%
    ImGui::SetNextWindowPos(ImVec2(0, 0)); // Position at top-left corner
    ImGui::SetNextWindowSize(ImVec2(programState.windowWidth * (1-programState.renderWidthPercentage), programState.windowHeight)); // 30% width, full height
    ImGui::Begin("Controls");
    ImGui::SetWindowFontScale(1.f);

    // Display the counter
    ImGui::Text("Counter: %d", counter);
    // Button to increment the counter
    if (ImGui::Button("Increment")) {
        counter++; // Increment the counter when the button is clicked
    }

    // Your UI code here
    ImGui::End();
}