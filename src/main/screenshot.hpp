#pragma once

#include <optional>
#include <string>
#include <vector>

// The framebuffer as top-row-first RGB. Captured before a save dialog opens,
// so the dialog covering the window is not what gets written.
struct CapturedFrame {
    int width = 0;
    int height = 0;
    std::vector<unsigned char> rgb;
};

[[nodiscard]] std::optional<CapturedFrame> captureFramebuffer();

#ifdef __EMSCRIPTEN__
// WebGL2 saves through the page canvas rather than a framebuffer read.
// EM_JS gives this C linkage.
extern "C" void downloadCanvasPng();
#endif
