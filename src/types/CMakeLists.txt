set(TYPES_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/Node.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Camera.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ColorMaterial.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/CombTextureMaterial.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/MandelboxMesh.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/MandelbulbMesh.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ResourceManager.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ShaderResource.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/StaticMesh.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/TextureMaterial.cpp
)

add_library(types OBJECT ${TYPES_SRC})

target_link_libraries(types PUBLIC vulkan-headers::vulkan-headers)
target_link_libraries(types PUBLIC glm::glm)
target_link_libraries(types PUBLIC glfw)
target_link_libraries(types PUBLIC tinyobjloader)

