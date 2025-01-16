#! /bin/bash

BUILD_TYPE=Debug

if [ $# -gt 1 ]; then
	echo "Need to provide built type"
	echo "$0 <build type>"
	exit 1
elif [ $# -eq 1 ]; then
BUILD_TYPE="$1"
fi

mkdir build
conan install . --output-folder=build --build=missing -s build_type=Debug
conan install . --output-folder=build --build=missing -s build_type=Release

echo "Build type is $BUILD_TYPE"

CMAKE_FLAGS="-DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE"
CMAKE_FLAGS="$CMAKE_FLAGS -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -G Ninja"

if [ -n "$KALEIDOSCOPE_CC" ]; then
	CMAKE_FLAGS="$CMAKE_FLAGS -DCMAKE_C_COMPILER=$KALEIDOSCOPE_CC"
fi

if [ -n "$KALEIDOSCOPE_CXX" ]; then
	CMAKE_FLAGS="$CMAKE_FLAGS -DCMAKE_CXX_COMPILER=$KALEIDOSCOPE_CXX"
fi


cmake -B build $CMAKE_FLAGS
