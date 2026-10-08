#pragma once

#include "featureLevelSet.hpp"

#include <atomic>
#include <memory>
#include <optional>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

// Builds one level set without blocking the interface. Distance-field slabs run on a
// worker (or one batch per frame where there is no thread), then marching cubes runs
// once, on that same thread. The mesh is collected with take() on the interface thread.
class LevelSetJob {
public:
    LevelSetJob(std::vector<TraitPoint> points, std::shared_ptr<const MultiField> dataset,
                float isoValue, glm::vec3 colour, std::string id);
    void start();
    void stepOnMainThread(int slabs);
    [[nodiscard]] float progress() const;
    [[nodiscard]] bool finished() const;
    [[nodiscard]] bool runsOnWorker() const { return startedOnThread; }
    void cancel();
    [[nodiscard]] std::optional<FeatureLevelSet> take();
private:
    void run(std::stop_token token);
    bool stopRequested(std::stop_token token) const;
    void finishCancelled();
    void finishSurface();

    std::shared_ptr<const MultiField> dataset;
    std::vector<TraitPoint> points;
    DistanceFieldJob field;
    float isoValue = 0.f;
    glm::vec3 colour{};
    std::string id;
    std::optional<FeatureLevelSet> result;
    bool cancelled = false;
    bool startedOnThread = false;
    std::atomic<bool> stop{false};
    std::atomic<bool> complete{false};
    std::jthread worker;
};
