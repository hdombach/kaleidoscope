
serial_sources = [
	'Args.cpp',
	'DocEngine.cpp',
	'Error.cpp',
	'Tokenizer.cpp',
	'main.cpp',
	'Validate.cpp',
	]

serial_deps = [
	vulkan,
	# tests,
]

serial_libs = [
	util,
	codegen,
	vulkan_wrap,
	ui,
]

doc_engine = static_library(
	'doc_engine',
	serial_sources,
	include_directories: [root_include],
	dependencies: serial_deps,
	link_with: serial_libs,
)
