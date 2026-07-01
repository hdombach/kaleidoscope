ui_sources = [
	'AppView.cpp',
	'CameraView.cpp',
	'Misc.cpp',
	'TextureView.cpp',
]

ui_deps = [
	glm,
	imgui,
	portable_file_dialogs,
	vulkan,
	glfw3,
]

ui = static_library(
	'ui',
	ui_sources,
	include_directories: [root_include],
	dependencies: ui_deps,
	link_with: [vulkan_wrap],
)
