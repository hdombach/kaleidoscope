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

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h")

list(APPEND SOURCES "${CMAKE_BINARY_DIR}/imgui_impl_vulkan.cpp")
list(APPEND HEADERS "${CMAKE_BINARY_DIR}/imgui_impl_vulkan.h")

list(APPEND SOURCES "${CMAKE_BINARY_DIR}/imgui_impl_glfw.cpp")
list(APPEND HEADERS "${CMAKE_BINARY_DIR}/imgui_impl_glfw.h")

set(INCLUDE_DIRS "")
foreach(HEADER ${HEADERS})
	get_filename_component(TEMP_DIR ${HEADER} PATH)
	list(APPEND INCLUDE_DIRS ${TEMP_DIR})
endforeach()
list(REMOVE_DUPLICATES INCLUDE_DIRS)

include_directories(${HEADERS})

add_executable(kaleidoscope ${SOURCES})
target_include_directories(kaleidoscope PRIVATE ${INCLUDE_DIRS})
target_link_libraries(kaleidoscope PUBLIC ${Vulkan_LIBRARIES})
target_link_libraries(kaleidoscope PUBLIC glfw)
target_link_libraries(kaleidoscope PUBLIC stb::stb)
target_link_libraries(kaleidoscope PUBLIC tinyobjloader)
target_link_libraries(kaleidoscope PUBLIC glm::glm)
target_link_libraries(kaleidoscope PUBLIC imgui::imgui)
target_link_libraries(kaleidoscope PUBLIC vulkan-headers::vulkan-headers)
target_link_libraries(kaleidoscope PUBLIC shaderc::shaderc)
target_link_libraries(kaleidoscope PUBLIC portable-file-dialogs::portable-file-dialogs)
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	target_compile_options(kaleidoscope PUBLIC -g -O0)
endif()

file(MAKE_DIRECTORY "build")
file(CREATE_LINK "../assets" "${CMAKE_BINARY_DIR}/assets" SYMBOLIC)

message(STATUS "printing libraries")
message(STATUS glfw)

add_subdirectory(src/shaders)
