#pragma once
#include "imgui.h"

#include <memory>
#include <string>
#include <vector>

#include "../../main/state.hpp"
#include "../levelSets/sessionState.hpp"
#include "../multiField/multiField.hpp"

struct AttributeWidget {
    std::shared_ptr<MultiField> dataset;
    int attributeIndex = 0;
    bool active = false;
    float value = 0.f;
    // Bin counts for the attribute's samples. Filled once when the widgets are built.
    std::vector<int> histogram;

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
    bool keepDistanceField = false;
    glm::vec3 levelSetColour{1.f, 0.f, 0.f};
    char levelSetIdBuffer[256] = "0";
    // One drag is one undo step. The start value is captured when the widget is pressed.
    enum class LevelSetEdit { None, Transparency, Colour, IsoValue };
    LevelSetEdit levelSetEdit = LevelSetEdit::None;
    int levelSetEditIndex = -1;
    float levelSetEditTransparency = 1.f;
    glm::vec3 levelSetEditColour{1.f, 0.f, 0.f};
    float levelSetEditIso = 0.f;
    // The distance slider's own position. Mouse-up does not write a new value, so this
    // has to survive until the drag ends or the slider snaps back to isoValue.
    float levelSetEditShown = 0.f;

    void trackTransparencyEdit(int index, float before, float after);
    void trackColourEdit(int index, glm::vec3 before, glm::vec3 after);
    void trackIsoEdit(int index, float before, float after);
public:
    GUI(SessionState& session, RenderState& rendering) : session(session), rendering(rendering) {}
    void render();
    void renderLightingControls();
    void createDatasetWidgets();
};