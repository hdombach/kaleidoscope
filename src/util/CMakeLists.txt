set(UTIL_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/KError.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/file.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/log.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/math.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/PrintTools.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/StringRef.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Env.cpp
)

add_library(util OBJECT ${UTIL_SRC})

target_link_libraries(util PUBLIC vulkan-headers::vulkan-headers)
target_link_libraries(util PUBLIC tinyobjloader)
target_link_libraries(util PUBLIC glm::glm)
target_link_libraries(util PUBLIC glfw)
target_link_libraries(util PUBLIC imgui::imgui)

