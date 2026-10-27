-- Export Compile Commands module for clangd support
require "ecc/ecc"

newoption
{
    trigger = "graphics",
    value = "OPENGL_VERSION",
    description = "version of OpenGL to build raylib against",
    allowed = {
        { "opengl11", "OpenGL 1.1"},
        { "opengl21", "OpenGL 2.1"},
        { "opengl33", "OpenGL 3.3"},
        { "opengl43", "OpenGL 4.3"},
    },
    default = "opengl33"
}

newoption
{
    trigger = "backend",
    value = "BACKEND_TYPE",
    description = "Backend Platform to use",
    allowed = {
        { "glfw", "GLFW"},
        { "rgfw", "RGFW"},
    },
    default = "glfw"
}

newoption
{
    trigger = "wayland",
    value = "WAYLAND",
    description = "build for wayland",
    allowed = {
        { "off", "Off"},
        { "on", "On"}
    },
    default = "off"
}

function download_progress(total, current)
    local ratio = current / total;
    ratio = math.min(math.max(ratio, 0), 1);
    local percent = math.floor(ratio * 100);
    print("Download progress (" .. percent .. "%/100%)")
end

function check_raylib()
    os.chdir("external")
    if(os.isdir("raylib-master") == false) then
        if(not os.isfile("raylib-master.zip")) then
            print("Raylib not found, downloading from github")
            local result_str, response_code = http.download("https://github.com/raysan5/raylib/archive/refs/heads/master.zip", "raylib-master.zip", {
                progress = download_progress,
                headers = { "From: Premake", "Referer: Premake" }
            })
        end
        print("Unzipping to " ..  os.getcwd())
        zip.extract("raylib-master.zip", os.getcwd())
        os.remove("raylib-master.zip")
    end
    os.chdir("../")
end

function build_externals()
     print("calling externals")
     check_raylib()
end

function platform_defines()
    filter {"options:backend=glfw"}
        defines{"PLATFORM_DESKTOP"}

    filter {"options:backend=rgfw"}
        defines{"PLATFORM_DESKTOP_RGFW"}

    filter {"options:graphics=opengl43"}
        defines{"GRAPHICS_API_OPENGL_43"}

    filter {"options:graphics=opengl33"}
        defines{"GRAPHICS_API_OPENGL_33"}

    filter {"options:graphics=opengl21"}
        defines{"GRAPHICS_API_OPENGL_21"}

    filter {"options:graphics=opengl11"}
        defines{"GRAPHICS_API_OPENGL_11"}

    filter {"options:wayland=off"}
        defines {"_GLFW_X11"}

    filter {"options:wayland=on"}
        defines {"_GLFW_WAYLAND"}

    filter {}
end

-- if you don't want to download raylib, then set this to false, and set the raylib dir to where you want raylib to be pulled from, must be full sources.
downloadRaylib = true
raylib_dir = "external/raylib-master"

workspaceName = path.getbasename(path.getdirectory(os.getcwd()));

if (os.isdir('build_files') == false) then
    os.mkdir('build_files')
end

if (os.isdir('external') == false) then
    os.mkdir('external')
end

workspace (workspaceName)
    location "../"
    configurations { "Debug", "Release"}
    platforms { "x64" }

    defaultplatform ("x64")

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

    filter { "platforms:x64" }
        architecture "x86_64"

    filter {}

    targetdir "bin/%{cfg.buildcfg}/"

if (downloadRaylib) then
    build_externals()
    end

    startproject(workspaceName)

    project (workspaceName)
        kind "ConsoleApp"
        location "../"
        targetdir "../bin/%{cfg.buildcfg}"

        filter {"action:gmake*"}
        filter{}

        vpaths
        {
            ["Header Files/*"] = { "../src/**.h"},
            ["Source Files/*"] = {"../src/**.c"},
            ["Game Resource Files/*"] = {"../resources/**"},
        }

        files {"../src/**.c", "../src/**.h", "../src/**.cpp"}

        filter{}

        includedirs { "../src" }
        includedirs { "../include" }

        links {"raylib"}

        cdialect "C17"

        includedirs {raylib_dir .. "/src" }

        flags { "ShadowedVariables"}
        platform_defines()

        links {"pthread", "m", "dl", "rt"}

        filter {"options:wayland=off"}
            links {"X11"}

        filter {"options:wayland=on"}
            links {"wayland-client", "wayland-cursor", "wayland-egl", "xkbcommon"}

        filter{}

        -- 1. Tell the compiler where the TFLite C headers are
        includedirs { "../include" }

        -- 2. Tell the linker where the TFLite shared library is
        libdirs { "../lib" }

        -- 3. Link TensorFlow Lite and the C++ standard library (required by OpenCV)
        links { "tensorflowlite_c", "stdc++" }

        -- 4. Inject OpenCV headers via pkg-config
        buildoptions { "`pkg-config --cflags opencv4`" }

        -- 5. Inject OpenCV libraries and the runtime path (rpath) for TFLite
        -- Note: Assuming your executable ends up in bin/Debug/, the relative path back up to lib/ is ../../lib
        linkoptions { 
            "`pkg-config --libs opencv4`",
            "-Wl,-rpath=../../lib" 
        }


    project "raylib"
        kind "StaticLib"

        platform_defines()

        location "../"

        --language "C"
        language "C++"
        targetdir "../bin/%{cfg.buildcfg}"

        filter {"options:wayland=on"}
            defines {"GLFW_LINUX_ENABLE_WAYLAND=TRUE" }

        filter {"options:wayland=on"}
            prebuildcommands {
                "@echo Generating Wayland protocols...",
                -- Core Wayland & Shell
                "@wayland-scanner client-header ../" .. raylib_dir .. "/src/external/glfw/deps/wayland/wayland.xml ../" .. raylib_dir .. "/src/wayland-client-protocol.h",
                "@wayland-scanner client-header ../" .. raylib_dir .. "/src/external/glfw/deps/wayland/xdg-shell.xml ../" .. raylib_dir .. "/src/xdg-shell-client-protocol.h",
                "@wayland-scanner client-header ../" .. raylib_dir .. "/src/external/glfw/deps/wayland/xdg-decoration-unstable-v1.xml ../" .. raylib_dir .. "/src/xdg-decoration-unstable-v1-client-protocol.h",

                -- Viewporter
                "@wayland-scanner client-header ../" .. raylib_dir .. "/src/external/glfw/deps/wayland/viewporter.xml ../" .. raylib_dir .. "/src/viewporter-client-protocol.h",

                -- Relative Pointer
                "@wayland-scanner client-header ../" .. raylib_dir .. "/src/external/glfw/deps/wayland/relative-pointer-unstable-v1.xml ../" .. raylib_dir .. "/src/relative-pointer-unstable-v1-client-protocol.h",
                -- Pointer Constraints
                "@wayland-scanner client-header ../" .. raylib_dir .. "/src/external/glfw/deps/wayland/pointer-constraints-unstable-v1.xml ../" .. raylib_dir .. "/src/pointer-constraints-unstable-v1-client-protocol.h",

                -- Fractional Scale
                "@wayland-scanner client-header ../" .. raylib_dir .. "/src/external/glfw/deps/wayland/fractional-scale-v1.xml ../" .. raylib_dir .. "/src/fractional-scale-v1-client-protocol.h",

                -- XDG Activation
                "@wayland-scanner client-header ../" .. raylib_dir .. "/src/external/glfw/deps/wayland/xdg-activation-v1.xml ../" .. raylib_dir .. "/src/xdg-activation-v1-client-protocol.h",
                -- Idle Inhibit
                "@wayland-scanner client-header ../" .. raylib_dir .. "/src/external/glfw/deps/wayland/idle-inhibit-unstable-v1.xml ../" .. raylib_dir .. "/src/idle-inhibit-unstable-v1-client-protocol.h",
            }
        filter {}

        filter{}

        includedirs {raylib_dir .. "/src", raylib_dir .. "/src/external/glfw/include" }
        vpaths
        {
            ["Header Files"] = { raylib_dir .. "/src/**.h"},
            ["Source Files/*"] = { raylib_dir .. "/src/**.c", raylib_dir .. "/src/**.cpp"},
        }
        files {raylib_dir .. "/src/*.h", raylib_dir .. "/src/*.c", raylib_dir .. "/src/*.cpp"}

        removefiles {raylib_dir .. "/src/rcore_*.c"}

        filter{}
