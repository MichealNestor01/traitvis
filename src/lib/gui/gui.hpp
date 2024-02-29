#pragma once
#include <string>
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
    char inputTextBuffer[256] = "/home/michealnestor/University/final-project/dataset/timestep02/config.txt";
    std::string displayText = "";
    // attribute widgets
    std::vector<AttributeWidget> attributeWidgets;
    // level set distance
    float levelSetDistance = 0.0f;
public:
    GUI(ProgramState &state) : programState(state) {}
    void render();
    void createDatasetWidgets();
}; 