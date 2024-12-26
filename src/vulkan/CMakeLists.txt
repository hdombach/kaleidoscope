set(VULKAN_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/DescriptorPool.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/DescriptorSet.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Fence.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Image.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Sampler.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Scene.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/SceneTexture.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Semaphore.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Shader.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/StaticBuffer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/StaticTexture.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/UIRenderPipeline.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/graphics.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/prev_pass/PrevPass.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/prev_pass/PrevPassCodegen.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/prev_pass/PrevPassMaterial.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/prev_pass/PrevPassMesh.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/prev_pass/PrevPassNode.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/ray_pass/RayPass.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ray_pass/RayPassMaterial.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ray_pass/RayPassMesh.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ray_pass/RayPassNode.cpp
)

add_library(vulkan OBJECT ${VULKAN_SRC})

target_link_libraries(vulkan PUBLIC vulkan-headers::vulkan-headers)
target_link_libraries(vulkan PUBLIC stb::stb)
target_link_libraries(vulkan PUBLIC glm::glm)
target_link_libraries(vulkan PUBLIC glfw)
target_link_libraries(vulkan PUBLIC imgui::imgui)
target_link_libraries(vulkan PUBLIC shaderc::shaderc)
target_link_libraries(vulkan PUBLIC tinyobjloader)

