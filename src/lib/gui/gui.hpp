#pragma once
#include "imgui.h"

#include "../../main/state.hpp" 
#include "../multiField/multiField.hpp"

struct AttributeWidget {
    Attribute & attribute;
    bool active;
    float value;
};

class GUI {
private:
    ProgramState &programState;
    // textbox state
    char inputTextBuffer[256] = "dataset/timestep02/config.txt";
    // attribute widgets
    std::vector<AttributeWidget> attributeWidgets;
    // level set slider values
    float levelSetDistance = 0.0f;
    glm::vec3 levelSetColour{1.f, 0.f, 0.f};
    char levelSetIdBuffer[256] = "0";
public:
    GUI(ProgramState &state) : programState(state) {}
    void render();
    void renderLightingControls();
    void createDatasetWidgets();
};