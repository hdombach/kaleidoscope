
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

doc_engine = executable(
	'doc_engine',
	serial_sources,
	include_directories: [src_include],
	dependencies: serial_deps,
	link_with: serial_libs,
)

example_serial = custom_target(
	'example_serial',
	input: 'example.dt',
	output: ['example.cpp', 'example.hpp'],
	command: [doc_engine, '@INPUT@', '-d', 'gen', '-o', '@OUTDIR@'],
	env: {'KALEIDOSCOPE_PATH': meson.project_source_root()},
	build_by_default: true,
)
