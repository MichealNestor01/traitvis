#include "datasetReader.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "../multiField/multiField.hpp"
#include "datasetConfig.hpp"
#include "fileReader.hpp"
#include "configParser.hpp"

namespace {

void prepareField(MultiField& multiField, const DatasetDirConfig& config, const AttributePerFileDataset& dataset) {
    multiField.name = config.name;
    multiField.xVals = dataset.xVals;
    multiField.yVals = dataset.yVals;
    multiField.zVals = dataset.zVals;
    multiField.strides = makeStrides(dataset.scheme, dataset.dimOrder, dataset.xVals, dataset.yVals, dataset.zVals);
}

template <typename File>
bool readAttributeFile(MultiField& multiField, const DatasetDirConfig& config, const File& file, int valuesPerFile) {
    const std::filesystem::path path = config.filePath / file.filename;
    std::vector<float> vals = readFloatBinaryFile(path, static_cast<std::size_t>(valuesPerFile), true);
    if (vals.size() == 0) {
        multiField.error = "Failed to read values from " + path.string() + "\n";
        return false;
    }
    // A configured fill value becomes NaN so it passes through normalisation
    // unchanged and loses every distance comparison (NaN < x is false).
    if (config.noData.has_value()) {
        const float sentinel = *config.noData;
        std::ranges::replace_if(vals,
            [sentinel](float value) { return value == sentinel; },
            std::numeric_limits<float>::quiet_NaN());
    }
    // reinterpret the lower and upper bounds as the actual lowest and greatest value in the file
    // values will then be normalised between these bounds
    float lowerBound = file.upperBound;
    float upperBound = file.lowerBound;
    for (float val : vals) {
        if (std::isnan(val)) continue;
        if (val < lowerBound && val >= file.lowerBound) lowerBound = val;
        if (val > upperBound && val <= file.upperBound) upperBound = val;
    }

    multiField.attributeDomain.push_back({file.name, {lowerBound, upperBound}, std::move(vals)});
    return true;
}

}

DatasetReadJob::DatasetReadJob(std::string path) : path(std::move(path)) {}

void DatasetReadJob::start() {
    if (startedOnThread || complete.load()) return;
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
    startedOnThread = true;
    worker = std::jthread([this](std::stop_token token) { run(token); });
#endif
}

void DatasetReadJob::stepOnMainThread() {
    if (complete.load(std::memory_order_acquire)) return;
    if (stop.load()) { markCancelled(); return; }
    stepFile();
}

float DatasetReadJob::progress() const {
    if (complete.load(std::memory_order_acquire)) return 1.f;
    const int total = fileCount.load();
    if (total <= 0) return 0.f;
    return static_cast<float>(filesDone.load()) / static_cast<float>(total);
}

bool DatasetReadJob::finished() const {
    return complete.load(std::memory_order_acquire);
}

void DatasetReadJob::cancel() {
    stop.store(true);
    if (worker.joinable()) worker.request_stop();
}

std::optional<MultiField> DatasetReadJob::take() {
    if (!finished() || cancelled) return std::nullopt;
    return std::move(result);
}

void DatasetReadJob::run(std::stop_token token) {
    while (!complete.load(std::memory_order_acquire)) {
        if (stop.load() || token.stop_requested()) {
            markCancelled();
            return;
        }
        stepFile();
    }
}

void DatasetReadJob::markCancelled() {
    cancelled = true;
    complete.store(true, std::memory_order_release);
}

void DatasetReadJob::stepFile() {
    if (complete.load(std::memory_order_acquire)) return;
    if (stop.load()) { markCancelled(); return; }

    if (!parsed) {
        config = parseConfig(path);
        parsed = true;
        if (!config.ok()) {
            result.error = config.error;
            complete.store(true, std::memory_order_release);
            return;
        }
        if (!config.dataset || config.dataset->format != ATTRIBUTEPERFILE) {
            result.error = "Dataset format not currently supported\n";
            complete.store(true, std::memory_order_release);
            return;
        }
        const auto* dataset = static_cast<const AttributePerFileDataset*>(config.dataset.get());
        prepareField(result, config, *dataset);
        fileCount.store(static_cast<int>(dataset->getFiles().size()));
    }

    const auto* dataset = static_cast<const AttributePerFileDataset*>(config.dataset.get());
    const auto& files = dataset->getFiles();
    if (nextFile >= static_cast<int>(files.size())) {
        result.normaliseAttributes();
        complete.store(true, std::memory_order_release);
        return;
    }
    if (!readAttributeFile(result, config, files[nextFile], dataset->getValuesPerFile())) {
        complete.store(true, std::memory_order_release);
        return;
    }
    ++nextFile;
    filesDone.store(nextFile);
    if (nextFile >= static_cast<int>(files.size())) {
        result.normaliseAttributes();
        complete.store(true, std::memory_order_release);
    }
}

MultiField readDataset(std::string filepath) {
    DatasetReadJob job(std::move(filepath));
    while (!job.finished()) job.stepOnMainThread();
    auto loaded = job.take();
    if (!loaded) return {};
    return std::move(*loaded);
}
