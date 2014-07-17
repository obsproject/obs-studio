
add_subdirectory(w32-pthreads)
add_subdirectory(glad)

find_package(Jansson 2.5 QUIET)

if(NOT JANSSON_FOUND)
	message(STATUS "Jansson >=2.5 not found, building bundled version")

	add_subdirectory(jansson)

	set(OBS_JANSSON_IMPORT "jansson" CACHE INTERNAL "Internal var")
	set(OBS_JANSSON_INCLUDE_DIRS "" CACHE INTERNAL "Internal var")
else()
	message(STATUS "Using system Jansson library")

	set(OBS_JANSSON_IMPORT "${JANSSON_LIBRARIES}" CACHE INTERNAL "Internal var")
	set(OBS_JANSSON_INCLUDE_DIRS "${JANSSON_INCLUDE_DIRS}" CACHE INTERNAL "Internal var")
endif()
