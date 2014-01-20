project(w32-pthreads)

add_definitions(-D__CLEANUP_C)
add_definitions(-DPTW32_BUILD)
add_definitions(-DPTW32_STATIC_LIB)

set(w32-pthreads_SOURCES
	pthread.c)

set(w32-pthreads_HEADERS
	implement.h
	pthread.h
	sched.h
	semaphore.h)

add_library(w32-pthreads STATIC
	${w32-pthreads_SOURCES}
	${w32-pthreads_HEADERS})
target_link_libraries(w32-pthreads)
