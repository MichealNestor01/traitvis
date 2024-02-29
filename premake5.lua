-- premake5.lua
workspace "TraitVis"
    configurations { "Debug", "Release" }

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
    }

    files {
        "src/main/*.cpp", 
        "src/main/*.hpp", 
        "src/lib/**.cpp",
        "src/lib/**.hpp",
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

project "read_binary"
    location "_build_/read_binary"
    kind "ConsoleApp"
    language "C++"
    targetdir "bin/%{cfg.buildcfg}"

    
    files { 
        "src/read_binary/*.cpp",
        "src/read_binary/*.hpp",
        "src/lib/parsingData/*.hpp", 
        "src/lib/parsingData/*.cpp" 
    }

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
            "src/lib/multiField/*.hpp",
            "src/lib/multiField/*.cpp",
            "src/lib/parsingData/*.hpp", 
            "src/lib/parsingData/*.cpp", 
        }
    
        filter "configurations:Debug"
            defines { "DEBUG" }
            symbols "On"
    
        filter "configurations:Release"
            defines { "NDEBUG" }
            optimize "On"