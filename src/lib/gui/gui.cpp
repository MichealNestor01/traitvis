#include <GLFW/glfw3.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "../gui/gui.hpp"
#include "../../main/state.hpp" 

int counter = 0;

void renderGUI(ProgramState &state) {
    // setup ui before rendering:
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // ImGui UI on the left 30%
    ImGui::SetNextWindowPos(ImVec2(0, 0)); // Position at top-left corner
    ImGui::SetNextWindowSize(ImVec2(state.windowWidth * (1-state.renderWidthPercentage), state.windowHeight)); // 30% width, full height
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