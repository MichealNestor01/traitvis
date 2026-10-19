#include "screenshot.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

EM_JS(void, downloadCanvasPng, (), {
    var canvas = Module["canvas"];
    if (!canvas) return;
    canvas.toBlob(function(blob) {
        if (!blob) return;
        var url = URL.createObjectURL(blob);
        var link = document.createElement("a");
        link.href = url;
        link.download = "traitvis.png";
        link.click();
        URL.revokeObjectURL(url);
    });
});
#endif

std::optional<CapturedFrame> captureFramebuffer() {
    GLFWwindow* window = glfwGetCurrentContext();
    if (window == nullptr) return std::nullopt;
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    if (width <= 0 || height <= 0) return std::nullopt;

    GLint previousAlignment = 4;
    glGetIntegerv(GL_PACK_ALIGNMENT, &previousAlignment);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    const auto count = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 3;
    std::vector<unsigned char> bottomFirst(count);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, bottomFirst.data());
    glPixelStorei(GL_PACK_ALIGNMENT, previousAlignment);

    // glReadPixels starts at the bottom row. The PNG writer starts at the top.
    CapturedFrame frame;
    frame.width = width;
    frame.height = height;
    frame.rgb.resize(count);
    const auto stride = static_cast<std::size_t>(width) * 3;
    for (int y = 0; y < height; ++y) {
        const auto from = bottomFirst.begin() + static_cast<std::ptrdiff_t>((height - 1 - y) * stride);
        const auto to = frame.rgb.begin() + static_cast<std::ptrdiff_t>(y * stride);
        std::copy(from, from + static_cast<std::ptrdiff_t>(stride), to);
    }
    return frame;
}
