cmake_minimum_required(VERSION 3.22...3.25)

option(ENABLE_PLUGINS "Enable building OBS plugins" ON)

if(OBS_CMAKE_VERSION VERSION_GREATER_EQUAL 3.0.0)
  if(NOT ENABLE_PLUGINS)
    set_property(GLOBAL APPEND PROPERTY OBS_FEATURES_DISABLED "Plugin Support")
    return()
  endif()

  set_property(GLOBAL APPEND PROPERTY OBS_FEATURES_ENABLED "Plugin Support")

  macro(check_obs_browser)
    if((OS_WINDOWS AND CMAKE_GENERATOR_PLATFORM MATCHES "(Win32|x64)") OR OS_MACOS OR OS_LINUX)
      if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/obs-browser/CMakeLists.txt")
        message(FATAL_ERROR "Required submodule 'obs-browser' not available.")
      else()
        add_subdirectory(obs-browser)
      endif()
    else()
      add_custom_target(obs-browser)
      target_disable(obs-browser)
    endif()
  endmacro()

  macro(check_obs_websocket)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/obs-websocket/CMakeLists.txt")
      message(FATAL_ERROR "Required submodule 'obs-websocket' not available.")
    else()
      add_subdirectory(obs-websocket)
    endif()
  endmacro()

  # Add plugins in alphabetical order to retain order in IDE projects
  add_obs_plugin(
    aja
    PLATFORMS WINDOWS MACOS LINUX
    WITH_MESSAGE
  )
  add_obs_plugin(coreaudio-encoder PLATFORMS WINDOWS MACOS)
  add_obs_plugin(
    decklink
    PLATFORMS WINDOWS MACOS LINUX
    WITH_MESSAGE
  )
  add_obs_plugin(image-source)
  add_obs_plugin(linux-alsa PLATFORMS LINUX FREEBSD OPENBSD)
  add_obs_plugin(linux-capture PLATFORMS LINUX FREEBSD OPENBSD)
  add_obs_plugin(linux-jack PLATFORMS LINUX FREEBSD OPENBSD)
  add_obs_plugin(linux-pipewire PLATFORMS LINUX FREEBSD OPENBSD)
  add_obs_plugin(linux-pulseaudio PLATFORMS LINUX FREEBSD OPENBSD)
  add_obs_plugin(linux-v4l2 PLATFORMS LINUX FREEBSD OPENBSD)
  add_obs_plugin(mac-avcapture PLATFORMS MACOS)
  add_obs_plugin(mac-capture PLATFORMS MACOS)
  add_obs_plugin(mac-syphon PLATFORMS MACOS)
  add_obs_plugin(mac-videotoolbox PLATFORMS MACOS)
  add_obs_plugin(mac-virtualcam PLATFORMS MACOS)
  add_obs_plugin(nv-filters PLATFORMS WINDOWS)

  check_obs_browser()

  add_obs_plugin(obs-ffmpeg)
  add_obs_plugin(obs-filters)
  add_obs_plugin(obs-libfdk)
  add_obs_plugin(obs-nvenc PLATFORMS WINDOWS LINUX)
  add_obs_plugin(obs-outputs)
  add_obs_plugin(
    obs-qsv11
    PLATFORMS WINDOWS LINUX
    ARCHITECTURES x64 x86_64
  )
  add_obs_plugin(obs-text PLATFORMS WINDOWS)
  add_obs_plugin(obs-transitions)
  add_obs_plugin(
    obs-vst
    PLATFORMS WINDOWS MACOS LINUX
    WITH_MESSAGE
  )
  add_obs_plugin(obs-webrtc)

  check_obs_websocket()

  add_obs_plugin(obs-x264)
  add_obs_plugin(oss-audio PLATFORMS FREEBSD OPENBSD)
  add_obs_plugin(rtmp-services)
  add_obs_plugin(sndio PLATFORMS LINUX FREEBSD OPENBSD)
  add_obs_plugin(text-freetype2)
  add_obs_plugin(vlc-video WITH_MESSAGE)
  add_obs_plugin(win-capture PLATFORMS WINDOWS)
  add_obs_plugin(win-dshow PLATFORMS WINDOWS)
  add_obs_plugin(win-wasapi PLATFORMS WINDOWS)

  return()
endif()

if(NOT ENABLE_PLUGINS)
  obs_status(STATUS "Building with plugins disabled.")
  return()
endif()

function(check_obs_browser)
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/obs-browser/CMakeLists.txt)
    add_subdirectory(obs-browser)
  else()
    obs_status(FATAL_ERROR "obs-browser submodule not available.")
  endif()
endfunction()

# APPLE/WIN32/UNIX are soft-deprecated: https://discourse.cmake.org/t/platform-id-vs-win32-vs-cmake-system-name/1226/2
if(OS_WINDOWS)
  add_subdirectory(coreaudio-encoder)
  add_subdirectory(win-wasapi)
  add_subdirectory(win-dshow)
  add_subdirectory(win-capture)
  add_subdirectory(decklink)
  add_subdirectory(obs-qsv11)
  add_subdirectory(obs-text)
  add_subdirectory(vlc-video)
  add_subdirectory(obs-vst)
  add_subdirectory(nv-filters)

  check_obs_browser()
elseif(OS_MACOS)
  add_subdirectory(coreaudio-encoder)
  add_subdirectory(mac-avcapture)
  add_subdirectory(mac-capture)
  add_subdirectory(mac-videotoolbox)
  add_subdirectory(mac-syphon)
  add_subdirectory(mac-virtualcam)
  add_subdirectory(decklink)
  add_subdirectory(vlc-video)
  add_subdirectory(linux-jack)
  add_subdirectory(obs-vst)

  check_obs_browser()
elseif(OS_LINUX)
  add_subdirectory(linux-capture)
  add_subdirectory(linux-pulseaudio)
  add_subdirectory(linux-v4l2)
  add_subdirectory(linux-jack)
  add_subdirectory(linux-alsa)
  add_subdirectory(linux-pipewire)
  add_subdirectory(decklink)
  add_subdirectory(vlc-video)
  add_subdirectory(sndio)
  add_subdirectory(obs-vst)
  add_subdirectory(obs-qsv11)

  check_obs_browser()
elseif(OS_FREEBSD)
  add_subdirectory(linux-capture)
  add_subdirectory(linux-pipewire)
  add_subdirectory(linux-pulseaudio)
  add_subdirectory(linux-v4l2)
  add_subdirectory(linux-jack)
  add_subdirectory(linux-alsa)
  add_subdirectory(vlc-video)
  add_subdirectory(oss-audio)
  add_subdirectory(sndio)

  obs_status(STATUS "obs-browser plugin not available.")
  obs_status(STATUS "obs-vst plugin not available.")
elseif(OS_OPENBSD)
  add_subdirectory(linux-capture)
  add_subdirectory(sndio)

  obs_status(STATUS "obs-browser plugin not available.")
  obs_status(STATUS "obs-vst plugin not available.")
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/obs-websocket/CMakeLists.txt)
  add_subdirectory(obs-websocket)
else()
  obs_status(FATAL_ERROR "obs-websocket submodule not available.")
endif()

add_subdirectory(image-source)
add_subdirectory(obs-x264)
add_subdirectory(obs-libfdk)
add_subdirectory(obs-ffmpeg)
add_subdirectory(obs-outputs)
add_subdirectory(obs-filters)
add_subdirectory(obs-transitions)
add_subdirectory(rtmp-services)
add_subdirectory(text-freetype2)
add_subdirectory(aja)
add_subdirectory(obs-webrtc)
