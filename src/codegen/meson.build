codegen_sources = [
	'CfgNode.cpp',
	'CfgContext.cpp',
	'AstNode.cpp',
	'SParser.cpp',
	'TemplGen.cpp',
	'TemplObj.cpp',
	'AbsoluteSolver.cpp',
	'AbsoluteTable.cpp',
	'Tokenizer.cpp',
	'TemplTokenizer.cpp',
	'ParserContext.cpp',
	'Error.cpp',
]

cfg_test_sources = files([
	'../tests/main.cpp',
	'../tests/Test.cpp',
	'CfgNodeTest.cpp',
	'TemplGenTest.cpp',
	'TokenizerTest.cpp',
	'ParserTest.cpp',
])

codegen_deps = [
	glm,
	vulkan_headers
	# tests,
]

codegen_libs = [
	util
]


codegen = static_library(
	'codegen',
	codegen_sources,
	include_directories: [root_include],
	dependencies: codegen_deps,
	link_with: codegen_libs,
)

