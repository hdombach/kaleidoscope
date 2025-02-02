set(TEST_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/Test.cpp
)

add_library(tests OBJECT ${TEST_SRC})

target_link_libraries(tests PUBLIC util)
