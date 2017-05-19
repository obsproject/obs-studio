project(obs-filters)

find_package(Libspeexdsp QUIET)
if(LIBSPEEXDSP_FOUND)
	set(obs-filters_LIBSPEEXDSP_SOURCES
		noise-suppress-filter.c)
	set(obs-filters_LIBSPEEXDSP_LIBRARIES
		${LIBSPEEXDSP_LIBRARIES})
else()
	message(STATUS "Speexdsp library not found, speexdsp filters disabled")
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/obs-filters-config.h.in"
	"${CMAKE_BINARY_DIR}/plugins/obs-filters/config/obs-filters-config.h")

set(obs-filters_config_HEADERS
	"${CMAKE_BINARY_DIR}/plugins/obs-filters/config/obs-filters-config.h")
include_directories(${LIBSPEEXDSP_INCLUDE_DIRS}
	"${CMAKE_BINARY_DIR}/plugins/obs-filters/config")

set(obs-filters_SOURCES
	obs-filters.c
	color-correction-filter.c
	async-delay-filter.c
	gpu-delay.c
	crop-filter.c
	scale-filter.c
	scroll-filter.c
	chroma-key-filter.c
	color-key-filter.c
	color-grade-filter.c
	sharpness-filter.c
	gain-filter.c
	noise-gate-filter.c
	mask-filter.c
	compressor-filter.c)

add_library(obs-filters MODULE
	${obs-filters_SOURCES}
	${obs-filters_config_HEADERS}
	${obs-filters_LIBSPEEXDSP_SOURCES})
target_link_libraries(obs-filters
	libobs
	${obs-filters_LIBSPEEXDSP_LIBRARIES})

install_obs_plugin_with_data(obs-filters data)
