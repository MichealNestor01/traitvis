#include "application.hpp"
#include "webDataset.hpp"
#include "../platform/gl.hpp"

#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/matrix_inverse.hpp>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <algorithm>
#include <cfloat>
#include <iostream>
#include <span>
#include <stdexcept>
#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#include <emscripten.h>
#include <GLFW/emscripten_glfw3.h>
#endif

#include "callbacks.hpp"
#include "inputProcessing.hpp"
#include "../lib/camera/pointLight.hpp"
#include "../lib/meshTools/cube.hpp"

namespace {

#ifdef __EMSCRIPTEN__
bool onPointerLockChange(int, const EmscriptenPointerlockChangeEvent* event, void* user) {
    auto* window = static_cast<GLFWwindow*>(user);
    auto* state = static_cast<RenderState*>(glfwGetWindowUserPointer(window));
    if (!state) return true;
    if (event->isActive) {
        // Space can be released again before the browser finishes locking.
        if (state->enableCam) state->pointerLocked = true;
        else glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        return true;
    }
    state->releaseCam(window);
    return true;
}

// While the pointer is locked, contrib.glfw3 stores each movement as the cursor
// position and skips the callback when the next movement is identical. Reading
// movementX here keeps turning when the cursor would otherwise sit in the corner.
EM_BOOL onCapturedMouseMove(int, const EmscriptenMouseEvent* event, void* user) {
    auto* window = static_cast<GLFWwindow*>(user);
    auto* state = static_cast<RenderState*>(glfwGetWindowUserPointer(window));
    if (!state || !state->enableCam || !state->pointerLocked) return EM_FALSE;
    state->cam.addLook(static_cast<float>(event->movementX), static_cast<float>(event->movementY));
    return EM_FALSE;
}

bool onPointerLockError(int, const void*, void* user) {
    auto* window = static_cast<GLFWwindow*>(user);
    if (auto* state = static_cast<RenderState*>(glfwGetWindowUserPointer(window)))
        state->releaseCam(window);
    return true;
}
#endif

#ifdef __EMSCRIPTEN__
GLFWkeyfun previousKeyCallback = nullptr;

void webKeyCallback(GLFWwindow* keyWindow, int key, int scancode, int action, int mods) {
    if (previousKeyCallback) previousKeyCallback(keyWindow, key, scancode, action, mods);
    auto* state = static_cast<RenderState*>(glfwGetWindowUserPointer(keyWindow));
    // Escape always drops the camera, including when a panel had the keyboard.
    if (state && state->enableCam && key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        state->releaseCam(keyWindow);
        return;
    }
    // WantCaptureKeyboard is the previous frame, which is what ImGui uses for this event.
    if (ImGui::GetIO().WantCaptureKeyboard) return;
    onWebKey(keyWindow, key, action);
}
#endif

GLFWcursorposfun previousCursorCallback = nullptr;
GLFWmousebuttonfun previousMouseButtonCallback = nullptr;
GLFWscrollfun previousScrollCallback = nullptr;

void cursorCallback(GLFWwindow* cursorWindow, double x, double y) {
    auto* state = static_cast<RenderState*>(glfwGetWindowUserPointer(cursorWindow));
    // Captured look must not move ImGui's cursor onto a panel.
    if (state && state->enableCam) {
#ifndef __EMSCRIPTEN__
        state->cam.updateDirection(x, y);
#endif
        return;
    }
    if (previousCursorCallback) previousCursorCallback(cursorWindow, x, y);
    if (state) state->cam.updateMousePos(x, y);
}

void mouseButtonCallback(GLFWwindow* buttonWindow, int button, int action, int mods) {
    auto* state = static_cast<RenderState*>(glfwGetWindowUserPointer(buttonWindow));
    if (state && state->enableCam) return;
    if (previousMouseButtonCallback) previousMouseButtonCallback(buttonWindow, button, action, mods);
}

void scrollCallback(GLFWwindow* scrollWindow, double xoffset, double yoffset) {
    auto* state = static_cast<RenderState*>(glfwGetWindowUserPointer(scrollWindow));
    if (state && state->enableCam) return;
    if (previousScrollCallback) previousScrollCallback(scrollWindow, xoffset, yoffset);
}

void bindDepthTexture(unsigned int texture) {
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
}

void drawLevelSet(ShaderProgram& shader, FeatureLevelSet& levelSet, const GpuMesh& cubeMesh, const glm::vec3& activeColour, const glm::vec3& inactiveColour) {
    if (!levelSet.mesh) levelSet.mesh = uploadMesh(levelSet.surface);
    const GpuMesh& mesh = *levelSet.mesh;
    shader.setUniformb("invertNormal", levelSet.invertNormals);
    shader.setUniform3f("objectColor", levelSet.colour);
    shader.setUniformf("objectTransparency", levelSet.transparency);
    shader.setUniformb("useInstanceModel", false);
    glBindVertexArray(mesh.vao.get());
    glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);

    if (!levelSet.showActiveInactivePixels) return;
    shader.setUniformb("useInstanceModel", true);
    glBindVertexArray(cubeMesh.vao.get());
    if (mesh.activeInstances) {
        shader.setUniform3f("objectColor", activeColour);
        bindInstanceBuffer(mesh.activeInstances.get());
        glDrawArraysInstanced(GL_TRIANGLES, 0, cubeMesh.vertexCount, static_cast<GLsizei>(levelSet.surface.activeVertices.size()));
    }
    if (mesh.inactiveInstances) {
        shader.setUniform3f("objectColor", inactiveColour);
        bindInstanceBuffer(mesh.inactiveInstances.get());
        glDrawArraysInstanced(GL_TRIANGLES, 0, cubeMesh.vertexCount, static_cast<GLsizei>(levelSet.surface.inactiveVertices.size()));
    }
    shader.setUniformb("useInstanceModel", false);
}

void bindArraySampler(bool depth) {
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    if (depth) glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_NONE);
}

void datasetAabb(const SessionState& session, glm::vec3& minCorner, glm::vec3& maxCorner) {
    int x = 1;
    int y = 1;
    int z = 1;
    const auto consider = [&](const MultiField* field) {
        if (field == nullptr) return;
        x = std::max(x, field->xVals);
        y = std::max(y, field->yVals);
        z = std::max(z, field->zVals);
    };
    if (const std::shared_ptr<MultiField> active = session.activeDataset()) consider(active.get());
    for (const FeatureLevelSet& levelSet : session.levelSets)
        if (levelSet.dataset) consider(levelSet.dataset.get());
    minCorner = glm::vec3(0.f);
    maxCorner = glm::vec3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
}

bool anyDrawnLevelSet(const SessionState& session) {
    for (const FeatureLevelSet& levelSet : session.levelSets)
        if (levelSet.active && levelSet.transparency > 0.f) return true;
    return false;
}

void drawOpaqueLevelSets(ShaderProgram& shader, SessionState& session, const GpuMesh& cubeMesh,
                         const glm::vec3& activeColour, const glm::vec3& inactiveColour) {
    for (FeatureLevelSet& levelSet : session.levelSets)
        if (levelSet.active && levelSet.transparency >= 1.f)
            drawLevelSet(shader, levelSet, cubeMesh, activeColour, inactiveColour);
}

void drawTransparentLevelSets(ShaderProgram& shader, SessionState& session, const GpuMesh& cubeMesh,
                              const glm::vec3& activeColour, const glm::vec3& inactiveColour) {
    for (FeatureLevelSet& levelSet : session.levelSets)
        if (levelSet.active && levelSet.transparency > 0.f && levelSet.transparency < 1.f)
            drawLevelSet(shader, levelSet, cubeMesh, activeColour, inactiveColour);
}

void drawLightMarker(ShaderProgram& shader, const GpuMesh& cubeMesh, glm::vec3 position, float size,
                     glm::vec3 colour, glm::mat4& identity) {
    glm::mat4 model = glm::translate(glm::mat4(1.f), position) * glm::scale(glm::mat4(1.f), glm::vec3(size));
    shader.setUniformMat4f("model", &model);
    shader.setUniformb("emissive", true);
    shader.setUniformb("useInstanceModel", false);
    shader.setUniformb("invertNormal", false);
    shader.setUniformf("objectTransparency", 1.f);
    glBindVertexArray(cubeMesh.vao.get());

    // The fill is the light colour, which is often white on the white background.
    // Push it back slightly so the dark edges below win the depth test and stay visible.
    shader.setUniform3f("objectColor", colour);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.f, 1.f);
    glDrawArrays(GL_TRIANGLES, 0, cubeMesh.vertexCount);
    glDisable(GL_POLYGON_OFFSET_FILL);

#ifndef __EMSCRIPTEN__
    GLint polygonMode[2] = {GL_FILL, GL_FILL};
    glGetIntegerv(GL_POLYGON_MODE, polygonMode);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    shader.setUniform3f("objectColor", glm::vec3(0.08f));
    glDrawArrays(GL_TRIANGLES, 0, cubeMesh.vertexCount);
    glPolygonMode(GL_FRONT_AND_BACK, polygonMode[0]);
#endif

    shader.setUniformb("emissive", false);
    shader.setUniformMat4f("model", &identity);
}

void attachDepth(GLenum target, unsigned int name, bool array, int layer) {
    if (array) glFramebufferTextureLayer(target, GL_DEPTH_ATTACHMENT, name, 0, layer);
    else glFramebufferTexture2D(target, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, name, 0);
}

void blitDepth(unsigned int readFbo, unsigned int readName, bool readArray, int readLayer,
               unsigned int drawFbo, unsigned int drawName, bool drawArray, int drawLayer, int size) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, readFbo);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    attachDepth(GL_READ_FRAMEBUFFER, readName, readArray, readLayer);
    glReadBuffer(GL_NONE);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFbo);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    attachDepth(GL_DRAW_FRAMEBUFFER, drawName, drawArray, drawLayer);
    setDrawBuffer(GL_NONE);
    glBlitFramebuffer(0, 0, size, size, 0, 0, size, size, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}

struct LitFromLight {
    bool ready = false;
    int layers = 0;
};

// WebGL2 does not publish an occlusion-query result until the browser regains
// control, so a same-frame read is always zero and the peel stops immediately.
// On that target every requested layer is drawn. An empty layer is cleared to
// transparent and does not change the composite.
template <typename Draw>
bool drawPeel(unsigned int query, Draw&& draw) {
#ifndef __EMSCRIPTEN__
    glBeginQuery(GL_ANY_SAMPLES_PASSED, query);
#endif
    draw();
#ifndef __EMSCRIPTEN__
    glEndQuery(GL_ANY_SAMPLES_PASSED);
    unsigned int passed = 0;
    glGetQueryObjectuiv(query, GL_QUERY_RESULT, &passed);
    return passed != 0;
#else
    (void)query;
    return true;
#endif
}

LitFromLight renderLightShadows(LightShadowBuffers& shadows, SessionState& session, ShaderProgram& shader,
                                const GpuMesh& cubeMesh, const LightFrustum& frustum, int maxDepthPeels,
                                const glm::vec3& activeColour, const glm::vec3& inactiveColour) {
    if (!shadows.ensure(kLightShadowResolution, maxDepthPeels)) return {};
    const int size = shadows.resolution;

    glViewport(0, 0, size, size);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
#ifndef __EMSCRIPTEN__
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif

    glm::mat4 projection = frustum.projection;
    glm::mat4 view = frustum.view;
    glm::mat4 identity(1.f);
    shader.setUniformb("shadowPass", true);
    shader.setUniformb("emissive", false);
    shader.setUniformb("shadowsEnabled", false);
    shader.setUniformb("hasPeelDepth", false);
    shader.setUniformMat4f("projection", &projection);
    shader.setUniformMat4f("view", &view);
    shader.setUniformMat4f("model", &identity);

    glBindFramebuffer(GL_FRAMEBUFFER, shadows.fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadows.depthArray, 0, 0);
    setDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glClear(GL_DEPTH_BUFFER_BIT);
    drawOpaqueLevelSets(shader, session, cubeMesh, activeColour, inactiveColour);

    glClearColor(0.f, 0.f, 0.f, 0.f);
    int layerCount = 0;
    for (int peel = 0; peel < maxDepthPeels; ++peel) {
        const unsigned int writeDepth = shadows.depth[peel % 2];
        blitDepth(shadows.fbo, shadows.depthArray, true, 0, shadows.readFbo, writeDepth, false, 0, size);

        glBindFramebuffer(GL_FRAMEBUFFER, shadows.fbo);
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, shadows.alphaArray, 0, peel);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, writeDepth, 0);
        setDrawBuffer(GL_COLOR_ATTACHMENT0);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glClear(GL_COLOR_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        shader.setUniformb("hasPeelDepth", peel > 0);
        glBindTexture(GL_TEXTURE_2D, peel > 0 ? shadows.depth[(peel + 1) % 2] : shadows.dummyDepth);

        const bool drew = drawPeel(shadows.query, [&] {
            drawTransparentLevelSets(shader, session, cubeMesh, activeColour, inactiveColour);
        });
        if (!drew) break;

        // The winning depth is copied with the array unbound from every sampler.
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindFramebuffer(GL_FRAMEBUFFER, shadows.fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
        blitDepth(shadows.readFbo, writeDepth, false, 0, shadows.fbo, shadows.depthArray, true, peel + 1, size);
        layerCount = peel + 1;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, shadows.fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    shader.setUniformb("shadowPass", false);
    shader.setUniformb("hasPeelDepth", false);
    glClearColor(1.f, 1.f, 1.f, 0.f);
    glActiveTexture(GL_TEXTURE0);
    return {true, layerCount};
}

void errorCallback(int error, const char* description) {
    std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
}

void drawSky(ShaderProgram& shader, unsigned int vao, const RenderState& state, const glm::mat4& invViewProj, float viewportWidth, float viewportHeight) {
    const GLboolean cull = glIsEnabled(GL_CULL_FACE);
    GLint previousProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_BLEND);
    glUseProgram(shader.program.get());
    shader.setUniform3f("zenithColour", state.skyZenith);
    shader.setUniform3f("groundColour", state.skyGround);
    shader.setUniform3f("cameraPosition", state.cam.getPos());
    glm::mat4 inverse = invViewProj;
    shader.setUniformMat4f("invViewProj", &inverse);
    shader.setUniform2f("viewportSize", viewportWidth, viewportHeight);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    if (cull) glEnable(GL_CULL_FACE);
    if (previousProgram != 0) glUseProgram(static_cast<GLuint>(previousProgram));
}

void drawFullscreenTexture(ShaderProgram& shader, unsigned int vao, unsigned int texture) {
    glUseProgram(shader.program.get());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

}

bool DepthPeelBuffers::ensure(int newWidth, int newHeight, int peelCount) {
    if (newWidth <= 0 || newHeight <= 0) return false;
    if (peelCount < 1) peelCount = 1;
    if (peelCount > kMaxDepthPeelCap) peelCount = kMaxDepthPeelCap;

    const bool first = fbo == 0;
    const bool sizeChanged = !first && (width != newWidth || height != newHeight);
    if (!first && !sizeChanged && static_cast<int>(colour.size()) == peelCount) return true;

    if (first) {
        glGenFramebuffers(1, &fbo);
        glGenTextures(2, depth);
        glGenTextures(1, &dummyDepth);
        glGenQueries(1, &query);
        glBindTexture(GL_TEXTURE_2D, dummyDepth);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 1, 1, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
        bindDepthTexture(dummyDepth);

        const float triangle[] = {-1.f, -1.f, 3.f, -1.f, -1.f, 3.f};
        glGenVertexArrays(1, &compositeVao);
        glGenBuffers(1, &compositeVbo);
        glBindVertexArray(compositeVao);
        glBindBuffer(GL_ARRAY_BUFFER, compositeVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(triangle), triangle, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
        glBindVertexArray(0);
    }

    if (first || sizeChanged) {
        for (unsigned int texture : depth) {
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, newWidth, newHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
            bindDepthTexture(texture);
        }
    }

    int have = static_cast<int>(colour.size());
    if (have > peelCount) {
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(have - peelCount, colour.data() + peelCount);
        colour.resize(static_cast<std::size_t>(peelCount));
        have = peelCount;
    }
    if (have < peelCount) {
        colour.resize(static_cast<std::size_t>(peelCount));
        glGenTextures(peelCount - have, colour.data() + have);
    }
    const int uploadFrom = (first || sizeChanged) ? 0 : have;
    for (int index = uploadFrom; index < peelCount; ++index) {
        glBindTexture(GL_TEXTURE_2D, colour[static_cast<std::size_t>(index)]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, newWidth, newHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    if (first) {
        glGenFramebuffers(1, &sceneFbo);
        glGenTextures(1, &sceneColour);
        glGenTextures(1, &sceneDepth);
    }
    if (first || sizeChanged) {
        glBindTexture(GL_TEXTURE_2D, sceneColour);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, newWidth, newHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, sceneDepth);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, newWidth, newHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
        bindDepthTexture(sceneDepth);
        glBindFramebuffer(GL_FRAMEBUFFER, sceneFbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneColour, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, sceneDepth, 0);
        setDrawBuffer(GL_COLOR_ATTACHMENT0);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        const bool sceneComplete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        if (!sceneComplete) {
            if (!reportedIncomplete) {
                std::cerr << "Scene framebuffer is incomplete\n";
                reportedIncomplete = true;
            }
            return false;
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colour[0], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth[0], 0);
    const bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (!complete) {
        if (!reportedIncomplete) {
            std::cerr << "Depth peel framebuffer is incomplete\n";
            reportedIncomplete = true;
        }
        return false;
    }

    width = newWidth;
    height = newHeight;
    return true;
}

void DepthPeelBuffers::destroy() {
    if (fbo != 0) glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(2, depth);
    if (!colour.empty()) glDeleteTextures(static_cast<GLsizei>(colour.size()), colour.data());
    if (dummyDepth != 0) glDeleteTextures(1, &dummyDepth);
    if (query != 0) glDeleteQueries(1, &query);
    if (compositeVbo != 0) glDeleteBuffers(1, &compositeVbo);
    if (compositeVao != 0) glDeleteVertexArrays(1, &compositeVao);
    if (sceneFbo != 0) glDeleteFramebuffers(1, &sceneFbo);
    if (sceneColour != 0) glDeleteTextures(1, &sceneColour);
    if (sceneDepth != 0) glDeleteTextures(1, &sceneDepth);
    sceneFbo = 0;
    sceneColour = 0;
    sceneDepth = 0;
    fbo = 0;
    depth[0] = depth[1] = 0;
    colour.clear();
    width = 0;
    height = 0;
    dummyDepth = 0;
    query = 0;
    compositeVbo = 0;
    compositeVao = 0;
}

bool LightShadowBuffers::ensure(int newResolution, int peelCount) {
    if (newResolution <= 0) return false;
    if (peelCount < 1) peelCount = 1;
    if (peelCount > kMaxDepthPeelCap) peelCount = kMaxDepthPeelCap;

    const bool first = fbo == 0;
    const bool sizeChanged = !first && resolution != newResolution;
    const bool countChanged = !first && peelSlots != peelCount;
    if (!first && !sizeChanged && !countChanged) return true;

    if (first) {
        glGenFramebuffers(1, &fbo);
        glGenFramebuffers(1, &readFbo);
        glGenTextures(2, depth);
        glGenTextures(1, &depthArray);
        glGenTextures(1, &alphaArray);
        glGenTextures(1, &dummyDepth);
        glGenQueries(1, &query);
        glBindTexture(GL_TEXTURE_2D, dummyDepth);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 1, 1, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
        bindDepthTexture(dummyDepth);
    }

    if (first || sizeChanged) {
        for (unsigned int texture : depth) {
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, newResolution, newResolution, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
            bindDepthTexture(texture);
        }
    }

    if (first || sizeChanged || countChanged) {
        glBindTexture(GL_TEXTURE_2D_ARRAY, depthArray);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT24, newResolution, newResolution, peelCount + 1,
                     0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
        bindArraySampler(true);
        glBindTexture(GL_TEXTURE_2D_ARRAY, alphaArray);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R8, newResolution, newResolution, peelCount, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        bindArraySampler(false);
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, alphaArray, 0, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth[0], 0);
    setDrawBuffer(GL_COLOR_ATTACHMENT0);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    const bool peelComplete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

    const auto depthOnly = [](unsigned int target, unsigned int name, bool array, int layer) {
        glBindFramebuffer(GL_FRAMEBUFFER, target);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
        attachDepth(GL_FRAMEBUFFER, name, array, layer);
        setDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    };
    const bool opaqueComplete = depthOnly(fbo, depthArray, true, 0);
    const bool blitComplete = depthOnly(readFbo, depth[0], false, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (!peelComplete || !opaqueComplete || !blitComplete) {
        if (!reportedIncomplete) {
            std::cerr << "Light shadow framebuffer is incomplete\n";
            reportedIncomplete = true;
        }
        return false;
    }

    resolution = newResolution;
    peelSlots = peelCount;
    return true;
}

void LightShadowBuffers::bindForLighting(bool useLayers) {
    if (dummyDepthArray == 0) {
        glGenTextures(1, &dummyDepthArray);
        glBindTexture(GL_TEXTURE_2D_ARRAY, dummyDepthArray);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT24, 1, 1, 1, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
        bindArraySampler(true);
        glGenTextures(1, &dummyAlphaArray);
        glBindTexture(GL_TEXTURE_2D_ARRAY, dummyAlphaArray);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R8, 1, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        bindArraySampler(false);
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    }

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D_ARRAY, useLayers && depthArray != 0 ? depthArray : dummyDepthArray);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D_ARRAY, useLayers && alphaArray != 0 ? alphaArray : dummyAlphaArray);
    glActiveTexture(GL_TEXTURE0);
}

void LightShadowBuffers::destroy() {
    if (fbo != 0) glDeleteFramebuffers(1, &fbo);
    if (readFbo != 0) glDeleteFramebuffers(1, &readFbo);
    glDeleteTextures(2, depth);
    if (depthArray != 0) glDeleteTextures(1, &depthArray);
    if (alphaArray != 0) glDeleteTextures(1, &alphaArray);
    if (dummyDepth != 0) glDeleteTextures(1, &dummyDepth);
    if (dummyDepthArray != 0) glDeleteTextures(1, &dummyDepthArray);
    if (dummyAlphaArray != 0) glDeleteTextures(1, &dummyAlphaArray);
    if (query != 0) glDeleteQueries(1, &query);
    fbo = 0;
    readFbo = 0;
    depth[0] = depth[1] = 0;
    depthArray = 0;
    alphaArray = 0;
    dummyDepth = 0;
    dummyDepthArray = 0;
    dummyAlphaArray = 0;
    query = 0;
    resolution = 0;
    peelSlots = 0;
}

bool Application::init() {
    glfwSetErrorCallback(errorCallback);

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    glfwReady = true;
#ifdef __EMSCRIPTEN__
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_SAMPLES, 0);
    glfwWindowHint(GLFW_STENCIL_BITS, 0);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

    const int windowWidth = 800;
    const int windowHeight = 600;
    window = glfwCreateWindow(windowWidth, windowHeight, "TraitVis", nullptr, nullptr);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        shutdown();
        return false;
    }
    glfwMakeContextCurrent(window);
#ifndef __EMSCRIPTEN__
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        shutdown();
        return false;
    }
#endif
    glReady = true;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(kImGuiGlslVersion);
    imguiReady = true;
    ImFont* font = io.Fonts->AddFontFromFileTTF("assets/fonts/OpenSans-VariableFont_wdth,wght.ttf", 24.0f);
    if (font != nullptr) io.FontDefault = font;

    glfwSwapInterval(1);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glViewport(0, 0, windowWidth, windowHeight);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glEnable(GL_DEPTH_TEST);
    glClearColor(1.f, 1.f, 1.f, 0.f);

    try {
        lighting.emplace(std::vector<ShaderProgram::ShaderSource>{
            {GL_VERTEX_SHADER, "assets/litSurface.vert"},
            {GL_FRAGMENT_SHADER, "assets/litSurface.frag"}
        });
        composite.emplace(std::vector<ShaderProgram::ShaderSource>{
            {GL_VERTEX_SHADER, "assets/compositeLayer.vert"},
            {GL_FRAGMENT_SHADER, "assets/compositeLayer.frag"}
        });
        sky.emplace(std::vector<ShaderProgram::ShaderSource>{
            {GL_VERTEX_SHADER, "assets/compositeLayer.vert"},
            {GL_FRAGMENT_SHADER, "assets/sky.frag"}
        });
        distanceField.emplace();
        session.gpuDistanceField = [this](std::span<const TraitPoint> points, const MultiField& field) {
            return distanceField->compute(points, field);
        };
    } catch (const std::runtime_error& error) {
        std::cerr << "Fatal: " << error.what() << std::endl;
        shutdown();
        return false;
    }

    rendering.emplace(RenderState{
        {glm::vec3(0.f), glm::vec3(1.f, 1.f, 1.f), glm::vec3(0.f, 1.f, 0.f)},
        windowWidth,
        windowHeight
    });
    gui.emplace(session, *rendering);
    glfwSetWindowUserPointer(window, &*rendering);
#ifdef __EMSCRIPTEN__
    // The browser can drop pointer lock on its own (Escape, or focus leaving the tab).
    emscripten_set_pointerlockchange_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, window, false, onPointerLockChange);
    emscripten_set_pointerlockerror_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, window, false, onPointerLockError);
    emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, window, true, onCapturedMouseMove);
    previousKeyCallback = glfwSetKeyCallback(window, webKeyCallback);
#endif
    previousCursorCallback = glfwSetCursorPosCallback(window, cursorCallback);
    previousMouseButtonCallback = glfwSetMouseButtonCallback(window, mouseButtonCallback);
    previousScrollCallback = glfwSetScrollCallback(window, scrollCallback);
    cube.emplace(uploadMesh(std::span<const float>(cubeVerticesWithNormals)));
    const float skyTriangle[] = {-1.f, -1.f, 3.f, -1.f, -1.f, 3.f};
    glGenVertexArrays(1, &skyVao);
    glGenBuffers(1, &skyVbo);
    glBindVertexArray(skyVao);
    glBindBuffer(GL_ARRAY_BUFFER, skyVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyTriangle), skyTriangle, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glBindVertexArray(0);

    glUseProgram(lighting->program.get());
    lighting->setUniformf("objectTransparency", 1.f);
    lighting->setUniformb("useInstanceModel", false);
    lighting->setUniformb("hasPeelDepth", false);
    lighting->setUniformb("shadowPass", false);
    lighting->setUniformb("emissive", false);
    lighting->setUniformb("shadowsEnabled", false);
    lighting->setUniformb("pointLight", false);
    lighting->setUniformi("lightLayerCount", 0);
    lighting->setUniformi("peelDepth", 0);
    lighting->setUniformi("lightDepth", 1);
    lighting->setUniformi("lightAlpha", 2);
    lightShadows.bindForLighting(false);
    glUseProgram(composite->program.get());
    composite->setUniformi("layer", 0);
    traitvisBindApplication(this);
    return true;
}

void Application::frame() {
    RenderState& state = *rendering;
    state.updateTime();
#ifdef __EMSCRIPTEN__
    // The first measurement during init can see the 800x600 canvas instead of #view.
    // Retry until the canvas actually fills the view; the observer keeps later resizes.
    static bool webViewFitted = false;
    if (!webViewFitted) emscripten::glfw3::MakeCanvasResizable(window, "#view");
#endif
    glfwPollEvents();
#ifdef __EMSCRIPTEN__
    if (!webViewFitted) {
        webViewFitted = EM_ASM_INT({
            const view = document.getElementById('view');
            const canvas = document.getElementById('canvas');
            if (!view || !canvas) return 0;
            const viewRect = view.getBoundingClientRect();
            const canvasRect = canvas.getBoundingClientRect();
            const fillsWidth = Math.abs(viewRect.width - window.innerWidth) < 2;
            const matched = Math.abs(viewRect.width - canvasRect.width) < 2
                && Math.abs(viewRect.height - canvasRect.height) < 2;
            return fillsWidth && matched && viewRect.height > 2 ? 1 : 0;
        }) != 0;
    }
#endif

    ImGuiIO& io = ImGui::GetIO();
    if (state.enableCam || not io.WantCaptureKeyboard) processInput(window);

#ifndef __EMSCRIPTEN__
    if (not state.enableCam) {
        double xpos = 0.0;
        double ypos = 0.0;
        glfwGetCursorPos(window, &xpos, &ypos);
        state.cam.updateMousePos(xpos, ypos);
    }
#endif
    state.cam.updateCamera(state.deltaTime);

    glDisable(GL_SCISSOR_TEST);
    glViewport(state.windowWidth * (1 - state.renderWidthPercentage), 0, state.windowWidth * state.renderWidthPercentage, state.windowHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 proj = state.getProjectionMatrix();
    glm::mat4 view = state.cam.getViewMatrix();
    const glm::mat4 invViewProj = glm::inverse(proj * view);
    const float viewWidth = static_cast<float>(state.windowWidth) * state.renderWidthPercentage;
    const float viewHeight = static_cast<float>(state.windowHeight);
    drawSky(*sky, skyVao, state, invViewProj, viewWidth, viewHeight);

    ShaderProgram& lightingShader = *lighting;
    glUseProgram(lightingShader.program.get());
    lightingShader.setUniform3f("ambientLightColour", state.ambientLightColour);
    lightingShader.setUniformf("ambientLightStrength", state.ambientLightStrength);
    lightingShader.setUniform3f("diffuseLightColour", state.diffuseLightColour);
    lightingShader.setUniformf("diffuseLightStrength", state.diffuseLightStrength);
    lightingShader.setUniformb("hasPeelDepth", false);
    lightingShader.setUniformb("shadowPass", false);
    lightingShader.setUniformb("emissive", false);
    lightingShader.setUniformb("shadowsEnabled", false);
    lightingShader.setUniformb("pointLight", state.pointLight);
    glm::vec3 clipNormal(0.f);
    if (state.clipAxis == 0) clipNormal.x = 1.f;
    else if (state.clipAxis == 2) clipNormal.z = 1.f;
    else clipNormal.y = 1.f;
    lightingShader.setUniformb("clipEnabled", state.clipEnabled);
    lightingShader.setUniform3f("clipNormal", clipNormal);
    lightingShader.setUniformf("clipDistance", state.clipPosition);

    LitFromLight lit;
    if (state.pointLight) {
        glm::vec3 boundsMin(0.f);
        glm::vec3 boundsMax(1.f);
        datasetAabb(session, boundsMin, boundsMax);
        const LightFrustum lightFrustum = fitPointLight(state.lightPosition, boundsMin, boundsMax);
        glm::mat4 lightViewProjection = lightFrustum.projection * lightFrustum.view;
        lightingShader.setUniform3f("lightPosition", state.lightPosition);
        lightingShader.setUniformf("lightRange", state.lightRange);
        lightingShader.setUniformf("shadowFloor", state.shadowFloor);
        lightingShader.setUniformMat4f("lightViewProjection", &lightViewProjection);
        lightShadows.bindForLighting(false);
        if (state.shadowsEnabled && anyDrawnLevelSet(session)) {
            lit = renderLightShadows(lightShadows, session, lightingShader, *cube, lightFrustum,
                                      state.maxDepthPeels, colourGreen, colourRed);
        }
    } else {
        glm::vec3 direction = state.diffuseLightDirection;
        if (glm::dot(direction, direction) < 1e-8f) direction = glm::vec3(0.f, 0.f, -1.f);
        lightingShader.setUniform3f("diffuseLightDirection", glm::normalize(direction));
    }

    int fbWidth = 0;
    int fbHeight = 0;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    bool anyTransparent = false;
    for (const auto& levelSet : session.levelSets)
        if (levelSet.active && levelSet.transparency > 0.f && levelSet.transparency < 1.f) anyTransparent = true;

    glViewport(state.windowWidth * (1.f - state.renderWidthPercentage), 0,
               state.windowWidth * state.renderWidthPercentage, state.windowHeight);
#ifndef __EMSCRIPTEN__
    if (state.drawWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
    if (state.backFaceCulling) glEnable(GL_CULL_FACE);
    else glDisable(GL_CULL_FACE);

    lightingShader.setUniformMat4f("projection", &proj);
    lightingShader.setUniformMat4f("view", &view);
    lightingShader.setUniformMat4f("model", &defaultModel);
    lightingShader.setUniformb("hasPeelDepth", false);
    lightingShader.setUniformb("shadowPass", false);
    lightingShader.setUniformb("shadowsEnabled", lit.ready);
    lightingShader.setUniformi("lightLayerCount", lit.layers);
    if (lit.ready) lightShadows.bindForLighting(true);

    glActiveTexture(GL_TEXTURE0);
    if (peels.dummyDepth != 0) glBindTexture(GL_TEXTURE_2D, peels.dummyDepth);

    glDepthMask(GL_TRUE);
    const bool useSceneTarget = anyTransparent && peels.ensure(fbWidth, fbHeight, state.maxDepthPeels);
    if (useSceneTarget) {
        glBindFramebuffer(GL_FRAMEBUFFER, peels.sceneFbo);
        glClearColor(1.f, 1.f, 1.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        drawSky(*sky, skyVao, state, invViewProj, static_cast<float>(fbWidth), static_cast<float>(fbHeight));
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawOpaqueLevelSets(lightingShader, session, *cube, colourGreen, colourRed);
    if (state.pointLight)
        drawLightMarker(lightingShader, *cube, state.lightPosition, state.lightSize, state.diffuseLightColour, defaultModel);

    if (useSceneTarget) {
        // A colour blit onto the canvas is invalid when the formats differ, and it
        // was dropping every surface once one of them became transparent.
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        drawFullscreenTexture(*composite, peels.compositeVao, peels.sceneColour);
        glEnable(GL_DEPTH_TEST);
        glUseProgram(lightingShader.program.get());
    }

    int peelCount = 0;
    if (anyTransparent && peels.ensure(fbWidth, fbHeight, state.maxDepthPeels)) {
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        const glm::vec4 sceneClear = glm::vec4(1.f, 1.f, 1.f, 0.f);
        glClearColor(0.f, 0.f, 0.f, 0.f);

        for (int peel = 0; peel < state.maxDepthPeels; ++peel) {
            const unsigned int writeDepth = peels.depth[peel % 2];
            glBindFramebuffer(GL_FRAMEBUFFER, peels.fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, peels.colour[peel], 0);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, writeDepth, 0);
            // Depth-only blit. A colour read buffer makes this call fail on WebGL,
            // which leaves the peel depth at 0 and rejects every fragment.
            glBindFramebuffer(GL_READ_FRAMEBUFFER, peels.sceneFbo);
            glReadBuffer(GL_NONE);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, peels.fbo);
            setDrawBuffer(GL_NONE);
            glBlitFramebuffer(0, 0, fbWidth, fbHeight, 0, 0, fbWidth, fbHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
            setDrawBuffer(GL_COLOR_ATTACHMENT0);
            glBindFramebuffer(GL_FRAMEBUFFER, peels.fbo);
            glClear(GL_COLOR_BUFFER_BIT);

            glActiveTexture(GL_TEXTURE0);
            lightingShader.setUniformb("hasPeelDepth", peel > 0);
            glBindTexture(GL_TEXTURE_2D, peel > 0 ? peels.depth[(peel + 1) % 2] : peels.dummyDepth);

            const bool drew = drawPeel(peels.query, [&] {
                drawTransparentLevelSets(lightingShader, session, *cube, colourGreen, colourRed);
            });
            if (!drew) break;
            peelCount = peel + 1;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(sceneClear.r, sceneClear.g, sceneClear.b, sceneClear.a);
        glActiveTexture(GL_TEXTURE0);
        if (peelCount > 0) {
#ifndef __EMSCRIPTEN__
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            for (int peel = peelCount - 1; peel >= 0; --peel)
                drawFullscreenTexture(*composite, peels.compositeVao, peels.colour[peel]);
            glEnable(GL_DEPTH_TEST);
#ifndef __EMSCRIPTEN__
            if (state.drawWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
#endif
        }
        if (state.backFaceCulling) glEnable(GL_CULL_FACE);
        else glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glDepthMask(GL_TRUE);
        glUseProgram(lightingShader.program.get());
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    if (state.enableCam) {
        // The captured cursor position is not a point on the panels. Parking it
        // off-screen stops sliders and buttons from following the look direction.
        io.ConfigFlags |= ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoMouseCursorChange;
        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    } else {
        io.ConfigFlags &= ~(ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoMouseCursorChange);
    }
    ImGui::NewFrame();
    gui->render();
    gui->renderLightingControls();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
}

void Application::shutdown() {
    if (shutDown) return;
    shutDown = true;
    traitvisBindApplication(nullptr);

    // GPU ids have to be deleted while the context is still current.
    if (glReady) {
        if (imguiReady) {
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
            imguiReady = false;
        }
        peels.destroy();
        lightShadows.destroy();
        cube.reset();
        session.clearHistory();
        session.levelSets.clear();
        session.gpuDistanceField = nullptr;
        distanceField.reset();
        lighting.reset();
        composite.reset();
        sky.reset();
        if (skyVbo != 0) glDeleteBuffers(1, &skyVbo);
        if (skyVao != 0) glDeleteVertexArrays(1, &skyVao);
        skyVbo = 0;
        skyVao = 0;
        glReady = false;
    }
    gui.reset();
    if (window != nullptr) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    if (glfwReady) {
        glfwTerminate();
        glfwReady = false;
    }
}

bool Application::shouldClose() const {
    return window == nullptr || glfwWindowShouldClose(window);
}
