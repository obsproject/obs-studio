cmake_minimum_required(VERSION 3.22...3.25)

legacy_check()

add_library(obs-filters MODULE)
add_library(OBS::filters ALIAS obs-filters)

target_sources(
  obs-filters
  PRIVATE obs-filters.c
          color-correction-filter.c
          async-delay-filter.c
          gpu-delay.c
          hdr-tonemap-filter.c
          crop-filter.c
          scale-filter.c
          scroll-filter.c
          chroma-key-filter.c
          color-key-filter.c
          color-grade-filter.c
          sharpness-filter.c
          eq-filter.c
          gain-filter.c
          noise-gate-filter.c
          mask-filter.c
          invert-audio-polarity.c
          compressor-filter.c
          limiter-filter.c
          expander-filter.c
          luma-key-filter.c)

target_link_libraries(obs-filters PRIVATE OBS::libobs)

include(cmake/speexdsp.cmake)
include(cmake/rnnoise.cmake)
include(cmake/nvidia.cmake)

if(OS_WINDOWS)
  target_link_libraries(obs-filters PRIVATE OBS::w32-pthreads)

  configure_file(cmake/windows/obs-module.rc.in obs-filters.rc)
  target_sources(obs-filters PRIVATE obs-filters.rc)
endif()

set_target_properties_obs(obs-filters PROPERTIES FOLDER plugins PREFIX "")
