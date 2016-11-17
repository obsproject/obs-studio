project(obs-frontend-api)

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/libobs")

add_definitions(-DLIBOBS_EXPORTS)

set(obs-frontend-api_SOURCES
	obs-frontend-api.cpp)

set(obs-frontend-api_HEADERS
	obs-frontend-internal.hpp
	obs-frontend-api.h)

add_library(obs-frontend-api SHARED
	${obs-frontend-api_SOURCES}
	${obs-frontend-api_HEADERS})
target_link_libraries(obs-frontend-api
	libobs)

if(UNIX AND NOT APPLE)
	set_target_properties(obs-frontend-api
		PROPERTIES
			OUTPUT_NAME obs-frontend-api
			VERSION 0.0
			SOVERSION 0
			)
endif()

install_obs_core(obs-frontend-api)
