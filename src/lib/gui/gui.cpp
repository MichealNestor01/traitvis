#include <GLFW/glfw3.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "gui.hpp"
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


    if (ImGui::Button("Load Dataset")) {
        programState.loadDataset(std::string(inputTextBuffer)); 
        createDatasetWidgets();
    }
    ImGui::SameLine();
    // Text input
    ImGui::InputText(" ", inputTextBuffer, IM_ARRAYSIZE(inputTextBuffer));


    // Display the text from the input box underneath it
    if (programState.loadedDataset) {
        if (programState.dataset.readError) {
            ImGui::Text("Dataset read error, check console output.");
        } else {
            std::string message = "Loaded dataset: " + programState.dataset.name;
            ImGui::Text("%s", message.c_str());
            ImGui::Text("Define Trait:");
            // draw all the widgets in the attributeWidgets vector
            for (auto &widget : attributeWidgets) {
                ImGui::Text("%s: ", widget.attribute.name.c_str());
                std::string label = "##" + widget.attribute.name;
                ImGui::SliderFloat(label.c_str(), &widget.value, widget.attribute.bounds.lower, widget.attribute.bounds.upper, "%.7f");
            }
        }
    }

    // Your UI code here
    ImGui::End();
}

void GUI::createDatasetWidgets() {
    if (programState.dataset.readError) return;
    for (Attribute& attribute : programState.dataset.attributeDomain) {
        attributeWidgets.push_back({attribute, attribute.bounds.lower});
    }
}