#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <string>

#ifndef __EMSCRIPTEN__
#include <cstdio>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
extern char** environ;
#endif

#include "gui.hpp"
#include "../../main/state.hpp" 
#include "../multiField/multiField.hpp"

#ifndef __EMSCRIPTEN__
namespace {

enum class FileDialogStatus { Selected, Cancelled, Unavailable };

struct FileDialogOutcome {
    FileDialogStatus status;
    std::string path;
};

FileDialogOutcome runFileDialog(const std::vector<std::string>& args) {
    int pipeEnds[2] = {-1, -1};
    if (pipe(pipeEnds) != 0) return {FileDialogStatus::Unavailable, {}};

    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions, pipeEnds[1], STDOUT_FILENO);
    posix_spawn_file_actions_addclose(&actions, pipeEnds[0]);
    posix_spawn_file_actions_addclose(&actions, pipeEnds[1]);

    std::vector<char*> argv;
    argv.reserve(args.size() + 1);
    for (const std::string& arg : args) argv.push_back(const_cast<char*>(arg.c_str()));
    argv.push_back(nullptr);

    pid_t pid = 0;
    const int spawned = posix_spawnp(&pid, argv[0], &actions, nullptr, argv.data(), environ);
    posix_spawn_file_actions_destroy(&actions);
    close(pipeEnds[1]);
    if (spawned != 0) {
        close(pipeEnds[0]);
        return {FileDialogStatus::Unavailable, {}};
    }

    std::string output;
    char buffer[512];
    for (ssize_t count = 0; (count = read(pipeEnds[0], buffer, sizeof(buffer))) > 0; )
        output.append(buffer, static_cast<std::size_t>(count));
    close(pipeEnds[0]);

    int status = 0;
    waitpid(pid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) == 127) return {FileDialogStatus::Unavailable, {}};
    if (WEXITSTATUS(status) != 0) return {FileDialogStatus::Cancelled, {}};
    while (!output.empty() && (output.back() == '\n' || output.back() == '\r')) output.pop_back();
    if (output.empty()) return {FileDialogStatus::Cancelled, {}};
    return {FileDialogStatus::Selected, std::move(output)};
}

FileDialogOutcome pickConfigFile(const std::string& current) {
    std::vector<std::string> zenity = {
        "zenity", "--file-selection",
        "--title=Load dataset",
        "--file-filter=Dataset config | *.txt",
        "--file-filter=All files | *",
    };
    if (!current.empty()) zenity.push_back("--filename=" + current);
    const FileDialogOutcome zenityResult = runFileDialog(zenity);
    if (zenityResult.status != FileDialogStatus::Unavailable) return zenityResult;

    return runFileDialog({
        "kdialog", "--getopenfilename",
        current.empty() ? std::string(".") : current,
        "*.txt|Dataset config\n*|All files",
    });
}

}
#endif

void GUI::render() {


    // ImGui UI on the left 30%
    ImGui::SetNextWindowPos(ImVec2(0, 0)); // Position at top-left corner
    ImGui::SetNextWindowSize(ImVec2(programState.windowWidth * (1-programState.renderWidthPercentage), programState.windowHeight)); // 30% width, full height
    ImGui::Begin("Controls");
    ImGui::SetWindowFontScale(1.f);

    if (programState.pollDatasetLoad()) createDatasetWidgets();
    programState.pollLevelSet();

    if (programState.activeLoad) {
        ImGui::ProgressBar(programState.activeLoad->progress());
        if (ImGui::Button("Cancel load")) programState.activeLoad->cancel();
    } else if (ImGui::Button("Load Dataset")) {
        fileDialogError.clear();
        programState.beginDatasetLoad(std::string(inputTextBuffer));
    }
#ifndef __EMSCRIPTEN__
    ImGui::SameLine();
    const bool busy = programState.activeLoad.has_value() || programState.activeLevelSet.has_value();
    if (busy) ImGui::BeginDisabled();
    if (ImGui::Button("Browse")) {
        const FileDialogOutcome picked = pickConfigFile(inputTextBuffer);
        if (picked.status == FileDialogStatus::Selected) {
            if (picked.path.size() >= sizeof(inputTextBuffer)) {
                fileDialogError = "Selected path is too long.";
            } else {
                std::snprintf(inputTextBuffer, sizeof(inputTextBuffer), "%s", picked.path.c_str());
                fileDialogError.clear();
                programState.beginDatasetLoad(inputTextBuffer);
            }
        } else if (picked.status == FileDialogStatus::Unavailable) {
            fileDialogError = "No file dialog is available. Type the config path instead.";
        }
    }
    if (busy) ImGui::EndDisabled();
#endif
    ImGui::SameLine();
    ImGui::InputText("##DatasetPath", inputTextBuffer, IM_ARRAYSIZE(inputTextBuffer));
    if (!fileDialogError.empty()) ImGui::TextUnformatted(fileDialogError.c_str());

    if (programState.activeLevelSet) {
        ImGui::ProgressBar(programState.activeLevelSet->progress());
        if (ImGui::Button("Cancel level set")) programState.activeLevelSet->cancel();
    }

    // Display the text from the input box underneath it)
    if (not programState.loadedDataset) {
        ImGui::End();
        return;
    }

    if (!programState.dataset->ok()) {
        ImGui::TextUnformatted(programState.dataset->error.c_str());
    } else {
        std::string message = "Loaded dataset: " + programState.dataset->name;
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
        TraitPoint newVertex;
        for (auto &widget : attributeWidgets) {
            if (widget.active) {
                newVertex.values.push_back({&widget.attribute, widget.value});
            }
        }
        if (newVertex.values.size() > 0) programState.addAttributeSpacePointToBuffer(newVertex);
    }

    if (programState.attributeSpacePointsBuffer.size() != 0) {
        ImGui::Text(" ");
        ImGui::Text("Attribute Vertices: ");
        int i = 0;
        for (auto &vertex : programState.attributeSpacePointsBuffer) {
            if (ImGui::Button(std::string("Remove Vertex: " + std::to_string(i)).c_str())) {
                programState.attributeSpacePointsBuffer.erase(programState.attributeSpacePointsBuffer.begin() + i);
            }
            ImGui::SameLine();
            if (ImGui::Button(std::string("Load Vertex: " + std::to_string(i)).c_str())) {
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
        ImGui::SliderFloat("##distance", &levelSetDistance, 0, 100, "%.4f");
        ImGui::Text("Select level set colour: ");
        ImGui::ColorEdit3("##levelSetColour", &levelSetColour.x);
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
            if (not uniqueId) ImGui::Text("Level set id must be unique.");
            else if (!programState.activeLevelSet)
                programState.generateLevelSet(levelSetDistance/100, levelSetColour, std::string(levelSetIdBuffer));
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
        ImGui::SliderFloat((std::string("##Transparency") + levelSet.id).c_str(), &levelSet.transparency, 0, 1, "%.4f");
        ImGui::ColorEdit3((std::string("##colour") + levelSet.id).c_str(), &levelSet.colour.x);
        ImGui::Checkbox((std::string("Show boundary vertices##") + levelSet.id).c_str(), &levelSet.showActiveInactivePixels);
        ImGui::Checkbox((std::string("Invert Normals##") + levelSet.id).c_str(), &levelSet.invertNormals);
        ImGui::Text(" ");
        i++;
    }

    ImGui::End();
}

void GUI::createDatasetWidgets() {
    attributeWidgets.clear();
    if (!programState.dataset || !programState.dataset->ok()) return;
    for (Attribute& attribute : programState.dataset->attributeDomain)
        attributeWidgets.push_back({attribute, false, attribute.bounds.lower});
}

void GUI::renderLightingControls() {
    // ImGui UI on the left 30%
    ImGui::SetNextWindowPos(ImVec2(programState.windowWidth - 300.f, 0)); // Position at top-right corner
    ImGui::SetNextWindowSize(ImVec2(300, 530)); 

    ImGui::Begin("Rendering Parameters");
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

    ImGui::Text(" ");
    ImGui::Text("Diffuse Light Direction:");
    ImGui::SliderFloat("X##1", &programState.diffuseLightDirection.x, -1, 1, "%.2f");    
    ImGui::SliderFloat("Y##1", &programState.diffuseLightDirection.y, -1, 1, "%.2f");    
    ImGui::SliderFloat("Z##1", &programState.diffuseLightDirection.z, -1, 1, "%.2f");    

    ImGui::Text(" ");
    ImGui::Checkbox("Back Face Culling", &programState.backFaceCulling);
#ifndef __EMSCRIPTEN__
    ImGui::Text(" ");
    ImGui::Checkbox("Draw Wireframes", &programState.drawWireframe);
#endif
    ImGui::Text(" ");
    ImGui::Text("Camera Controls:");
    ImGui::SliderFloat("SPEED##1", &programState.cam.speed, 0, 250, "%.0f");    
    ImGui::SliderFloat("POS X##1", &programState.cam.pos.x, -10000, 10000, "%.0f");    
    ImGui::SliderFloat("POS Y##1", &programState.cam.pos.y, -10000, 10000, "%.0f");    
    ImGui::SliderFloat("POS Z##1", &programState.cam.pos.z, -10000, 10000, "%.0f");   
    ImGui::SliderFloat("FRONT X##1", &programState.cam.front.x, -1, 1, "%.4f");    
    ImGui::SliderFloat("FRONT Y##1", &programState.cam.front.y, sin(-89), sin(89), "%.4f");    
    ImGui::SliderFloat("FRONT Z##1", &programState.cam.front.z, -1, 1, "%.4f"); 
    ImGui::SliderFloat("FOV##1", &programState.fov, 0, 90, "%.0f"); 
    ImGui::End();
}