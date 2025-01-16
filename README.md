# kaleidoscope

# Compiling For Release
```
conan install . --output-folder=build --build=missing
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

# Compiling For Debug 
```
conan install . --output-folder=build --build=missing --profile=debug
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

# Compiling
Or you can just use `setup.sh`
If you want to specify which compiler to use, you can define
- `KALEIDOSCOPE_CC`
- `KALEIDOSCOPE_CXX`


# Overview
This will be the successor of Fractal Renderer (https://github.com/hdombach/Fractal-Renderer/tree/main).
The goal is to include more tools like the ability to load textures and meshes instead of just using SDF's and procedural textures.

Currently, I'm still building up framework based on the vulkan tutorial.
I just got a viewport working correctly in ImGui.
<img width="1417" alt="kaleidescope2" src="https://github.com/user-attachments/assets/6027e532-f33e-4339-bfd3-ef5a79e2989c">


# Misc sources
[grunge texture](https://www.deviantart.com/fictionchick/art/Grunge-Texture-Overlay-PNG-428805936)
