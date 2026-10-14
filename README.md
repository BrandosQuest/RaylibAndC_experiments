# RaylibAndC_experiments

A raylib + C project for experimenting with interactive audio synthesis controlled by hand tracking.

## Prerequisites (Ubuntu)

```bash
sudo apt install build-essential libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl-dev
```

For **Wayland** support (optional):
```bash
sudo apt install wayland-protocols libwayland-dev libxkbcommon-dev
```

## Build

```bash
# Generate makefiles (first time, or after changing premake5.lua)
cd build && ./premake5 gmake && cd ..

# Build (debug by default)
make

# Build release
make config=release_x64
```

## Run

```bash
./bin/Debug/RaylibAndC_experiments      # debug build
./bin/Release/RaylibAndC_experiments    # release build
```

## Project Structure

```
src/               Source code (.c, .h)
resources/         Game assets (textures, sounds, etc.)
build/             Premake5 binary and build scripts
  external/        Raylib source (auto-downloaded by premake)
  ecc/             Export Compile Commands module for clangd
bin/               Compiled binaries (gitignored)
obj/               Object files (gitignored)
```

## VSCode

Open the folder in VSCode and press **F5** to build and debug. Tasks and launch configs are pre-configured for Linux/GDB.

## License

Based on [raylib-quickstart](https://github.com/raylib-extras/raylib-quickstart) (CC0 1.0).
