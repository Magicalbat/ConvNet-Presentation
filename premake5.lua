workspace "Presentation"
    configurations { "debug", "release" }

project "plugin_mnist"
    language "C"
    location "."
    kind "SharedLib"

    objdir "bin-int/%{prj.name}"
    targetdir "bin/plugins"
    targetprefix ""

    files {
        "plugin_mnist.c"
    }

    includedirs {
        "AnimatedPresentation/app/include",
        "AnimatedPresentation/core/include",
        "turbospork/include",
        "turbospork/third_party"
    }

    links {
        "lib/TurboSpork",
    }

    filter "system:linux"
        linkoptions { "-Lbin", "-l:core.so" }

    filter "system:windows"
        links { "bin/core" }

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
            pic "On"
            links {
                "m", "X11", "GL", "GLX", "dl"
            }

    filter "system:windows"
        systemversion "latest"

        links {
            "gdi32", "kernel32", "user32", "winmm", "opengl32", "bcrypt"
        }

