#include "sessionState.hpp"

#include <algorithm>
#include <utility>

std::shared_ptr<MultiField> SessionState::activeDataset() const {
    if (active < 0 || active >= static_cast<int>(datasets.size())) return nullptr;
    return datasets[active];
}

void SessionState::addDataset(std::shared_ptr<MultiField> dataset) {
    datasets.push_back(std::move(dataset));
    active = static_cast<int>(datasets.size()) - 1;
    attributeSpacePointsBuffer.clear();
}

bool SessionState::setActiveDataset(int index) {
    if (index < 0 || index >= static_cast<int>(datasets.size()) || index == active) return false;
    active = index;
    attributeSpacePointsBuffer.clear();
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
    if (changed) attributeSpacePointsBuffer.clear();
    return changed;
}

void SessionState::addAttributeSpacePointToBuffer(TraitPoint point) {
    attributeSpacePointsBuffer.push_back(std::move(point));
}

void SessionState::generateLevelSet(std::vector<TraitPoint> points, float euclideanDistance, glm::vec3 colour, std::string id) {
    const std::shared_ptr<MultiField> field = activeDataset();
    if (!field || !field->ok()) return;
    levelSets.emplace_back(std::move(points), std::shared_ptr<const MultiField>(field), euclideanDistance, colour, std::move(id));
}

void SessionState::generateLevelSet(float euclideanDistance, glm::vec3 colour, std::string id) {
    const std::shared_ptr<MultiField> field = activeDataset();
    if (activeLevelSet || !field || !field->ok()) return;
    activeLevelSet.emplace(attributeSpacePointsBuffer, std::shared_ptr<const MultiField>(field), euclideanDistance, colour, std::move(id));
    activeLevelSet->start();
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
        levelSets.push_back(std::move(*made));
    activeLevelSet.reset();
}
