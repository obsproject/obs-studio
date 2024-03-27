add_library(libobs-version OBJECT)
add_library(OBS::libobs-version ALIAS libobs-version)

configure_file(obsversion.c.in obsversion.c @ONLY)

target_sources(libobs-version PRIVATE obsversion.c PUBLIC obsversion.h)

target_include_directories(libobs-version PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")

set_property(TARGET libobs-version PROPERTY FOLDER core)
