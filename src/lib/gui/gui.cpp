#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "gui.hpp"
#include "../../main/state.hpp" 
#include "../multiField/multiField.hpp"


void GUI::render() {


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
    ImGui::InputText("##DatasetPath", inputTextBuffer, IM_ARRAYSIZE(inputTextBuffer));

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
                newVertex.values.push_back({&widget.attribute, widget.value});
            }
        }
        if (newVertex.values.size() > 0) programState.addAttributeSpaceVertexToBuffer(newVertex);
    }

    if (programState.attributeSpaceVerticesBuffer.size() != 0) {
        ImGui::Text(" ");
        ImGui::Text("Attribute Vertices: ");
        int i = 0;
        for (auto &vertex : programState.attributeSpaceVerticesBuffer) {
            if (ImGui::Button(std::string("Remove Attribute Vertex: " + std::to_string(i)).c_str())) {
                programState.attributeSpaceVerticesBuffer.erase(programState.attributeSpaceVerticesBuffer.begin() + i);
            }
            ImGui::SameLine();
            if (ImGui::Button(std::string("Load Attribute Vertex: " + std::to_string(i)).c_str())) {
                for (auto &widget : attributeWidgets) {
                    bool foundMatch = false;
                    for (auto &value : vertex.values) {
                        if (widget.attribute.name == value.attribute->name) {
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

        ImGui::Text("Normalised Euclidean distance for level set: ");
        ImGui::SliderFloat("##distance", &levelSetDistance, 0, 100, "%.2f");
        ImGui::Text("Select level set colour: ");
        ImGui::SliderFloat("R", &levelSetRed, 0, 255, "%.0f");
        ImGui::SliderFloat("G", &levelSetGreen, 0, 255, "%.0f");
        ImGui::SliderFloat("B", &levelSetBlue, 0, 255, "%.0f");
        ImGui::Text("Level set name: ");
        ImGui::SameLine(); 
        ImGui::InputText("##levelSetId", levelSetIdBuffer, IM_ARRAYSIZE(levelSetIdBuffer));

        if (ImGui::Button("Generate Feature Level Set")) {
            // check id is unique
            bool uniqueId = true;
            for (auto &levelSet : programState.levelSets) {
                if (std::string(levelSetIdBuffer) == levelSet.id) {
                    uniqueId = false;
                    break;
                }
            }
            if (not uniqueId) {
                ImGui::Text("Level set id must be unique.");
            } else {
                FeatureLevelSet newLevelSet(programState.attributeSpaceVerticesBuffer, programState.dataset, levelSetDistance/100, glm::vec3(levelSetRed/255.f, levelSetGreen/255.f, levelSetBlue/255.f), std::string(levelSetIdBuffer));
                programState.levelSets.push_back(newLevelSet);
            }
        }
    }

    if (programState.levelSets.size() == 0) {
        ImGui::End();
        return;
    }

    ImGui::Text(" ");
    ImGui::Text("Level Set Controls: ");
    int i = 0;
    for (auto &levelSet : programState.levelSets) {
        ImGui::Text(levelSet.id.c_str());
        ImGui::SameLine();
        ImGui::Checkbox((std::string("##show") + levelSet.id).c_str(), &levelSet.active);
        ImGui::SameLine();
        if (ImGui::Button(std::string("Remove " + levelSet.id).c_str())) 
            programState.levelSets.erase(programState.levelSets.begin() + i);
        ImGui::Text("Transparency: ");
        ImGui::SameLine();
        ImGui::SliderFloat((std::string("##Transparency") + levelSet.id).c_str(), &levelSet.transparency, 0, 1, "%.02f");
        ImGui::Text("Render depth: ");
        ImGui::SameLine();
        ImGui::SliderFloat((std::string("##Render Depth") + levelSet.id).c_str(), &levelSet.renderDepth, 0, 1, "%.02f");
        ImGui::Checkbox((std::string("Show boundary vertices##") + levelSet.id).c_str(), &levelSet.showActiveInactivePixels);
        ImGui::Text(" ");
        i++;
    }

    ImGui::End();
}

void GUI::createDatasetWidgets() {
    if (programState.dataset.readError) return;
    for (Attribute& attribute : programState.dataset.attributeDomain) 
        attributeWidgets.push_back({attribute, false, attribute.bounds.lower});
}

void GUI::renderLightingControls() {
    // ImGui UI on the left 30%
    ImGui::SetNextWindowPos(ImVec2(programState.windowWidth - 300.f, 0)); // Position at top-right corner
    ImGui::SetNextWindowSize(ImVec2(300, 530)); 

    ImGui::Begin("Lighting");
    ImGui::SetWindowFontScale(1.f);

    ImGui::Text("Ambient Light:");
    ImGui::SliderFloat("R##1", &programState.ambientLightColour.x, 0, 1, "%.2f");
    ImGui::SliderFloat("G##1", &programState.ambientLightColour.y, 0, 1, "%.2f");
    ImGui::SliderFloat("B##1", &programState.ambientLightColour.z, 0, 1, "%.2f");
    ImGui::SliderFloat("Strength##1", &programState.ambientLightStrength, 0, 1, "%.2f");

    ImGui::Text(" ");
    ImGui::Text("Diffuse Light:");
    ImGui::SliderFloat("R##2", &programState.diffuseLightColour.x, 0, 1, "%.2f");
    ImGui::SliderFloat("G##2", &programState.diffuseLightColour.y, 0, 1, "%.2f");
    ImGui::SliderFloat("B##2", &programState.diffuseLightColour.z, 0, 1, "%.2f");
    ImGui::SliderFloat("Strength##2", &programState.diffuseLightStrength, 0, 1, "%.2f");
    ImGui::Text("Diffuse Light Direction:");
    ImGui::SliderFloat("X##1", &programState.diffuseLightDirection.x, -1, 1, "%.2f");    
    ImGui::SliderFloat("Y##1", &programState.diffuseLightDirection.y, -1, 1, "%.2f");    
    ImGui::SliderFloat("Z##1", &programState.diffuseLightDirection.z, -1, 1, "%.2f");    

    ImGui::End();
}