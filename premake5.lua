-- premake5.lua
workspace "TraitVis"
    configurations { "Debug", "Release" }

project "read_binary"
    location "_build_/read_binary"
    kind "ConsoleApp"
    language "C++"
    targetdir "bin/%{cfg.buildcfg}"

    
    files { 
        "src/read_binary/*.cpp",
        "src/read_binary/*.hpp",
        "src/lib/**.hpp", 
        "src/lib/**.cpp" 
    }

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"