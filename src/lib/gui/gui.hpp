#pragma once
#include "../../main/state.hpp" 

class GUI {
private:
    // ui state
    const ProgramState &programState;
    int counter = 0;
public:
    GUI(const ProgramState &state) : programState(state) {}
    void render();
};