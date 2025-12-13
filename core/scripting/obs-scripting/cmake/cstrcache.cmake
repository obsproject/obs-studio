add_library(obs-cstrcache INTERFACE)
add_library(OBS::cstrcache ALIAS obs-cstrcache)

target_sources(obs-cstrcache INTERFACE cstrcache.cpp cstrcache.h)
target_include_directories(obs-cstrcache INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
