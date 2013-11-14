include_directories(${obs_SOURCE_DIR}/libobs)

add_library(test-input MODULE
	test-filter.c
	test-input.c
	test-random.c)

target_link_libraries(test-input
	libobs)

file(COPY
	${obs_SOURCE_DIR}/build/data/test-input/draw.effect
	${obs_SOURCE_DIR}/build/data/test-input/test.effect
	DESTINATION ${obs_BINARY_DIR}/data/test-input/)
