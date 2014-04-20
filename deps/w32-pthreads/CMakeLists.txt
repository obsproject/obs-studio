project(w32-pthreads)

add_definitions(-D__CLEANUP_C)
add_definitions(-DPTW32_BUILD)

set(w32-pthreads_SOURCES
	pthread.c)

set(w32-pthreads_HEADERS
	implement.h
	pthread.h
	sched.h
	semaphore.h)

add_library(w32-pthreads SHARED
	${w32-pthreads_SOURCES}
	${w32-pthreads_HEADERS})
target_link_libraries(w32-pthreads)

install_obs_core(w32-pthreads)
