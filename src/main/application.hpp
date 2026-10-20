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
#ifdef __EMSCRIPTEN__
    // The canvas depth buffer is not DEPTH_COMPONENT24, so the opaque pass is
    // drawn here and peels copy depth from this target.
    unsigned int sceneFbo = 0;
    unsigned int sceneColour = 0;
    unsigned int sceneDepth = 0;
    bool sceneBlitChecked = false;
#endif
    int width = 0;
    int height = 0;
    bool reportedIncomplete = false;

    // Allocates `peelCount` colour textures. A smaller count deletes the extras.
    // The two depth textures stay; peel i writes depth[i % 2] and reads the other.
    bool ensure(int newWidth, int newHeight, int peelCount);
    void destroy();
};

// Square shadow maps. The light peel does not follow the window size.
constexpr int kLightShadowResolution = 1024;

// Depth array layer 0 is the opaque surface. Layer i+1 and alpha layer i are
// transparent peel i, nearest to the light first. The peel test itself still
// ping-pongs the two ordinary depth textures.
struct LightShadowBuffers {
    unsigned int fbo = 0;
    unsigned int readFbo = 0;
    unsigned int depth[2] = {};
    unsigned int depthArray = 0;
    unsigned int alphaArray = 0;
    unsigned int dummyDepth = 0;
    unsigned int dummyDepthArray = 0;
    unsigned int dummyAlphaArray = 0;
    unsigned int query = 0;
    int resolution = 0;
    int peelSlots = 0;
    bool reportedIncomplete = false;

    bool ensure(int newResolution, int peelCount);
    void bindForLighting(bool useLayers);
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
    // The browser dataset callbacks load into this session.
    [[nodiscard]] SessionState& sessionState() { return session; }

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
    LightShadowBuffers lightShadows;
    glm::vec3 colourRed{1.f, 0.f, 0.f};
    glm::vec3 colourGreen{0.f, 1.f, 0.f};
    glm::mat4 defaultModel{1.f};
};
