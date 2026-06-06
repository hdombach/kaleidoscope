set(DOC_ENGINE_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/Args.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/DocEngine.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Error.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Tokenizer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Validate.cpp
)

add_executable(doc_engine ${DOC_ENGINE_SRC})

set(INCLUDE_DIRS
	${CMAKE_BINARY_DIR}
	src
)

target_include_directories(doc_engine PRIVATE ${INCLUDE_DIRS})
target_link_libraries(doc_engine PUBLIC types util cg tests vulkan ui)
target_link_libraries(doc_engine PUBLIC vulkan-headers::vulkan-headers)
target_link_libraries(doc_engine PUBLIC ${Vulkan_LIBRARIES})

set_target_properties(doc_engine PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
