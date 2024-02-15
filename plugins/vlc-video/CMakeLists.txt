cmake_minimum_required(VERSION 3.22...3.25)

legacy_check()

macro(check_vlc_path)
  if(OS_WINDOWS OR OS_MACOS)
    if(NOT EXISTS "${VLC_PATH}/include/vlc/libvlc.h")
      message(FATAL_ERROR "Unable to find libvlc.h. Check if VLC_PATH set correctly.")
    endif()

    target_include_directories(vlc-video PRIVATE "${VLC_PATH}/include/vlc")
  elseif(
    OS_LINUX
    OR OS_FREEBSD
    OR OS_OPENBSD)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
      pkg_check_modules(PC_Libvlc QUIET libvlc)
    endif()

    if(PC_Libvlc_INCLUDEDIR)
      target_include_directories(vlc-video PRIVATE "${PC_Libvlc_INCLUDEDIR}/vlc")
    endif()
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
