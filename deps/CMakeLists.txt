
if(WIN32)
	add_subdirectory(w32-pthreads)
endif()

add_subdirectory(glew)

set(BUILD_DOCS FALSE CACHE INTERNAL "" FORCE)
set(STATIC_CRT TRUE CACHE INTERNAL "" FORCE)
set(BUILD_SHARED_LIBS FALSE CACHE INTERNAL "" FORCE)
set(WITHOUT_TESTS TRUE CACHE INTERNAL "" FORCE)
add_subdirectory(jansson)

