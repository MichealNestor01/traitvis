#pragma once

#include "featureLevelSet.hpp"
#include "levelSetJob.hpp"
#include "../parsingData/datasetReader.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

// Datasets, trait vertices, and level sets. No window, camera, or OpenGL.
// A level set holds its own shared_ptr, so removing a dataset does not free a surface
// that was built from it. Trait vertices always refer to the active dataset.
struct SessionState {
    std::vector<std::shared_ptr<MultiField>> datasets;
    int active = -1;
    std::vector<TraitPoint> attributeSpacePointsBuffer;
    std::vector<FeatureLevelSet> levelSets;
    // Declared last so a running read joins before the datasets are destroyed.
    std::optional<DatasetReadJob> activeLoad;
    std::optional<LevelSetJob> activeLevelSet;

    [[nodiscard]] std::shared_ptr<MultiField> activeDataset() const;
    void addDataset(std::shared_ptr<MultiField> dataset);
    // Returns true when the active dataset changed, so the interface can rebuild its sliders.
    bool setActiveDataset(int index);
    bool removeDataset(int index);
    void addAttributeSpacePointToBuffer(TraitPoint point);
    // Synchronous. Tests build a level set without a background job.
    void generateLevelSet(std::vector<TraitPoint> points, float euclideanDistance, glm::vec3 colour, std::string id);
    // Uses the trait buffer and finishes on a background job.
    void generateLevelSet(float euclideanDistance, glm::vec3 colour, std::string id);
    void beginDatasetLoad(std::string path);
    // Steps a load that has no worker, then adds a finished dataset and makes it active.
    // Returns true when the active dataset changed. A cancel installs nothing.
    bool pollDatasetLoad();
    void pollLevelSet();
};
