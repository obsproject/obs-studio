include_directories(SYSTEM ${obs_SOURCE_DIR}/libobs)

add_library(test-input MODULE
	test-desktop.m
	test-filter.c
	test-input.c
	test-sinewave.c
	test-random.c)

set_source_files_properties(test-desktop.m
	PROPERTIES LANGUAGE C
		COMPILE_FLAGS "-fobjc-arc")

find_library(IOSURF IOSurface)


target_link_libraries(test-input
	libobs
	${IOSURF})

obs_add_plugin(test-input)

obs_add_data_source(/data/obs-plugins/test-input/
	${obs_SOURCE_DIR}/build/data/obs-plugins/test-input/draw_rect.effect
	${obs_SOURCE_DIR}/build/data/obs-plugins/test-input/draw.effect
	${obs_SOURCE_DIR}/build/data/obs-plugins/test-input/test.effect
	)
