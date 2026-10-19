#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <algorithm>
#include <filesystem>
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
#include "../meshTools/objMesh.hpp"
#include "../multiField/histogram.hpp"
#include "../multiField/multiField.hpp"

#ifndef __EMSCRIPTEN__
namespace {

enum class FileDialogStatus { Selected, Cancelled, Unavailable };

struct FileDialogOutcome {
    FileDialogStatus status;
    std::string path;
};

FileDialogOutcome runFileDialog(const std::vector<const char*>& args) {
    int pipeEnds[2] = {-1, -1};
    if (pipe(pipeEnds) != 0) return {FileDialogStatus::Unavailable, {}};

    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions, pipeEnds[1], STDOUT_FILENO);
    posix_spawn_file_actions_addclose(&actions, pipeEnds[0]);
    posix_spawn_file_actions_addclose(&actions, pipeEnds[1]);

    std::vector<char*> argv;
    argv.reserve(args.size() + 1);
    for (const char* arg : args) argv.push_back(const_cast<char*>(arg));
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
    const std::string filename = "--filename=" + current;
    std::vector<const char*> zenity = {
        "zenity", "--file-selection",
        "--title=Load dataset",
        "--file-filter=Dataset config | *.txt",
        "--file-filter=All files | *",
    };
    if (!current.empty()) zenity.push_back(filename.c_str());
    const FileDialogOutcome zenityResult = runFileDialog(zenity);
    if (zenityResult.status != FileDialogStatus::Unavailable) return zenityResult;

    const std::string start = current.empty() ? std::string(".") : current;
    const std::vector<const char*> kdialog = {
        "kdialog", "--getopenfilename",
        start.c_str(),
        "*.txt|Dataset config\n*|All files",
    };
    return runFileDialog(kdialog);
}

FileDialogOutcome pickMeshFile() {
    const std::vector<const char*> zenity = {
        "zenity", "--file-selection",
        "--title=Import OBJ",
        "--file-filter=Wavefront OBJ | *.obj",
        "--file-filter=All files | *",
    };
    const FileDialogOutcome zenityResult = runFileDialog(zenity);
    if (zenityResult.status != FileDialogStatus::Unavailable) return zenityResult;
    const std::vector<const char*> kdialog = {
        "kdialog", "--getopenfilename", ".",
        "*.obj|Wavefront OBJ\n*|All files",
    };
    return runFileDialog(kdialog);
}

FileDialogOutcome pickSaveMesh(const std::string& suggested) {
    const std::string filename = "--filename=" + suggested;
    const std::vector<const char*> zenity = {
        "zenity", "--file-selection", "--save", "--confirm-overwrite",
        "--title=Export OBJ",
        filename.c_str(),
        "--file-filter=Wavefront OBJ | *.obj",
        "--file-filter=All files | *",
    };
    const FileDialogOutcome zenityResult = runFileDialog(zenity);
    if (zenityResult.status != FileDialogStatus::Unavailable) return zenityResult;
    const std::vector<const char*> kdialog = {
        "kdialog", "--getsavefilename", suggested.c_str(),
        "*.obj|Wavefront OBJ\n*|All files",
    };
    return runFileDialog(kdialog);
}

std::string uniqueMeshId(const SessionState& session, std::string stem) {
    if (stem.empty()) stem = "mesh";
    auto taken = [&](const std::string& id) {
        for (const FeatureLevelSet& levelSet : session.levelSets)
            if (levelSet.id == id) return true;
        return false;
    };
    if (!taken(stem)) return stem;
    for (int suffix = 2; ; ++suffix) {
        const std::string candidate = stem + " " + std::to_string(suffix);
        if (!taken(candidate)) return candidate;
    }
}

}
#endif

namespace {

constexpr int kAttributeHistogramBins = 64;
constexpr float kAttributeHistogramHeight = 40.f;

// The samples are already normalised onto [0, 1] when the bounds have a span,
// and the slider still runs over those bounds, so the bins line up with it.
// A zero span skips normalisation and the samples stay at the bound.
FloatRange histogramRange(const Attribute& attribute) {
    if (attribute.bounds.upper > attribute.bounds.lower) return {0.f, 1.f};
    return attribute.bounds;
}

void plotAttributeHistogram(AttributeWidget& widget) {
    if (widget.histogram.empty()) return;

    const auto countAt = [](void* data, int index) {
        return static_cast<float>(static_cast<const int*>(data)[index]);
    };
    const int peak = std::max(1, std::ranges::max(widget.histogram));
    const std::string label = "##histogram" + std::to_string(widget.attributeIndex);
    ImGui::PlotHistogram(
        label.c_str(),
        countAt,
        widget.histogram.data(),
        static_cast<int>(widget.histogram.size()),
        0,
        nullptr,
        0.f,
        static_cast<float>(peak),
        ImVec2(0.f, kAttributeHistogramHeight));
}

}

void GUI::render() {


    // ImGui UI on the left 30%
    ImGui::SetNextWindowPos(ImVec2(0, 0)); // Position at top-left corner
    ImGui::SetNextWindowSize(ImVec2(rendering.windowWidth * (1-rendering.renderWidthPercentage), rendering.windowHeight)); // 30% width, full height
    ImGui::Begin("Controls");
    ImGui::SetWindowFontScale(1.f);

    if (session.pollDatasetLoad()) createDatasetWidgets();
    session.pollLevelSet();

    ImGui::BeginDisabled(!session.canUndo());
    if (ImGui::Button("Undo")) session.undo();
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!session.canRedo());
    if (ImGui::Button("Redo")) session.redo();
    ImGui::EndDisabled();

    if (session.activeLoad) {
        ImGui::ProgressBar(session.activeLoad->progress());
        if (ImGui::Button("Cancel load")) session.activeLoad->cancel();
    } else if (ImGui::Button("Load Dataset")) {
        fileDialogError.clear();
        session.beginDatasetLoad(std::string(inputTextBuffer));
    }
#ifndef __EMSCRIPTEN__
    ImGui::SameLine();
    const bool busy = session.activeLoad.has_value() || session.activeLevelSet.has_value();
    if (busy) ImGui::BeginDisabled();
    if (ImGui::Button("Browse")) {
        const FileDialogOutcome picked = pickConfigFile(inputTextBuffer);
        if (picked.status == FileDialogStatus::Selected) {
            if (picked.path.size() >= sizeof(inputTextBuffer)) {
                fileDialogError = "Selected path is too long.";
            } else {
                std::snprintf(inputTextBuffer, sizeof(inputTextBuffer), "%s", picked.path.c_str());
                fileDialogError.clear();
                session.beginDatasetLoad(inputTextBuffer);
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
#ifndef __EMSCRIPTEN__
    if (busy) ImGui::BeginDisabled();
    if (ImGui::Button("Import OBJ")) {
        const FileDialogOutcome picked = pickMeshFile();
        if (picked.status == FileDialogStatus::Selected) {
            ObjMesh mesh = importOBJ(picked.path);
            if (!mesh.ok()) {
                meshError = std::move(mesh.error);
            } else {
                meshError.clear();
                const std::string id = uniqueMeshId(session, std::filesystem::path(picked.path).stem().string());
                session.addLevelSet(FeatureLevelSet(
                    {}, std::move(mesh.surface), glm::vec3{1.f, 0.f, 0.f}, id, {}));
            }
        } else if (picked.status == FileDialogStatus::Unavailable) {
            meshError = "No file dialog is available.";
        }
    }
    if (busy) ImGui::EndDisabled();
    if (session.levelSets.empty() && !meshError.empty())
        ImGui::TextUnformatted(meshError.c_str());
#endif

    if (session.activeLevelSet) {
        ImGui::ProgressBar(session.activeLevelSet->progress());
        if (ImGui::Button("Cancel level set")) session.activeLevelSet->cancel();
    }

    if (!session.datasets.empty()) {
        ImGui::Text("Datasets:");
        int removeIndex = -1;
        for (int datasetIndex = 0; datasetIndex < static_cast<int>(session.datasets.size()); ++datasetIndex) {
            const std::shared_ptr<MultiField>& loaded = session.datasets[datasetIndex];
            const std::string name = loaded->ok() ? loaded->name : loaded->name + " (failed)";
            const std::string label = name + "##dataset" + std::to_string(datasetIndex);
            if (ImGui::RadioButton(label.c_str(), session.active == datasetIndex)) {
                if (session.setActiveDataset(datasetIndex)) createDatasetWidgets();
            }
            ImGui::SameLine();
            if (ImGui::SmallButton(("Remove##dataset" + std::to_string(datasetIndex)).c_str()))
                removeIndex = datasetIndex;
        }
        if (removeIndex >= 0 && session.removeDataset(removeIndex)) createDatasetWidgets();

        const std::shared_ptr<MultiField> active = session.activeDataset();
        if (active && !active->ok()) {
            ImGui::TextUnformatted(active->error.c_str());
        } else if (active) {
            ImGui::Text(" ");
            ImGui::Text("Define Trait:");
            for (auto &widget : attributeWidgets) {
                const Attribute& attribute = widget.attribute();
                ImGui::Text("%s: ", attribute.name.c_str());
                if (not widget.active) ImGui::BeginDisabled();
                ImGui::SliderFloat(("##slider" + attribute.name).c_str(), &widget.value, attribute.bounds.lower, attribute.bounds.upper, "%.7f");
                if (not widget.active) ImGui::EndDisabled();
                ImGui::SameLine();
                ImGui::Checkbox(("##checkbox" + attribute.name).c_str(), &widget.active);
                plotAttributeHistogram(widget);
            }
        }

        if (active) {
            if (ImGui::Button("Add attribute vertex")) {
                TraitPoint newVertex;
                for (auto &widget : attributeWidgets) {
                    if (widget.active) {
                        newVertex.values.push_back({&widget.attribute(), widget.value});
                    }
                }
                if (newVertex.values.size() > 0) session.addAttributeSpacePointToBuffer(newVertex);
            }

            if (session.attributeSpacePointsBuffer.size() != 0) {
                ImGui::Text(" ");
                ImGui::Text("Attribute Vertices: ");
                int removePoint = -1;
                int i = 0;
                for (auto &vertex : session.attributeSpacePointsBuffer) {
                    if (ImGui::Button(std::string("Remove Vertex: " + std::to_string(i)).c_str()))
                        removePoint = i;
                    ImGui::SameLine();
                    if (ImGui::Button(std::string("Load Vertex: " + std::to_string(i)).c_str())) {
                        for (auto &widget : attributeWidgets) {
                            bool foundMatch = false;
                            for (auto &value : vertex.values) {
                                if (widget.attribute().name == value.attribute->name) {
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
                if (removePoint >= 0) session.removeAttributeSpacePoint(removePoint);

                ImGui::Text("Normalised Euclidean distance for level set: ");
                ImGui::SliderFloat("##distance", &levelSetDistance, 0, 100, "%.4f");
                ImGui::Checkbox("Keep distance field", &keepDistanceField);
                ImGui::Text("Select level set colour: ");
                ImGui::ColorEdit3("##levelSetColour", &levelSetColour.x);
                ImGui::Text("Level set name: ");
                ImGui::SameLine();
                ImGui::InputText("##levelSetId", levelSetIdBuffer, IM_ARRAYSIZE(levelSetIdBuffer));

                if (ImGui::Button("Generate Feature Level Set")) {
                    bool uniqueId = true;
                    for (auto &levelSet : session.levelSets) {
                        if (std::string(levelSetIdBuffer) == levelSet.id) {
                            uniqueId = false;
                            break;
                        }
                    }
                    if (not uniqueId) ImGui::Text("Level set id must be unique.");
                    else if (!session.activeLevelSet)
                        session.generateLevelSet(levelSetDistance/100, levelSetColour, std::string(levelSetIdBuffer), keepDistanceField);
                }
            }
        }
    }

    if (session.levelSets.size() == 0) {
        ImGui::End();
        return;
    }

    if (!meshError.empty()) ImGui::TextUnformatted(meshError.c_str());
    ImGui::Text(" ");
    ImGui::Text("Level Set Controls: ");
    int removeLevelSetIndex = -1;
    int i = 0;
    for (auto &levelSet : session.levelSets) {
        ImGui::Text("%s", levelSet.id.c_str());
        if (levelSet.dataset) {
            ImGui::SameLine();
            ImGui::TextDisabled("(%s)", levelSet.dataset->name.c_str());
        }
        ImGui::SameLine();
        ImGui::Checkbox((std::string("##show") + levelSet.id).c_str(), &levelSet.active);
#ifndef __EMSCRIPTEN__
        ImGui::SameLine();
        if (busy) ImGui::BeginDisabled();
        if (ImGui::Button((std::string("Export##") + levelSet.id).c_str())) {
            const FileDialogOutcome picked = pickSaveMesh(levelSet.id + ".obj");
            if (picked.status == FileDialogStatus::Selected) {
                if (exportOBJ(levelSet.surface, picked.path)) meshError.clear();
                else meshError = "Failed to write " + picked.path;
            } else if (picked.status == FileDialogStatus::Unavailable) {
                meshError = "No file dialog is available.";
            }
        }
        if (busy) ImGui::EndDisabled();
#endif
        ImGui::SameLine();
        if (ImGui::Button(std::string("Remove " + levelSet.id).c_str()))
            removeLevelSetIndex = i;
        ImGui::Text("Transparency: ");
        ImGui::SameLine();
        const float transparencyBefore = levelSet.transparency;
        ImGui::SliderFloat((std::string("##Transparency") + levelSet.id).c_str(), &levelSet.transparency, 0, 1, "%.4f");
        trackTransparencyEdit(i, transparencyBefore, levelSet.transparency);
        const glm::vec3 colourBefore = levelSet.colour;
        ImGui::ColorEdit3((std::string("##colour") + levelSet.id).c_str(), &levelSet.colour.x);
        trackColourEdit(i, colourBefore, levelSet.colour);
        if (levelSet.distanceField) {
            ImGui::Text("Distance: ");
            ImGui::SameLine();
            const float isoBefore = levelSet.isoValue;
            float shown = isoBefore * 100.f;
            if (levelSetEdit == LevelSetEdit::IsoValue && levelSetEditIndex == i)
                shown = levelSetEditShown;
            ImGui::SliderFloat((std::string("##iso") + levelSet.id).c_str(), &shown, 0.f, 100.f, "%.4f");
            if (ImGui::IsItemActive()) levelSetEditShown = shown;
            const float nextIso = shown / 100.f;
            // Rebuild while the drag is in progress. The undo step is still one command, recorded on release.
            if (nextIso != levelSet.isoValue && (ImGui::IsItemActive() || ImGui::IsItemDeactivated()))
                levelSet.reslice(nextIso);
            trackIsoEdit(i, isoBefore, nextIso);
        }
        ImGui::Checkbox((std::string("Show boundary vertices##") + levelSet.id).c_str(), &levelSet.showActiveInactivePixels);
        ImGui::Checkbox((std::string("Invert Normals##") + levelSet.id).c_str(), &levelSet.invertNormals);
        ImGui::Text(" ");
        i++;
    }
    if (removeLevelSetIndex >= 0) session.removeLevelSet(removeLevelSetIndex);

    ImGui::End();
}

void GUI::trackTransparencyEdit(int index, float before, float after) {
    if (ImGui::IsItemActivated()) {
        levelSetEdit = LevelSetEdit::Transparency;
        levelSetEditIndex = index;
        levelSetEditTransparency = before;
    }
    if (levelSetEdit != LevelSetEdit::Transparency || levelSetEditIndex != index) return;
    if (ImGui::IsItemDeactivatedAfterEdit())
        session.setLevelSetTransparency(index, levelSetEditTransparency, after);
    if (ImGui::IsItemDeactivated()) levelSetEdit = LevelSetEdit::None;
}

void GUI::trackIsoEdit(int index, float before, float after) {
    if (ImGui::IsItemActivated()) {
        levelSetEdit = LevelSetEdit::IsoValue;
        levelSetEditIndex = index;
        levelSetEditIso = before;
    }
    if (levelSetEdit != LevelSetEdit::IsoValue || levelSetEditIndex != index) return;
    if (ImGui::IsItemDeactivatedAfterEdit())
        session.setLevelSetIso(index, levelSetEditIso, after);
    if (ImGui::IsItemDeactivated()) levelSetEdit = LevelSetEdit::None;
}

void GUI::trackColourEdit(int index, glm::vec3 before, glm::vec3 after) {
    if (ImGui::IsItemActivated()) {
        levelSetEdit = LevelSetEdit::Colour;
        levelSetEditIndex = index;
        levelSetEditColour = before;
    }
    if (levelSetEdit != LevelSetEdit::Colour || levelSetEditIndex != index) return;
    if (ImGui::IsItemDeactivatedAfterEdit())
        session.setLevelSetColour(index, levelSetEditColour, after);
    if (ImGui::IsItemDeactivated()) levelSetEdit = LevelSetEdit::None;
}

void GUI::createDatasetWidgets() {
    attributeWidgets.clear();
    const std::shared_ptr<MultiField> field = session.activeDataset();
    if (!field || !field->ok()) return;
    for (int index = 0; index < static_cast<int>(field->attributeDomain.size()); ++index) {
        const Attribute& attribute = field->attributeDomain[index];
        attributeWidgets.push_back({
            field,
            index,
            false,
            attribute.bounds.lower,
            histogram(attribute.values, kAttributeHistogramBins, histogramRange(attribute)),
        });
    }
}

static float clipAxisExtent(const SessionState& session, int axis) {
    int extent = 1;
    const auto consider = [&](const MultiField* field) {
        if (field == nullptr) return;
        const int size = axis == 0 ? field->xVals : axis == 1 ? field->yVals : field->zVals;
        extent = std::max(extent, size);
    };
    if (const std::shared_ptr<MultiField> active = session.activeDataset()) consider(active.get());
    for (const FeatureLevelSet& levelSet : session.levelSets)
        if (levelSet.dataset) consider(levelSet.dataset.get());
    return static_cast<float>(std::max(extent - 1, 0));
}

void GUI::renderLightingControls() {
    // ImGui UI on the left 30%
    ImGui::SetNextWindowPos(ImVec2(rendering.windowWidth - 300.f, 0)); // Position at top-right corner
    ImGui::SetNextWindowSize(ImVec2(300, 640)); 

    ImGui::Begin("Rendering Parameters");
    ImGui::SetWindowFontScale(1.f);

    ImGui::Text("Ambient Light:");
    ImGui::SliderFloat("R##1", &rendering.ambientLightColour.x, 0, 1, "%.2f");
    ImGui::SliderFloat("G##1", &rendering.ambientLightColour.y, 0, 1, "%.2f");
    ImGui::SliderFloat("B##1", &rendering.ambientLightColour.z, 0, 1, "%.2f");
    ImGui::SliderFloat("Strength##1", &rendering.ambientLightStrength, 0, 1, "%.2f");

    ImGui::Text(" ");
    ImGui::Text("Diffuse Light:");
    ImGui::SliderFloat("R##2", &rendering.diffuseLightColour.x, 0, 1, "%.2f");
    ImGui::SliderFloat("G##2", &rendering.diffuseLightColour.y, 0, 1, "%.2f");
    ImGui::SliderFloat("B##2", &rendering.diffuseLightColour.z, 0, 1, "%.2f");
    ImGui::SliderFloat("Strength##2", &rendering.diffuseLightStrength, 0, 1, "%.2f");

    ImGui::Text(" ");
    ImGui::Text("Diffuse Light Direction:");
    ImGui::SliderFloat("X##1", &rendering.diffuseLightDirection.x, -1, 1, "%.2f");    
    ImGui::SliderFloat("Y##1", &rendering.diffuseLightDirection.y, -1, 1, "%.2f");    
    ImGui::SliderFloat("Z##1", &rendering.diffuseLightDirection.z, -1, 1, "%.2f");    

    ImGui::Text(" ");
    ImGui::Checkbox("Back Face Culling", &rendering.backFaceCulling);
#ifndef __EMSCRIPTEN__
    ImGui::Text(" ");
    ImGui::Checkbox("Draw Wireframes", &rendering.drawWireframe);
#endif
    ImGui::Text(" ");
    ImGui::Text("Clip plane:");
    const bool clipWasEnabled = rendering.clipEnabled;
    ImGui::Checkbox("Enable##clip", &rendering.clipEnabled);
    if (rendering.clipEnabled) {
        const int previousAxis = rendering.clipAxis;
        if (ImGui::RadioButton("X##clip", rendering.clipAxis == 0)) rendering.clipAxis = 0;
        ImGui::SameLine();
        if (ImGui::RadioButton("Y##clip", rendering.clipAxis == 1)) rendering.clipAxis = 1;
        ImGui::SameLine();
        if (ImGui::RadioButton("Z##clip", rendering.clipAxis == 2)) rendering.clipAxis = 2;
        const float upper = clipAxisExtent(session, rendering.clipAxis);
        if (rendering.clipAxis != previousAxis || (rendering.clipEnabled && !clipWasEnabled))
            rendering.clipPosition = upper * 0.5f;
        rendering.clipPosition = std::clamp(rendering.clipPosition, 0.f, upper);
        ImGui::SliderFloat("Position##clip", &rendering.clipPosition, 0.f, upper, "%.1f");
    }
    ImGui::Text(" ");
    ImGui::Text("Camera Controls:");
    ImGui::SliderFloat("SPEED##1", &rendering.cam.speed, 0, 250, "%.0f");    
    ImGui::SliderFloat("POS X##1", &rendering.cam.pos.x, -10000, 10000, "%.0f");    
    ImGui::SliderFloat("POS Y##1", &rendering.cam.pos.y, -10000, 10000, "%.0f");    
    ImGui::SliderFloat("POS Z##1", &rendering.cam.pos.z, -10000, 10000, "%.0f");   
    ImGui::SliderFloat("FRONT X##1", &rendering.cam.front.x, -1, 1, "%.4f");    
    ImGui::SliderFloat("FRONT Y##1", &rendering.cam.front.y, sin(-89), sin(89), "%.4f");    
    ImGui::SliderFloat("FRONT Z##1", &rendering.cam.front.z, -1, 1, "%.4f"); 
    ImGui::SliderFloat("FOV##1", &rendering.fov, 0, 90, "%.0f"); 
    ImGui::End();
}