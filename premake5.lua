-- premake5.lua
workspace "TraitVis"
    configurations { "Debug", "Release", "ThreadSanitize" }
    cppdialect "C++20"

    filter "configurations:ThreadSanitize"
        symbols "On"
        buildoptions { "-fsanitize=thread" }
        linkoptions { "-fsanitize=thread" }
    filter {}

project "GoogleTest"
    location "_build_/googletest"
    kind "StaticLib"
    language "C++"
    targetdir "bin/%{cfg.buildcfg}"

    files { 
        "third_party/googletest/googletest/src/gtest-all.cc",
        "third_party/googletest/googletest/src/gtest_main.cc"
    }

    includedirs {
        "third_party/googletest/googletest",
        "third_party/googletest/googletest/include"
    }

    filter "configurations:Debug"
        defines {"DEBUG"}
        symbols "On"

    filter "configurations:Release"
        defines {"NDEBUG"}
        optimize "On"

-- GL-free library: parsing, the multifield model, distance fields, marching cubes,
-- and the shader file reader. No GL, GLFW, or ImGui on the include path, so a
-- platform include in here fails the build.
project "core"
    location "_build_/core"
    kind "StaticLib"
    language "C++"
    targetdir "bin/%{cfg.buildcfg}"

    files {
        "src/lib/parsingData/**",
        "src/lib/multiField/**",
        "src/lib/levelSets/**",
        "src/lib/marchingCubes/**",
        "src/lib/camera/**",
        "src/lib/util/**",
        "src/lib/shaderTools/shaderSource.*",
    }

    includedirs {
        "third_party/glm",
    }

    filter "configurations:Debug"
        defines {"DEBUG"}
        symbols "On"

    filter "configurations:Release"
        defines {"NDEBUG"}
        optimize "On"

project "main"
    location "_build_/main"
    kind "ConsoleApp"
    language "C++"
    targetdir "bin/%{cfg.buildcfg}"

    --include libraries
    includedirs {
        "third_party/glfw/include",
        "third_party/glad/include",
        "third_party/glm",
        "third_party/imgui",
        "third_party/imgui/backends"
    }

    libdirs {
        "third_party/glfw/src"
    }

    links {
        "glfw3", 
        "X11", 
        "Xrandr", 
        "Xinerama", 
        "Xi", 
        "Xxf86vm", 
        "Xcursor", 
        "pthread", 
        "dl",
        "GL",
        "core",
    }

    files {
        "src/main/*.cpp", 
        "src/main/*.hpp", 
        "src/lib/gui/**",
        "src/lib/meshTools/**",
        "src/lib/shaderTools/shaderProgram.cpp",
        "src/lib/shaderTools/shaderProgram.hpp",
        "third_party/glad/src/glad.c",
        "third_party/imgui/*.cpp",
        "third_party/imgui/backends/imgui_impl_glfw.cpp",
        "third_party/imgui/backends/imgui_impl_opengl3.cpp",
    }

    filter "configurations:Debug"
        defines {"DEBUG"}
        symbols "On"

    filter "configurations:Release"
        defines {"NDEBUG"}
        optimize "On"

project "tests"
    location "_build_/tests"
    kind "ConsoleApp"
    language "C++"
    targetdir "bin/%{cfg.buildcfg}"

    files {
        "tests/**.cpp",
    }

    links {
        "GoogleTest",
        "core",
        "pthread",
    }

    includedirs {
        "third_party/googletest/googletest/include",
        "third_party/glm",
        "src/**",
        "tests"
    }

    filter "configurations:Debug"
        defines {"DEBUG"}
        symbols "On"

    filter "configurations:Release"
        defines {"NDEBUG"}
        optimize "On"

project "read_binary"
    location "_build_/read_binary"
    kind "ConsoleApp"
    language "C++"
    targetdir "bin/%{cfg.buildcfg}"

    
    files { 
        "src/read_binary/*.cpp",
        "src/read_binary/*.hpp",
    }

    links { "core" }

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

project "read_multifield"
    location "_build_/read_multifield"
    kind "ConsoleApp"
    language "C++"
    targetdir "bin/%{cfg.buildcfg}"

    
    files { 
        "src/read_multifield/*.cpp",
        "src/read_multifield/*.hpp",
    }

    links { "core" }

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"