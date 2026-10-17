#pragma once
#include "imgui.h"

#include <memory>
#include <string>

#include "../../main/state.hpp"
#include "../levelSets/sessionState.hpp"
#include "../multiField/multiField.hpp"

struct AttributeWidget {
    std::shared_ptr<MultiField> dataset;
    int attributeIndex = 0;
    bool active = false;
    float value = 0.f;

    Attribute& attribute() { return dataset->attributeDomain[attributeIndex]; }
};

class GUI {
private:
    SessionState& session;
    RenderState& rendering;
    // textbox state
    char inputTextBuffer[4096] = "dataset/timestep02/config.txt";
    std::string fileDialogError;
    std::string meshError;
    // attribute widgets
    std::vector<AttributeWidget> attributeWidgets;
    // level set slider values
    float levelSetDistance = 0.0f;
    glm::vec3 levelSetColour{1.f, 0.f, 0.f};
    char levelSetIdBuffer[256] = "0";
public:
    GUI(SessionState& session, RenderState& rendering) : session(session), rendering(rendering) {}
    void render();
    void renderLightingControls();
    void createDatasetWidgets();
};