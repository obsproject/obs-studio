cmake_minimum_required(VERSION 3.22...3.25)

add_library(bpm OBJECT)
add_library(OBS::bpm ALIAS bpm)

target_sources(bpm PRIVATE bpm.c bpm-internal.h PUBLIC bpm.h)

target_include_directories(bpm PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(bpm PUBLIC OBS::libobs OBS::caption)

set_target_properties(bpm PROPERTIES FOLDER deps POSITION_INDEPENDENT_CODE TRUE)
