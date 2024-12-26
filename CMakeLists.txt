project(kaleidoscope)
cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})
set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})

#Find libraries
#find_library(LIB_VULKAN vulkan)
find_package(Vulkan REQUIRED)
find_program(GLSLC glslc)
find_package(stb REQUIRED)
find_package(glfw3 REQUIRED)
find_package(tinyobjloader REQUIRED)
find_package(glm REQUIRED)
find_package(imgui REQUIRED)
find_package(VulkanHeaders REQUIRED)
find_package(shaderc REQUIRED)
find_package(portable-file-dialogs REQUIRED)

if( NOT LIB_VULKAN)
	#message(FATAL_ERROR "Could not find vulkan")
endif()

if( NOT GLSLC)
	message(FATAL_ERROR "Could not find glslc")
endif()


function(compile_shader)
	set(OPTIONS)
	set(ONE_VALUE_ARGS FILE_NAME OUTPUT_FILE_NAME)
	set(MULTI_VALUE_ARGS)
	cmake_parse_arguments(ARG "${OPTIONS}" "${ONE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN} )

	if (NOT EXISTS ${ARG_OUTPUT_FILE_NAME})
		set(ARG_OUTPUT_FILE_NAME ${ARG_FILE_NAME}.spv)
	endif()

	set(FULL_OUTPUT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/${ARG_OUTPUT_FILE_NAME})
	set(FULL_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/${ARG_FILE_NAME})

	add_custom_command(
		OUTPUT ${FULL_OUTPUT_FILE_NAME}	
		COMMAND ${GLSLC} ${FULL_FILE_NAME} -o ${FULL_OUTPUT_FILE_NAME}
		DEPENDS ${FULL_FILE_NAME})

	add_custom_target(
		shaders.${ARG_FILE_NAME} ALL
		DEPENDS ${FULL_OUTPUT_FILE_NAME})
endfunction()

file(GLOB_RECURSE HEADERS "src/*.h")

set(SOURCES
	src/App.cpp
	src/main.cpp
	"${CMAKE_BINARY_DIR}/imgui_impl_vulkan.cpp"
	"${CMAKE_BINARY_DIR}/imgui_impl_glfw.cpp"
)

set(INCLUDE_DIRS
	${CMAKE_BINARY_DIR}
	src
)

include_directories(${INCLUDE_DIRS})

add_subdirectory(src/types)
add_subdirectory(src/ui)
add_subdirectory(src/util)
add_subdirectory(src/vulkan)

add_executable(kaleidoscope ${SOURCES})
target_include_directories(kaleidoscope PRIVATE ${INCLUDE_DIRS})
target_link_libraries(kaleidoscope PUBLIC ${Vulkan_LIBRARIES})
target_link_libraries(kaleidoscope PUBLIC glfw)
target_link_libraries(kaleidoscope PUBLIC vulkan-headers::vulkan-headers)
target_link_libraries(kaleidoscope PUBLIC types ui util vulkan)

message("Build type is ${CMAKE_BUILD_TYPE}")
if ("${CMAKE_BUILD_TYPE}" STREQUAL "debug")
	target_compile_options(kaleidoscope PUBLIC -g -O0)
endif()

file(MAKE_DIRECTORY "build")
file(CREATE_LINK "../assets" "${CMAKE_BINARY_DIR}/assets" SYMBOLIC)

message(STATUS "printing libraries")
message(STATUS glfw)
