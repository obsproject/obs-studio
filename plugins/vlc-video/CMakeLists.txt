cmake_minimum_required(VERSION 3.22...3.25)

legacy_check()

macro(check_vlc_path)
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_Libvlc QUIET libvlc)
  endif()

  find_path(
    LibVLC_INCLUDE_DIR
    NAMES vlc/libvlc.h
    HINTS ${PC_Libvlc_INCLUDEDIR} ${VLC_PATH}/include
    PATHS /usr/include /usr/local/include
    DOC "LibVLC include directory")

  if(LibVLC_INCLUDE_DIR)
    target_include_directories(vlc-video PRIVATE "${LibVLC_INCLUDE_DIR}")
  else()
    message(FATAL_ERROR "Required system header <vlc/libvlc.h> not found.")
  endif()
endmacro()

option(ENABLE_VLC "Build OBS with VLC plugin support" ON)
if(NOT ENABLE_VLC)
  target_disable(vlc-video)
  return()
endif()

add_library(vlc-video MODULE)
add_library(OBS::vlc-video ALIAS vlc-video)

target_sources(vlc-video PRIVATE vlc-video-plugin.c vlc-video-plugin.h vlc-video-source.c)

target_compile_options(vlc-video PRIVATE $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang>:-Wno-shorten-64-to-32>)

target_link_libraries(vlc-video PRIVATE OBS::libobs $<$<PLATFORM_ID:Windows>:OBS::w32-pthreads>)

check_vlc_path()

if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in vlc-video.rc)
  target_sources(vlc-video PRIVATE vlc-video.rc)
endif()

# cmake-format: off
set_target_properties_obs(vlc-video PROPERTIES FOLDER plugins PREFIX "")
# cmake-format: on
