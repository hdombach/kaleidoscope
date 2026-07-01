#! /bin/bash

BUILD_TYPE=Debug

if [ $# -gt 1 ]; then
	echo "Need to provide built type"
	echo "$0 <build type>"
	exit 1
elif [ $# -eq 1 ]; then
BUILD_TYPE="$1"
fi

echo "Installing dependencies"
mkdir build
conan install . --output-folder=build --build=missing -s build_type=Debug
conan install . --output-folder=build --build=missing -s build_type=Release

if [ -n "$KALEIDOSCOPE_CXX" ]; then
	CXX="$KALEIDOSCOPE_CXX"
fi

echo "Setting up build system"

if [ "$BUILD_TYPE" = "Debug" ]; then
	meson setup --native-file build/conan_meson_native.ini --buildtype debug build
else
	meson setup --native-file build/conan_meson_native.ini build
fi

echo "Run \"meson compile -C build\" to compile"
