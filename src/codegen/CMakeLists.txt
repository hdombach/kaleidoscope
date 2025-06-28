set(CG_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/CfgNode.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/CfgContext.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/AstNode.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/SParser.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/TemplGen.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/TemplObj.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/AbsoluteSolver.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/AbsoluteTable.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Tokenizer.cpp
)

set(CG_TEST
	${CMAKE_CURRENT_SOURCE_DIR}/CfgNodeTest.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/AstNodeTest.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/TemplGenTest.cpp
	#${CMAKE_CURRENT_SOURCE_DIR}/AbsoluteSolverTest.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/TokenizerTest.cpp
)

add_library(cg OBJECT ${CG_SRC})

target_link_libraries(cg PUBLIC glm::glm util tests)

add_executable(cfg_tests ${CMAKE_SOURCE_DIR}/src/tests/main.cpp ${CG_TEST})

target_link_libraries(cfg_tests tests cg util)
set_target_properties(cfg_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
