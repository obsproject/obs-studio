project(linux-jack)

option(ENABLE_JACK "Build OBS with JACK support" OFF)
if(NOT ENABLE_JACK)
  obs_status(DISABLED "linux-jack")
  return()
endif()

find_package(Jack REQUIRED)

add_library(linux-jack MODULE)
add_library(OBS::jack ALIAS linux-jack)

target_sources(linux-jack PRIVATE linux-jack.c jack-wrapper.c jack-input.c)

target_link_libraries(linux-jack PRIVATE OBS::libobs Jack::Jack)

set_target_properties(linux-jack PROPERTIES FOLDER "plugins" PROJECT_LABEL "JACK Audio")

setup_plugin_target(linux-jack)
