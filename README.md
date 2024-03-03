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




# Overview
This will be the successor of Fractal Renderer (https://github.com/hdombach/Fractal-Renderer/tree/main).
The goal is to include more tools like the ability to load textures and meshes instead of just using SDF's and procedural textures.

Currently, I'm still building up framework based on the vulkan tutorial.
I just got a viewport working correctly in ImGui.
<img width="912" alt="Screenshot 2023-06-15 at 2 18 58 PM" src="https://github.com/hdombach/kaleidoscope/assets/56201502/0176a856-c49e-436a-a7b8-be4daa070619">
