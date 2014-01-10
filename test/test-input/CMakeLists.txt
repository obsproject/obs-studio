include_directories(SYSTEM ${obs_SOURCE_DIR}/libobs)

add_library(test-input MODULE
	test-filter.c
	test-input.c
	test-random.c)

target_link_libraries(test-input
	libobs)

obs_add_plugin(test-input)

obs_add_data_source(/data/obs-plugins/test-input/
	${obs_SOURCE_DIR}/build/data/obs-plugins/test-input/draw.effect
	${obs_SOURCE_DIR}/build/data/obs-plugins/test-input/test.effect)
