project(kaleidoscope)
cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 17)

find_library(LIB_GLFW glfw)

if( NOT LIB_GLFW)
	message(FATAL_ERROR "Could not find glm")
endif()

find_library(LIB_VULKAN vulkan)

if( NOT LIB_VULKAN)
	message(FATAL_ERROR "Could not find vulkan")
endif()

add_executable(kaleidoscope src/main.cpp)
target_link_libraries(kaleidoscope ${LIB_GLFW} ${LIB_VULKAN})
