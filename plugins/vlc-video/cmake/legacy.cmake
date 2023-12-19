project(vlc-video)

option(ENABLE_VLC "Build OBS with VLC plugin support" ${OS_LINUX})
if(NOT ENABLE_VLC)
  obs_status(DISABLED "vlc-video")
  obs_status(
    WARNING
    "VLC plugin supported is not enabled by default - please switch ENABLE_VLC to ON to enable this functionality.")
  return()
endif()

find_package(LibVLC REQUIRED)

add_library(vlc-video MODULE)
add_library(OBS::vlc-video ALIAS vlc-video)

target_sources(vlc-video PRIVATE vlc-video-plugin.c vlc-video-plugin.h vlc-video-source.c)

target_link_libraries(vlc-video PRIVATE OBS::libobs VLC::LibVLC)

if(OS_WINDOWS)
  if(MSVC)
    target_link_libraries(vlc-video PRIVATE OBS::w32-pthreads)
  endif()

  set(MODULE_DESCRIPTION "OBS VLC Plugin")
  configure_file(${CMAKE_SOURCE_DIR}/cmake/bundle/windows/obs-module.rc.in vlc-video.rc)

  target_sources(vlc-video PRIVATE vlc-video.rc)

endif()

set_target_properties(vlc-video PROPERTIES FOLDER "plugins" PREFIX "")

setup_plugin_target(vlc-video)
