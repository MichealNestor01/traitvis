#include "webDataset.hpp"

#include "application.hpp"

#ifdef __EMSCRIPTEN__

#include <emscripten.h>

#include "../lib/parsingData/configParser.hpp"
#include "../lib/parsingData/fileReader.hpp"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

Application* boundApplication = nullptr;
// Bytes JavaScript is still copying. beginLoad takes ownership of this map.
std::unordered_map<std::string, std::vector<float>> stagedAttributes;

std::vector<std::string> splitCsv(const char* csv) {
    std::vector<std::string> names;
    if (csv == nullptr || csv[0] == '\0') return names;
    std::stringstream stream(csv);
    std::string name;
    while (std::getline(stream, name, ',')) {
        const auto first = name.find_first_not_of(" \t");
        if (first == std::string::npos) continue;
        const auto last = name.find_last_not_of(" \t");
        names.push_back(name.substr(first, last - first + 1));
    }
    return names;
}

std::vector<float> takeStaged(const std::shared_ptr<std::unordered_map<std::string, std::vector<float>>>& staged,
                              const std::filesystem::path& file, std::size_t count) {
    auto it = staged->find(file.filename().string());
    if (it == staged->end() || it->second.size() != count) return {};
    std::vector<float> values = std::move(it->second);
    staged->erase(it);
    // JavaScript copied the file's big-endian bytes straight into the floats.
    swapFloatEndianInPlace(values);
    return values;
}

}

void traitvisBindApplication(Application* application) {
    boundApplication = application;
}

extern "C" {

EMSCRIPTEN_KEEPALIVE
float* traitvisBeginAttribute(const char* filename, std::size_t count) {
    if (filename == nullptr || filename[0] == '\0' || count == 0) return nullptr;
    std::vector<float>& buffer = stagedAttributes[filename];
    buffer.assign(count, 0.f);
    return buffer.data();
}

EMSCRIPTEN_KEEPALIVE
void traitvisCancelPending() {
    stagedAttributes.clear();
}

// Caller frees the result with free(). An error starts with '!'.
// Each attribute is "filename\\tname\\tcount\\n".
EMSCRIPTEN_KEEPALIVE
char* traitvisDescribeConfig(const char* configPath) {
    std::string text;
    if (configPath == nullptr) {
        text = "!Missing config path\n";
    } else {
        const DatasetDirConfig config = parseConfig(configPath);
        if (!config.ok()) {
            text = "!" + config.error;
        } else if (!config.dataset || config.dataset->format != ATTRIBUTEPERFILE) {
            text = "!Dataset format not currently supported\n";
        } else {
            const auto* dataset = static_cast<const AttributePerFileDataset*>(config.dataset.get());
            for (const auto& file : dataset->getFiles()) {
                text += file.filename;
                text += '\t';
                text += file.name;
                text += '\t';
                text += std::to_string(dataset->getValuesPerFile());
                text += '\n';
            }
        }
    }
    char* copy = static_cast<char*>(std::malloc(text.size() + 1));
    if (copy == nullptr) return nullptr;
    std::memcpy(copy, text.c_str(), text.size() + 1);
    return copy;
}

// Returns 1 when the session accepted the load. A 0 leaves the staged buffers freed.
EMSCRIPTEN_KEEPALIVE
int traitvisBeginLoad(const char* configPath, const char* selectedCsv) {
    auto staged = std::make_shared<std::unordered_map<std::string, std::vector<float>>>(std::move(stagedAttributes));
    stagedAttributes.clear();
    if (boundApplication == nullptr || configPath == nullptr) return 0;

    std::vector<std::string> selected = splitCsv(selectedCsv);
    if (!selected.empty()) {
        for (auto it = staged->begin(); it != staged->end(); ) {
            if (std::find(selected.begin(), selected.end(), it->first) == selected.end()) it = staged->erase(it);
            else ++it;
        }
    }
    AttributeBytesProvider provider = [staged](const std::filesystem::path& file, std::size_t count) {
        return takeStaged(staged, file, count);
    };
    // A refused load drops `staged` with this provider, so the buffers do not leak.
    return boundApplication->sessionState().beginDatasetLoad(configPath, std::move(provider), std::move(selected)) ? 1 : 0;
}

}

#else

void traitvisBindApplication(Application*) {}

#endif
