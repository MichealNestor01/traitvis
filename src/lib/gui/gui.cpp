#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "gui.hpp"
#include "../../main/state.hpp" 
#include "../multiField/multiField.hpp"


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


    // Display the text from the input box underneath it)
    if (not programState.loadedDataset) {
        ImGui::End();
        return;
    }

    if (programState.dataset.readError) {
        ImGui::Text("Dataset read error, check console output.");
    } else {
        std::string message = "Loaded dataset: " + programState.dataset.name;
        ImGui::Text("%s", message.c_str());
        ImGui::Text(" ");
        ImGui::Text("Define Trait:");
        // draw all the widgets in the attributeWidgets vector
        for (auto &widget : attributeWidgets) {
            ImGui::Text("%s: ", widget.attribute.name.c_str());
            if (not widget.active) ImGui::BeginDisabled();
            ImGui::SliderFloat(("##slider" + widget.attribute.name).c_str(), &widget.value, widget.attribute.bounds.lower, widget.attribute.bounds.upper, "%.7f");
            if (not widget.active) ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::Checkbox(("##checkbox" + widget.attribute.name).c_str(), &widget.active);
        }
    }

    if (ImGui::Button("Add attribute vertex")) {
        AttributeVertex newVertex;
        for (auto &widget : attributeWidgets) {
            if (widget.active) {
                newVertex.values.push_back({widget.attribute, widget.value});
            }
        }
        if (newVertex.values.size() > 0) programState.addAttributeSpaceVertex(newVertex);
    }


    if (programState.attributeSpaceVertices.size() == 0) {
        ImGui::End();
        return;
    }
    
    ImGui::Text(" ");
    ImGui::Text("Attribute Vertices: ");
    int i = 0;
    for (auto &vertex : programState.attributeSpaceVertices) {
        if (ImGui::Button(std::string("Remove Attribute Vertex: " + std::to_string(i)).c_str())) {
            programState.attributeSpaceVertices.erase(programState.attributeSpaceVertices.begin() + i);
        }
        ImGui::SameLine();
        if (ImGui::Button(std::string("Load Attribute Vertex: " + std::to_string(i)).c_str())) {
            for (auto &widget : attributeWidgets) {
                bool foundMatch = false;
                for (auto &value : vertex.values) {
                    if (widget.attribute.name == value.attribute.name) {
                        foundMatch = true;
                        widget.active = true;
                        widget.value = value.value;
                    } 
                }
                if (not foundMatch) widget.active = false;
            }
        }
        i++;
    }

    ImGui::Text("Select Euclidian Distance for level set: ");
    ImGui::SliderFloat("##distance", &levelSetDistance, 0, 100, "%.0f");
    if (ImGui::Button("Generate Feature Level Set")) {
        programState.generateLevelSet(levelSetDistance);
    }

    ImGui::End();
}

void GUI::createDatasetWidgets() {
    if (programState.dataset.readError) return;
    for (Attribute& attribute : programState.dataset.attributeDomain) {
        attributeWidgets.push_back({attribute, false, attribute.bounds.lower});
    }
}