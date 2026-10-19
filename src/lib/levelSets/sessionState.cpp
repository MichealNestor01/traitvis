#include "sessionState.hpp"

#include <algorithm>
#include <memory>
#include <utility>

namespace {

struct AddTraitPointCommand : SessionCommand {
    TraitPoint point;
    int index = 0;
    bool placed = false;

    explicit AddTraitPointCommand(TraitPoint point) : point(std::move(point)) {}

    void redo(SessionState& session) override {
        if (!placed) {
            index = static_cast<int>(session.attributeSpacePointsBuffer.size());
            placed = true;
        }
        session.attributeSpacePointsBuffer.insert(session.attributeSpacePointsBuffer.begin() + index, point);
    }

    void undo(SessionState& session) override {
        session.attributeSpacePointsBuffer.erase(session.attributeSpacePointsBuffer.begin() + index);
    }

    [[nodiscard]] bool changesTraitPoints() const override { return true; }
};

struct RemoveTraitPointCommand : SessionCommand {
    int index;
    TraitPoint point;

    explicit RemoveTraitPointCommand(int index) : index(index) {}

    void redo(SessionState& session) override {
        point = session.attributeSpacePointsBuffer[static_cast<std::size_t>(index)];
        session.attributeSpacePointsBuffer.erase(session.attributeSpacePointsBuffer.begin() + index);
    }

    void undo(SessionState& session) override {
        session.attributeSpacePointsBuffer.insert(session.attributeSpacePointsBuffer.begin() + index, point);
    }

    [[nodiscard]] bool changesTraitPoints() const override { return true; }
};

// A level set owns a GPU mesh, so it is moved between the session and the command
// rather than copied. `held` stores it while it is not in the session.
struct AddLevelSetCommand : SessionCommand {
    int index = 0;
    bool placed = false;
    std::optional<FeatureLevelSet> held;

    explicit AddLevelSetCommand(FeatureLevelSet levelSet) : held(std::move(levelSet)) {}

    void redo(SessionState& session) override {
        if (!placed) {
            index = static_cast<int>(session.levelSets.size());
            placed = true;
        }
        session.levelSets.insert(session.levelSets.begin() + index, std::move(*held));
        held.reset();
    }

    void undo(SessionState& session) override {
        held = std::move(session.levelSets[static_cast<std::size_t>(index)]);
        session.levelSets.erase(session.levelSets.begin() + index);
    }
};

struct RemoveLevelSetCommand : SessionCommand {
    int index;
    std::optional<FeatureLevelSet> held;

    explicit RemoveLevelSetCommand(int index) : index(index) {}

    void redo(SessionState& session) override {
        held = std::move(session.levelSets[static_cast<std::size_t>(index)]);
        session.levelSets.erase(session.levelSets.begin() + index);
    }

    void undo(SessionState& session) override {
        session.levelSets.insert(session.levelSets.begin() + index, std::move(*held));
        held.reset();
    }
};

struct SetLevelSetColourCommand : SessionCommand {
    int index;
    glm::vec3 before;
    glm::vec3 after;

    SetLevelSetColourCommand(int index, glm::vec3 before, glm::vec3 after)
        : index(index), before(before), after(after) {}

    void redo(SessionState& session) override { session.levelSets[static_cast<std::size_t>(index)].colour = after; }
    void undo(SessionState& session) override { session.levelSets[static_cast<std::size_t>(index)].colour = before; }
};

struct SetLevelSetTransparencyCommand : SessionCommand {
    int index;
    float before;
    float after;

    SetLevelSetTransparencyCommand(int index, float before, float after)
        : index(index), before(before), after(after) {}

    void redo(SessionState& session) override { session.levelSets[static_cast<std::size_t>(index)].transparency = after; }
    void undo(SessionState& session) override { session.levelSets[static_cast<std::size_t>(index)].transparency = before; }
};

void eraseTraitCommands(std::vector<std::unique_ptr<SessionCommand>>& stack) {
    std::erase_if(stack, [](const std::unique_ptr<SessionCommand>& command) {
        return command->changesTraitPoints();
    });
}

}

std::shared_ptr<MultiField> SessionState::activeDataset() const {
    if (active < 0 || active >= static_cast<int>(datasets.size())) return nullptr;
    return datasets[active];
}

void SessionState::addDataset(std::shared_ptr<MultiField> dataset) {
    datasets.push_back(std::move(dataset));
    active = static_cast<int>(datasets.size()) - 1;
    clearTraitPoints();
}

bool SessionState::setActiveDataset(int index) {
    if (index < 0 || index >= static_cast<int>(datasets.size()) || index == active) return false;
    active = index;
    clearTraitPoints();
    return true;
}

bool SessionState::removeDataset(int index) {
    if (index < 0 || index >= static_cast<int>(datasets.size())) return false;
    const std::shared_ptr<MultiField> previous = activeDataset();
    datasets.erase(datasets.begin() + index);
    if (datasets.empty()) {
        active = -1;
    } else if (active == index) {
        active = std::min(index, static_cast<int>(datasets.size()) - 1);
    } else if (active > index) {
        --active;
    }
    const bool changed = activeDataset() != previous;
    if (changed) clearTraitPoints();
    return changed;
}

void SessionState::addAttributeSpacePointToBuffer(TraitPoint point) {
    execute(std::make_unique<AddTraitPointCommand>(std::move(point)));
}

void SessionState::removeAttributeSpacePoint(int index) {
    if (index < 0 || index >= static_cast<int>(attributeSpacePointsBuffer.size())) return;
    execute(std::make_unique<RemoveTraitPointCommand>(index));
}

void SessionState::generateLevelSet(std::vector<TraitPoint> points, float euclideanDistance, glm::vec3 colour, std::string id) {
    const std::shared_ptr<MultiField> field = activeDataset();
    if (!field || !field->ok()) return;
    addLevelSet(FeatureLevelSet(std::move(points), std::shared_ptr<const MultiField>(field), euclideanDistance, colour, std::move(id)));
}

void SessionState::generateLevelSet(float euclideanDistance, glm::vec3 colour, std::string id) {
    const std::shared_ptr<MultiField> field = activeDataset();
    if (activeLevelSet || !field || !field->ok()) return;
    activeLevelSet.emplace(attributeSpacePointsBuffer, std::shared_ptr<const MultiField>(field), euclideanDistance, colour, std::move(id));
    activeLevelSet->start();
}

void SessionState::addLevelSet(FeatureLevelSet levelSet) {
    execute(std::make_unique<AddLevelSetCommand>(std::move(levelSet)));
}

void SessionState::removeLevelSet(int index) {
    if (index < 0 || index >= static_cast<int>(levelSets.size())) return;
    execute(std::make_unique<RemoveLevelSetCommand>(index));
}

void SessionState::setLevelSetColour(int index, glm::vec3 before, glm::vec3 after) {
    if (index < 0 || index >= static_cast<int>(levelSets.size()) || before == after) return;
    execute(std::make_unique<SetLevelSetColourCommand>(index, before, after));
}

void SessionState::setLevelSetTransparency(int index, float before, float after) {
    if (index < 0 || index >= static_cast<int>(levelSets.size()) || before == after) return;
    execute(std::make_unique<SetLevelSetTransparencyCommand>(index, before, after));
}

void SessionState::undo() {
    if (undoStack.empty()) return;
    std::unique_ptr<SessionCommand> command = std::move(undoStack.back());
    undoStack.pop_back();
    command->undo(*this);
    redoStack.push_back(std::move(command));
}

void SessionState::redo() {
    if (redoStack.empty()) return;
    std::unique_ptr<SessionCommand> command = std::move(redoStack.back());
    redoStack.pop_back();
    command->redo(*this);
    undoStack.push_back(std::move(command));
}

void SessionState::clearHistory() {
    undoStack.clear();
    redoStack.clear();
}

void SessionState::beginDatasetLoad(std::string path) {
    if (activeLoad) return;
    activeLoad.emplace(std::move(path));
    activeLoad->start();
}

bool SessionState::pollDatasetLoad() {
#if defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)
    if (activeLoad && !activeLoad->runsOnWorker() && !activeLoad->finished())
        activeLoad->stepOnMainThread();
#endif
    if (!activeLoad || !activeLoad->finished()) return false;
    std::optional<MultiField> loaded = activeLoad->take();
    activeLoad.reset();
    if (!loaded) return false;
    addDataset(std::make_shared<MultiField>(std::move(*loaded)));
    return true;
}

void SessionState::pollLevelSet() {
#if defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)
    if (activeLevelSet && !activeLevelSet->runsOnWorker() && !activeLevelSet->finished())
        activeLevelSet->stepOnMainThread(1);
#endif
    if (!activeLevelSet || !activeLevelSet->finished()) return;
    if (std::optional<FeatureLevelSet> made = activeLevelSet->take())
        addLevelSet(std::move(*made));
    activeLevelSet.reset();
}

void SessionState::execute(std::unique_ptr<SessionCommand> command) {
    command->redo(*this);
    undoStack.push_back(std::move(command));
    redoStack.clear();
}

void SessionState::clearTraitPoints() {
    attributeSpacePointsBuffer.clear();
    eraseTraitCommands(undoStack);
    eraseTraitCommands(redoStack);
}
