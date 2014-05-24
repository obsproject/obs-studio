
add_subdirectory(w32-pthreads)
add_subdirectory(glad)

find_package(Jansson 2.5 QUIET)

if(NOT JANSSON_FOUND)
	message(STATUS "Jansson >=2.5 not found, building bundled version")
	add_subdirectory(jansson)
else()
	message(STATUS "Using system Jansson library")
	add_library(jansson UNKNOWN IMPORTED)
	set_property(TARGET jansson PROPERTY
		IMPORTED_LOCATION "${JANSSON_LIBRARIES}"
		INTERFACE_INCLUDE_DIRECTORIES "${JANSSON_INCLUDE_DIRS}")
endif()

