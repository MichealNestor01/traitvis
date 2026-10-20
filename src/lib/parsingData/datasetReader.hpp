#pragma once

#include "../multiField/multiField.hpp"
#include "datasetConfig.hpp"

#include <atomic>
#include <filesystem>
#include <functional>
#include <optional>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

// Supplies one attribute file. Desktop reads the file; a browser build returns bytes from JavaScript.
using AttributeBytesProvider = std::function<std::vector<float>(const std::filesystem::path& file, std::size_t count)>;

[[nodiscard]] MultiField readDataset(const DatasetDirConfig& config, const AttributeBytesProvider& provider);
[[nodiscard]] MultiField readDataset(const std::filesystem::path& configFile);

// Reads a dataset off the interface thread. The result is published only by take(),
// which the interface calls once finished() is true. An empty provider reads the files.
class DatasetReadJob {
public:
    explicit DatasetReadJob(std::string path, AttributeBytesProvider provider = {});
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
    AttributeBytesProvider provider;
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