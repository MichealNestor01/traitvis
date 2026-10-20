#pragma once

// Desktop GL and WebGL2 share this header. Shader files do not carry a #version
// line; ShaderProgram prepends kGlslHeader for vertex and fragment stages.
#ifdef __EMSCRIPTEN__
    #include <GLES3/gl3.h>
    #include <emscripten.h>
    inline constexpr const char* kGlslHeader =
        "#version 300 es\n"
        "precision highp float;\n"
        "precision highp sampler2D;\n"
        "precision highp sampler2DArray;\n";
    inline constexpr const char* kImGuiGlslVersion = "#version 300 es";
    inline void setDrawBuffer(GLenum buffer) {
        const GLenum buffers[] = { buffer };
        glDrawBuffers(1, buffers);
    }
#else
    #include <glad/glad.h>
    inline constexpr const char* kGlslHeader = "#version 330 core\n";
    inline constexpr const char* kImGuiGlslVersion = "#version 130";
    inline void setDrawBuffer(GLenum buffer) { glDrawBuffer(buffer); }
#endif
