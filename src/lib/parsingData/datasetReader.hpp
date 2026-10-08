#pragma once

#include "../multiField/multiField.hpp"
#include "datasetConfig.hpp"

#include <atomic>
#include <optional>
#include <stop_token>
#include <string>
#include <thread>

MultiField readDataset(std::string filepath);

// Reads a dataset off the interface thread. The result is published only by take(),
// which the interface calls once finished() is true.
class DatasetReadJob {
public:
    explicit DatasetReadJob(std::string path);
    void start();
    void stepOnMainThread();
    [[nodiscard]] float progress() const;
    [[nodiscard]] bool finished() const;
    [[nodiscard]] bool runsOnWorker() const { return startedOnThread; }
    void cancel();
    [[nodiscard]] std::optional<MultiField> take();
private:
    void run(std::stop_token token);
    void stepFile();
    void markCancelled();

    std::string path;
    DatasetDirConfig config;
    MultiField result;
    int nextFile = 0;
    bool parsed = false;
    bool cancelled = false;
    bool startedOnThread = false;
    std::atomic<int> fileCount{0};
    std::atomic<int> filesDone{0};
    std::atomic<bool> stop{false};
    std::atomic<bool> complete{false};
    std::jthread worker;
};