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
	PUBLIC
		libobs)

if(UNIX AND NOT APPLE)
	set(DEST_DIR "${CMAKE_INSTALL_PREFIX}")
	foreach(LIB "libobs")
		set(PRIVATE_LIBS "${PRIVATE_LIBS} -l${LIB}")
	endforeach()
	CONFIGURE_FILE("obs-frontend-api.pc.in" "obs-frontend-api.pc" @ONLY)
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/obs-frontend-api.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
endif()

target_include_directories(obs-frontend-api
	PUBLIC
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
		"$<INSTALL_INTERFACE:${OBS_INCLUDE_DESTINATION}>")

if(UNIX AND NOT APPLE)
	set_target_properties(obs-frontend-api
		PROPERTIES
			OUTPUT_NAME obs-frontend-api
			VERSION 0.0
			SOVERSION 0
			)
endif()

install_obs_core(obs-frontend-api EXPORT obs-frontend-api)
install_obs_headers(obs-frontend-api.h)
