workspace "Presentation"
    configurations { "debug", "release" }

project "plugin_network"
    language "C"
    location "."
    kind "SharedLib"

    objdir "bin-int/%{prj.name}"
    targetdir "bin/plugins"
    targetprefix ""

    files {
        "plugin_network.c"
    }

    includedirs {
        "AnimatedPresentation/app/include",
        "AnimatedPresentation/core/include",
        "turbospork",
        "turbospork/include",
        "turbospork/third_party"
    }

    links {
        "./lib/TurboSpork",
    }

    -- TODO: platform independent?
    buildoptions { "-fPIC" }

    filter "system:linux"
        linkoptions { "-Lbin", "-l:core.so" }

    filter { }

    warnings "Extra"
    architecture "x64"
    toolset "clang"

    filter "configurations:debug"
        symbols "On"
        defines {
            "DEBUG"
        }

    filter "configurations:release"
        optimize "On"

        defines { "NDEBUG" }

    filter "system:linux"
            links {
                "m", "X11", "GL", "GLX", "dl"
            }

    filter "system:windows"
        systemversion "latest"

        links {
            "gdi32", "kernel32", "user32", "winmm", "opengl32"
        }

