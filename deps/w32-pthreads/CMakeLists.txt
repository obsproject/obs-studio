cmake_minimum_required(VERSION 3.24...3.25)

legacy_check()

add_library(w32-pthreads SHARED EXCLUDE_FROM_ALL )
add_library(OBS::w32-pthreads ALIAS w32-pthreads)

target_sources(w32-pthreads PRIVATE # cmake-format: sortable
                                    implement.h pthread.c pthread.h sched.h semaphore.h w32-pthreads.rc)

target_compile_definitions(w32-pthreads PRIVATE __CLEANUP_C PTW32_BUILD)

target_include_directories(w32-pthreads PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")

configure_file(cmake/windows/obs-module.rc.in w32-pthreads.rc)

set_target_properties_obs(w32-pthreads PROPERTIES FOLDER deps)

set_property(
  TARGET w32-pthreads
  APPEND
  PROPERTY PUBLIC_HEADER pthread.h sched.h)

target_export(w32-pthreads)
