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

project "read_multifield"
        location "_build_/read_multifield"
        kind "ConsoleApp"
        language "C++"
        targetdir "bin/%{cfg.buildcfg}"
    
        
        files { 
            "src/read_multifield/*.cpp",
            "src/read_multifield/*.hpp",
            "src/lib/**.hpp", 
            "src/lib/**.cpp" 
        }
    
        filter "configurations:Debug"
            defines { "DEBUG" }
            symbols "On"
    
        filter "configurations:Release"
            defines { "NDEBUG" }
            optimize "On"

project "vtk_tests"
        location "_build_/vtk_tests"
        kind "ConsoleApp"
        language "C++"
        targetdir "bin/%{cfg.buildcfg}"
    
        -- add vtk to include dirs
        includedirs { "/usr/include/vtk" }

        -- add vtk library dirs
        libdirs { "/usr/lib64/vtk" }

        -- link vtk libraries
        links { "vtkCommonCore", "vtksys" }
        
        files { 
            "src/vtk_tests/*.cpp",
            "src/vtk_tests/*.hpp",
            "src/lib/**.hpp", 
            "src/lib/**.cpp" 
        }
    
        filter "configurations:Debug"
            defines { "DEBUG" }
            symbols "On"
    
        filter "configurations:Release"
            defines { "NDEBUG" }
            optimize "On"