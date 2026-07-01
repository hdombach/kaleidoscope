util_sources = [
	'file.cpp',
	'log.cpp',
	'math.cpp',
	'PrintTools.cpp',
	'StringRef.cpp',
	'Env.cpp',
	'ThreadPool.cpp'
]

util_deps = [
	vulkan,
	tinyobjloader,
	glm,
	glfw3,
	imgui,
  vulkan,
  vulkan_headers,
]

util = static_library(
	'util',
	util_sources,
  include_directories: [root_include],
	dependencies: util_deps,
)
