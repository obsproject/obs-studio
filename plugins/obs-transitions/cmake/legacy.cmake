project(obs-transitions)

add_library(obs-transitions MODULE)
add_library(OBS::transition ALIAS obs-transitions)

target_sources(
  obs-transitions
  PRIVATE obs-transitions.c
          transition-slide.c
          transition-swipe.c
          transition-fade.c
          transition-cut.c
          transition-fade-to-color.c
          transition-luma-wipe.c
          transition-stinger.c)

if(NOT OS_MACOS)
  target_sources(
    obs-transitions
    PRIVATE data/fade_to_color_transition.effect data/fade_transition.effect data/luma_wipe_transition.effect
            data/slide_transition.effect data/stinger_matte_transition.effect data/swipe_transition.effect)

  get_target_property(_SOURCES obs-transitions SOURCES)
  set(_FILTERS ${_SOURCES})
  list(FILTER _FILTERS INCLUDE REGEX ".*\\.effect")

  source_group(
    TREE "${CMAKE_CURRENT_SOURCE_DIR}"
    PREFIX "Effect Files"
    FILES ${_FILTERS})
endif()

target_link_libraries(obs-transitions PRIVATE OBS::libobs)

if(OS_WINDOWS)
  set(MODULE_DESCRIPTION "OBS Transitions module")
  configure_file(${CMAKE_SOURCE_DIR}/cmake/bundle/windows/obs-module.rc.in obs-transitions.rc)

  target_sources(obs-transitions PRIVATE obs-transitions.rc)

endif()

set_target_properties(obs-transitions PROPERTIES FOLDER "plugins" PREFIX "")

setup_plugin_target(obs-transitions)
