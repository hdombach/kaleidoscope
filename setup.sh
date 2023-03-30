mkdir build
conan install . --output-folder=build --build=missing
cmake --preset conan-release -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -GNinja -DCMAKE_BUILD_TYPE=Debug
