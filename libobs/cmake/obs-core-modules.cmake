add_library(libobs-core-modules OBJECT)
add_library(OBS::libobs-core-modules ALIAS libobs-core-modules)

target_sources(libobs-core-modules PRIVATE obs-core-modules.c obs-core-modules.h)

target_include_directories(libobs-core-modules PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")

set_property(TARGET libobs-core-modules PROPERTY FOLDER core)
