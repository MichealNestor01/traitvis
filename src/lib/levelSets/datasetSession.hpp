#pragma once

#include "featureLevelSet.hpp"

#include <memory>
#include <string>
#include <vector>

// The datasets a session is showing, and the level sets built from them.
// A level set holds its own shared_ptr, so removing a dataset does not free a surface
// that was built from it. Trait vertices always refer to the active dataset.
struct DatasetSession {
    std::vector<std::shared_ptr<MultiField>> datasets;
    int active = -1;
    std::vector<TraitPoint> attributeSpacePointsBuffer;
    std::vector<FeatureLevelSet> levelSets;

    [[nodiscard]] std::shared_ptr<MultiField> activeDataset() const;
    void addDataset(std::shared_ptr<MultiField> dataset);
    // Returns true when the active dataset changed, so the interface can rebuild its sliders.
    bool setActiveDataset(int index);
    bool removeDataset(int index);
    void generateLevelSet(std::vector<TraitPoint> points, float euclideanDistance, glm::vec3 colour, std::string id);
};
