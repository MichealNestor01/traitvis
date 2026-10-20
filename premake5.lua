-- premake5.lua
workspace "TraitVis"
    configurations { "Debug", "Release", "ThreadSanitize", "Web" }
    cppdialect "C++20"

    filter "configurations:ThreadSanitize"
        symbols "On"
        buildoptions { "-fsanitize=thread" }
        linkoptions { "-fsanitize=thread" }
    -- symbols "On" stops Premake emitting a bare -s. emcc treats that as the
    -- start of a setting. -g0 drops the debug info that symbols "On" adds.
    filter "configurations:Web"
        defines { "NDEBUG" }
        symbols "On"
        buildoptions { "-O2", "-g0", "-msimd128", "--use-port=contrib.glfw3" }
        linkoptions { "-O2", "-g0", "--use-port=contrib.glfw3" }
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
        "src/lib/meshTools/objMesh.cpp",
        "src/lib/meshTools/objMesh.hpp",
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
        "src/lib/meshTools/gpuMesh.cpp",
        "src/lib/meshTools/gpuMesh.hpp",
        "src/lib/meshTools/gpuDistanceField.cpp",
        "src/lib/meshTools/gpuDistanceField.hpp",
        "src/lib/meshTools/cube.hpp",
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

    -- Link flags that name this executable stay here. The workspace Web filter
    -- must not export symbols or preload files, or core and tests fail to link.
    filter "configurations:Web"
        targetname "traitvis"
        targetextension ".html"
        defines { "IMGUI_IMPL_OPENGL_ES3" }
        removeincludedirs { "third_party/glfw/include" }
        removelinks { "glfw3", "X11", "Xrandr", "Xinerama", "Xi", "Xxf86vm", "Xcursor", "dl", "GL", "pthread" }
        removefiles { "third_party/glad/src/glad.c" }
        linkoptions {
            "-sMIN_WEBGL_VERSION=2", "-sMAX_WEBGL_VERSION=2",
            "-sALLOW_MEMORY_GROWTH=1", "-sMAXIMUM_MEMORY=4GB",
            "-sSTACK_SIZE=1MB",
            "@%{wks.location}/web/exports.rsp",
            "--preload-file %{wks.location}/assets@/assets",
            "--shell-file %{wks.location}/web/shell.html",
        }
    filter {}

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

    filter "configurations:Web"
        removelinks { "pthread" }
        linkoptions { "-sNODERAWFS=1" }
    filter {}

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