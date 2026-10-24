#pragma once

#include "featureLevelSet.hpp"
#include "levelSetJob.hpp"
#include "../parsingData/datasetReader.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

struct SessionState;

// One edit. Concrete commands live in sessionState.cpp; the session only stores this base.
struct SessionCommand {
    virtual ~SessionCommand() = default;
    virtual void undo(SessionState& session) = 0;
    virtual void redo(SessionState& session) = 0;
    // Trait vertices point into the active dataset. They are dropped when that dataset changes.
    [[nodiscard]] virtual bool changesTraitPoints() const { return false; }
    // A level set parked here still owns its dataset and any kept distance field.
    virtual void retainedLevelSets(std::vector<const FeatureLevelSet*>& out) const { (void)out; }
};

// Attributes and kept distance fields. A second copy of Isabel's fourteen attributes does not fit.
inline constexpr std::size_t kDatasetMemoryCap = 2ull * 1024ull * 1024ull * 1024ull;

// Datasets, trait vertices, and level sets. No window, camera, or OpenGL.
// A level set holds its own shared_ptr, so removing a dataset does not free a surface
// that was built from it. Trait vertices always refer to the active dataset.
// Undo covers adding and removing those vertices and level sets, and a level set's colour
// and transparency. Switching dataset drops the vertex history: the vertices' attribute
// pointers address the dataset that was active, and that buffer is cleared.
struct SessionState {
    std::vector<std::shared_ptr<MultiField>> datasets;
    int active = -1;
    std::vector<TraitPoint> attributeSpacePointsBuffer;
    std::vector<FeatureLevelSet> levelSets;
    // The read joins before the datasets are destroyed. History is last so a parked
    // level set is released while its dataset is still alive.
    std::optional<DatasetReadJob> activeLoad;
    std::optional<LevelSetJob> activeLevelSet;
    // Desktop sets this to a GL 4.3 compute dispatch. It runs on the thread that calls
    // generateLevelSet, which is the thread with the context. Empty, or a nullopt result,
    // keeps the CPU slabs. The core library does not call OpenGL itself.
    std::function<std::optional<ScalarField>(std::span<const TraitPoint>, const MultiField&)> gpuDistanceField;
    std::vector<std::unique_ptr<SessionCommand>> undoStack;
    std::vector<std::unique_ptr<SessionCommand>> redoStack;
    // Set when a load was refused. Cleared when a load starts.
    std::string loadError;

    [[nodiscard]] std::shared_ptr<MultiField> activeDataset() const;
    void addDataset(std::shared_ptr<MultiField> dataset);
    // Returns true when the active dataset changed, so the interface can rebuild its sliders.
    bool setActiveDataset(int index);
    bool removeDataset(int index);
    void addAttributeSpacePointToBuffer(TraitPoint point);
    void removeAttributeSpacePoint(int index);
    // Synchronous. Tests build a level set without a background job.
    // keepField retains the distance grid so the isovalue can change without computing it again.
    void generateLevelSet(std::vector<TraitPoint> points, float euclideanDistance, glm::vec3 colour, std::string id, bool keepField = false, bool closeAtMissingData = true);
    // Uses the trait buffer and finishes on a background job.
    // When gpuDistanceField returns a field, the slabs are skipped and the job only extracts the surface.
    void generateLevelSet(float euclideanDistance, glm::vec3 colour, std::string id, bool keepField = false, bool closeAtMissingData = true);
    void addLevelSet(FeatureLevelSet levelSet);
    // Reads a saved level set and extracts its surface on the level-set job.
    // The distance field is kept, so the distance slider can move it afterwards.
    bool openSavedLevelSet(std::span<const unsigned char> bytes);
    void removeLevelSet(int index);
    // `before` is the value at the start of the edit. The level set already holds `after`.
    void setLevelSetColour(int index, glm::vec3 before, glm::vec3 after);
    void setLevelSetTransparency(int index, float before, float after);
    // Re-extracts the surface from a kept distance field. No command is recorded when the field was discarded.
    void setLevelSetIso(int index, float before, float after);
    void undo();
    void redo();
    [[nodiscard]] bool canUndo() const noexcept { return !undoStack.empty(); }
    [[nodiscard]] bool canRedo() const noexcept { return !redoStack.empty(); }
    // Drops parked level sets. Call while the GL context is current, so a mesh that was
    // undone after being drawn deletes its buffers before the context goes away.
    void clearHistory();
    // Attribute samples still reachable from the dataset list, a level set, or the undo
    // stack, plus every kept distance field. A removed dataset stays counted while a
    // level set or an undo command still holds it.
    [[nodiscard]] std::size_t residentBytes() const;
    // `incoming` is the attribute bytes a new load would add.
    [[nodiscard]] bool loadFits(std::size_t incoming) const;
    // Returns false when a load is already running. An empty provider reads the files.
    // An empty `selectedFiles` reads every attribute.
    bool beginDatasetLoad(std::string path, AttributeBytesProvider provider = {}, std::vector<std::string> selectedFiles = {});
    // Steps a load that has no worker, then adds a finished dataset and makes it active.
    // Returns true when the active dataset changed. A cancel installs nothing.
    bool pollDatasetLoad();
    void pollLevelSet();

private:
    void execute(std::unique_ptr<SessionCommand> command);
    void clearTraitPoints();
};
