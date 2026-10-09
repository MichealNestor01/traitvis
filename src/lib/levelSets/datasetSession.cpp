#include "datasetSession.hpp"

#include <algorithm>
#include <utility>

std::shared_ptr<MultiField> DatasetSession::activeDataset() const {
    if (active < 0 || active >= static_cast<int>(datasets.size())) return nullptr;
    return datasets[active];
}

void DatasetSession::addDataset(std::shared_ptr<MultiField> dataset) {
    datasets.push_back(std::move(dataset));
    active = static_cast<int>(datasets.size()) - 1;
    attributeSpacePointsBuffer.clear();
}

bool DatasetSession::setActiveDataset(int index) {
    if (index < 0 || index >= static_cast<int>(datasets.size()) || index == active) return false;
    active = index;
    attributeSpacePointsBuffer.clear();
    return true;
}

bool DatasetSession::removeDataset(int index) {
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

void DatasetSession::generateLevelSet(std::vector<TraitPoint> points, float euclideanDistance, glm::vec3 colour, std::string id) {
    const std::shared_ptr<MultiField> field = activeDataset();
    if (!field || !field->ok()) return;
    levelSets.emplace_back(std::move(points), std::shared_ptr<const MultiField>(field), euclideanDistance, colour, std::move(id));
}
