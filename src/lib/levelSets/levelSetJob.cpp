#include "levelSetJob.hpp"

#include <utility>

namespace {

std::vector<TraitPoint> normalised(std::vector<TraitPoint> points) {
    for (TraitPoint& point : points) point.normalise();
    return points;
}

constexpr int kSlabsPerStep = 8;

}

LevelSetJob::LevelSetJob(std::vector<TraitPoint> pointsIn, std::shared_ptr<const MultiField> datasetIn,
                         float isoValueIn, glm::vec3 colourIn, std::string idIn)
    : dataset(std::move(datasetIn))
    , points(normalised(std::move(pointsIn)))
    , field(points, *dataset)
    , isoValue(isoValueIn)
    , colour(colourIn)
    , id(std::move(idIn)) {}

void LevelSetJob::start() {
    if (startedOnThread || complete.load()) return;
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
    startedOnThread = true;
    worker = std::jthread([this](std::stop_token token) { run(token); });
#endif
}

void LevelSetJob::stepOnMainThread(int slabs) {
    if (complete.load(std::memory_order_acquire)) return;
    if (stop.load()) { finishCancelled(); return; }
    if (!field.done()) {
        field.step(slabs);
        if (!field.done()) return;
    }
    if (stop.load()) { finishCancelled(); return; }
    finishSurface();
}

float LevelSetJob::progress() const {
    return field.progress();
}

bool LevelSetJob::finished() const {
    return complete.load(std::memory_order_acquire);
}

void LevelSetJob::cancel() {
    stop.store(true);
    if (worker.joinable()) worker.request_stop();
}

std::optional<FeatureLevelSet> LevelSetJob::take() {
    if (!finished() || cancelled) return std::nullopt;
    return std::move(result);
}

void LevelSetJob::run(std::stop_token token) {
    if (stopRequested(token)) { finishCancelled(); return; }
    while (!field.done()) {
        if (stopRequested(token)) { finishCancelled(); return; }
        field.step(kSlabsPerStep);
    }
    if (stopRequested(token)) { finishCancelled(); return; }
    finishSurface();
}

bool LevelSetJob::stopRequested(std::stop_token token) const {
    return stop.load() || token.stop_requested();
}

void LevelSetJob::finishCancelled() {
    cancelled = true;
    complete.store(true, std::memory_order_release);
}

void LevelSetJob::finishSurface() {
    Surface surface = extractSurface(std::move(field).take(), isoValue);
    result = FeatureLevelSet(points, std::move(surface), colour, id);
    complete.store(true, std::memory_order_release);
}
