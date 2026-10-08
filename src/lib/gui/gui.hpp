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
    char inputTextBuffer[256] = "";
    // attribute widgets
    std::vector<AttributeWidget> attributeWidgets;
    // level set slider values
    float levelSetDistance = 0.0f;
    float levelSetRed = 0.0f;
    float levelSetBlue = 0.0f;
    float levelSetGreen = 0.0f;
    char levelSetIdBuffer[256] = "0";
public:
    GUI(ProgramState &state) : programState(state) {}
    void render();
    void renderLightingControls();
    void createDatasetWidgets();
};