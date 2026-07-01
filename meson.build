project(
	'kaleidoscope',
	'cpp',
	version: '0.0.0',
	default_options: ['cpp_std=c++20']
)


root_include = include_directories('src')

glm = dependency('glm')
glfw3 = dependency('glfw3')
tinyobjloader = dependency('tinyobjloader')
imgui = dependency('imgui')
vulkan = dependency('vulkan')
vulkan_headers = dependency('vulkan-headers')
shaderc = dependency('shaderc')
portable_file_dialogs = dependency('portable-file-dialogs')
stb = dependency('stb')

subdir('src/util')
subdir('src/types')
subdir('src/codegen')
subdir('src/vulkan')
subdir('src/ui')
subdir('src/serial')

source_files = [
	'src/App.cpp',
	'src/main.cpp',
]

dependencies = [
	# dependency('types'),
	vulkan,
	vulkan_headers,
	glfw3,
	imgui,
]

libs = [
	util,
	vulkan_wrap,
	types,
	ui,
	codegen,
]

project_target = executable(
	meson.project_name(),
	source_files,
	include_directories: [root_include],
	dependencies: dependencies,
	link_with: libs
)

executable(
	'cfg_tests',
  cfg_test_sources,
  include_directories: [root_include],
	dependencies: codegen_deps,
	link_with: codegen,
)

executable(
	'doc_engine',
	link_with: doc_engine,
)
