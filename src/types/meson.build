type_sources = [
	'Camera.cpp',
	'ColorMaterial.cpp',
	'CombTextureMaterial.cpp',
	'MandelboxMesh.cpp',
	'MandelbulbMesh.cpp',
	'Node.cpp',
	'ResourceManager.cpp',
	'ShaderResource.cpp',
	'StaticMesh.cpp',
	'TextureMaterial.cpp'
]

types = static_library(
	'types',
	type_sources,
  include_directories: [src_include],
	dependencies: [
		glm,
		glfw3,
		tinyobjloader,
		imgui,
    vulkan,
	],
)
