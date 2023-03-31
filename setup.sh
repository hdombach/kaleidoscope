mkdir build
conan install . --output-folder=build --build=missing -s build_type=Debug
cmake -B build -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -G Ninja
