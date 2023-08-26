cmake_minimum_required(VERSION 3.22...3.25)
if(OS_WINDOWS)
  add_library(nv-filters MODULE)
  add_library(OBS::nv-filters ALIAS nv-filters)

  option(ENABLE_NVAFX "Enable building with NVIDIA Audio Effects SDK (requires redistributable to be installed)" ON)
  option(ENABLE_NVVFX "Enable building with NVIDIA Video Effects SDK (requires redistributable to be installed)" ON)

  if(ENABLE_NVAFX)
    target_enable_feature(nv-filters "NVIDIA Audio FX support" LIBNVAFX_ENABLED HAS_NOISEREDUCTION)
    target_sources(nv-filters PRIVATE nvidia-audiofx-filter.c)
  else()
    target_disable_feature(nv-filters "NVIDIA Audio FX support")
  endif()

  if(ENABLE_NVVFX)
    target_enable_feature(nv-filters "NVIDIA Video FX support" LIBNVVFX_ENABLED)
    target_sources(nv-filters PRIVATE nvidia-videofx-filter.c)
  else()
    target_disable_feature(nv-filters "NVIDIA Video FX support")
  endif()

  configure_file(cmake/windows/obs-module.rc.in nv-filters.rc)
  target_sources(nv-filters PRIVATE nv-filters.rc)
  target_sources(nv-filters PRIVATE nv-filters.c)

  target_link_libraries(nv-filters PRIVATE OBS::libobs $<$<PLATFORM_ID:Windows>:OBS::w32-pthreads>)

  set_target_properties_obs(nv-filters PROPERTIES FOLDER plugins PREFIX "")
endif()
