set(CG_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/CFG.cpp
)

add_library(cg OBJECT ${CG_SRC})

target_link_libraries(cg)

add_executable(cfg_tests ${CG_SRC} ${CMAKE_SOURCE_DIR}/src/tests/main.cpp)

target_link_libraries(cfg_tests tests)
set_target_properties(cfg_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
