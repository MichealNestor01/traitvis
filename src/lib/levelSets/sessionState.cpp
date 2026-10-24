#include "sessionState.hpp"
#include "savedLevelSet.hpp"

#include "../parsingData/configParser.hpp"

#include <algorithm>
#include <chrono>
#include <memory>
#include <unordered_set>
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

    void retainedLevelSets(std::vector<const FeatureLevelSet*>& out) const override {
        if (held) out.push_back(&*held);
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

    void retainedLevelSets(std::vector<const FeatureLevelSet*>& out) const override {
        if (held) out.push_back(&*held);
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

struct SetLevelSetIsoCommand : SessionCommand {
    int index;
    float before;
    float after;

    SetLevelSetIsoCommand(int index, float before, float after)
        : index(index), before(before), after(after) {}

    void redo(SessionState& session) override { session.levelSets[static_cast<std::size_t>(index)].reslice(after); }
    void undo(SessionState& session) override { session.levelSets[static_cast<std::size_t>(index)].reslice(before); }
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

void SessionState::generateLevelSet(std::vector<TraitPoint> points, float euclideanDistance, glm::vec3 colour, std::string id, bool keepField, bool closeAtMissingData) {
    const std::shared_ptr<MultiField> field = activeDataset();
    if (!field || !field->ok()) return;
    addLevelSet(FeatureLevelSet(std::move(points), std::shared_ptr<const MultiField>(field), euclideanDistance, colour, std::move(id), keepField, closeAtMissingData));
}

void SessionState::generateLevelSet(float euclideanDistance, glm::vec3 colour, std::string id, bool keepField, bool closeAtMissingData) {
    const std::shared_ptr<MultiField> field = activeDataset();
    if (activeLevelSet || !field || !field->ok()) return;
    const std::shared_ptr<const MultiField> dataset(field);

    if (gpuDistanceField) {
        // Samples are already normalised. Targets in the buffer are still in the slider's
        // units, and the job normalises its own copy for the level set. The shader has to
        // see those same normalised targets, so this copy is separate from the one the job keeps.
        std::vector<TraitPoint> normalisedPoints = attributeSpacePointsBuffer;
        for (TraitPoint& point : normalisedPoints) point.normalise();
        if (std::optional<ScalarField> computed = gpuDistanceField(normalisedPoints, *field)) {
            activeLevelSet.emplace(attributeSpacePointsBuffer, dataset, euclideanDistance, colour, std::move(id), keepField, std::move(*computed), closeAtMissingData);
            activeLevelSet->start();
            return;
        }
    }

    activeLevelSet.emplace(attributeSpacePointsBuffer, dataset, euclideanDistance, colour, std::move(id), keepField, closeAtMissingData);
    activeLevelSet->start();
}

namespace {

std::string uniqueLevelSetId(const SessionState& session, std::string stem) {
    if (stem.empty()) stem = "level set";
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

void SessionState::addLevelSet(FeatureLevelSet levelSet) {
    execute(std::make_unique<AddLevelSetCommand>(std::move(levelSet)));
}

bool SessionState::openSavedLevelSet(std::span<const unsigned char> bytes) {
    if (activeLevelSet) {
        loadError = "A level set is already being built.";
        return false;
    }
    SavedLevelSet saved = readSavedLevelSet(bytes);
    if (!saved.ok()) {
        loadError = std::move(saved.error);
        return false;
    }
    const std::size_t incoming = saved.field.values.size() * sizeof(float);
    if (!loadFits(incoming)) {
        loadError = "That level set is over the 2 GB limit.";
        return false;
    }
    const std::string id = uniqueLevelSetId(*this, std::move(saved.id));
    const float isoValue = saved.isoValue;
    const glm::vec3 colour = saved.colour;
    const float transparency = saved.transparency;
    const bool closeAtMissingData = saved.closeAtMissingData;
    loadError.clear();
    activeLevelSet.emplace(
        std::vector<TraitPoint>{},
        std::shared_ptr<const MultiField>{},
        isoValue,
        colour,
        id,
        true,
        std::move(saved.field),
        closeAtMissingData,
        transparency);
    activeLevelSet->start();
    return true;
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

void SessionState::setLevelSetIso(int index, float before, float after) {
    if (index < 0 || index >= static_cast<int>(levelSets.size()) || before == after) return;
    if (!levelSets[static_cast<std::size_t>(index)].distanceField) return;
    execute(std::make_unique<SetLevelSetIsoCommand>(index, before, after));
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

namespace {

std::size_t attributeBytes(const MultiField& field) {
    std::size_t bytes = 0;
    for (const Attribute& attribute : field.attributeDomain)
        bytes += attribute.values.size() * sizeof(float);
    return bytes;
}

std::size_t keptFieldBytes(const FeatureLevelSet& levelSet) {
    if (!levelSet.distanceField) return 0;
    return levelSet.distanceField->values.size() * sizeof(float);
}

std::size_t selectedAttributeBytes(const DatasetDirConfig& config, const std::vector<std::string>& selectedFiles) {
    if (!config.ok() || !config.dataset || config.dataset->format != ATTRIBUTEPERFILE) return 0;
    const auto* dataset = static_cast<const AttributePerFileDataset*>(config.dataset.get());
    std::size_t count = 0;
    for (const auto& file : dataset->getFiles()) {
        if (selectedFiles.empty() || std::find(selectedFiles.begin(), selectedFiles.end(), file.filename) != selectedFiles.end())
            ++count;
    }
    const std::size_t perFile = static_cast<std::size_t>(dataset->getValuesPerFile()) * sizeof(float);
    if (count > 0 && perFile > kDatasetMemoryCap / count) return kDatasetMemoryCap + 1;
    return count * perFile;
}

}

std::size_t SessionState::residentBytes() const {
    std::unordered_set<const MultiField*> seen;
    std::size_t bytes = 0;
    auto addDataset = [&](const MultiField* field) {
        if (field == nullptr || !seen.insert(field).second) return;
        bytes += attributeBytes(*field);
    };
    auto addLevelSet = [&](const FeatureLevelSet& levelSet) {
        addDataset(levelSet.dataset.get());
        bytes += keptFieldBytes(levelSet);
    };
    for (const std::shared_ptr<MultiField>& dataset : datasets) addDataset(dataset.get());
    for (const FeatureLevelSet& levelSet : levelSets) addLevelSet(levelSet);

    std::vector<const FeatureLevelSet*> parked;
    for (const std::unique_ptr<SessionCommand>& command : undoStack) command->retainedLevelSets(parked);
    for (const std::unique_ptr<SessionCommand>& command : redoStack) command->retainedLevelSets(parked);
    for (const FeatureLevelSet* levelSet : parked) addLevelSet(*levelSet);
    return bytes;
}

bool SessionState::loadFits(std::size_t incoming) const {
    if (incoming > kDatasetMemoryCap) return false;
    return residentBytes() <= kDatasetMemoryCap - incoming;
}

bool SessionState::beginDatasetLoad(std::string path, AttributeBytesProvider provider, std::vector<std::string> selectedFiles) {
    if (activeLoad) {
        loadError = "A dataset is already loading.";
        return false;
    }
    const std::size_t incoming = selectedAttributeBytes(parseConfig(path), selectedFiles);
    if (!loadFits(incoming)) {
        loadError = "That selection is over the 2 GB limit.";
        return false;
    }
    loadError.clear();
    activeLoad.emplace(std::move(path), std::move(provider), std::move(selectedFiles));
    activeLoad->start();
    return true;
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
    if (activeLevelSet && !activeLevelSet->runsOnWorker() && !activeLevelSet->finished()) {
        // One Isabel slab can be slower than a frame, so the first call does a
        // single slab and later calls in the same frame take more only while
        // they are cheap. Marching cubes still runs to completion on the last step.
        const auto budget = std::chrono::milliseconds(16);
        const auto start = std::chrono::steady_clock::now();
        int slabs = 1;
        while (activeLevelSet && !activeLevelSet->finished()) {
            activeLevelSet->stepOnMainThread(slabs);
            const auto elapsed = std::chrono::steady_clock::now() - start;
            if (elapsed >= budget) break;
            if (elapsed < budget / 4) slabs = std::min(slabs * 2, 64);
        }
    }
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
