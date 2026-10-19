#pragma once

#include "../lib/shaderTools/shaderProgram.hpp"
#include "../lib/gui/gui.hpp"
#include "../lib/meshTools/gpuMesh.hpp"
#include "../lib/meshTools/gpuDistanceField.hpp"

#include <glm.hpp>
#include <optional>
#include <vector>

struct DepthPeelBuffers {
    unsigned int fbo = 0;
    unsigned int depth[2] = {};
    std::vector<unsigned int> colour;
    unsigned int dummyDepth = 0;
    unsigned int query = 0;
    unsigned int compositeVao = 0;
    unsigned int compositeVbo = 0;
    int width = 0;
    int height = 0;
    bool reportedIncomplete = false;

    // Allocates `peelCount` colour textures. A smaller count deletes the extras.
    // The two depth textures stay; peel i writes depth[i % 2] and reads the other.
    bool ensure(int newWidth, int newHeight, int peelCount);
    void destroy();
};

// Owns the window, the GL context, and one frame. main() only drives the loop.
class Application {
public:
    Application() = default;
    ~Application() { shutdown(); }
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    [[nodiscard]] bool init();
    void frame();
    void shutdown();
    [[nodiscard]] bool shouldClose() const;

private:
    GLFWwindow* window = nullptr;
    bool glfwReady = false;
    bool glReady = false;
    bool imguiReady = false;
    bool shutDown = false;

    SessionState session;
    std::optional<RenderState> rendering;
    std::optional<GUI> gui;
    std::optional<ShaderProgram> lighting;
    std::optional<ShaderProgram> composite;
    std::optional<GpuDistanceField> distanceField;
    std::optional<GpuMesh> cube;
    DepthPeelBuffers peels;
    glm::vec3 colourRed{1.f, 0.f, 0.f};
    glm::vec3 colourGreen{0.f, 1.f, 0.f};
    glm::mat4 defaultModel{1.f};
};
