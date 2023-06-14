#! /bin/bash

if [ $# -lt 1 ]
then
	echo "Need to provide built type"
	echo "$0 <build type>"
	exit 1
fi

mkdir build
conan install . --output-folder=build --build=missing -s build_type=Debug
conan install . --output-folder=build --build=missing -s build_type=Release
cmake -B build -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=$1 -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -G Ninja
