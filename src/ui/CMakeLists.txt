set(UI_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/AppView.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/CameraView.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Misc.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/TextureView.cpp
)

add_library(ui OBJECT ${UI_SRC})

target_link_libraries(ui PUBLIC glm::glm)
target_link_libraries(ui PUBLIC imgui::imgui)
target_link_libraries(ui PUBLIC portable-file-dialogs::portable-file-dialogs)
target_link_libraries(ui PUBLIC vulkan-headers::vulkan-headers)
target_link_libraries(ui PUBLIC glfw)

