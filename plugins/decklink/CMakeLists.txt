cmake_minimum_required(VERSION 3.28...3.30)

option(ENABLE_DECKLINK "Build OBS with Decklink support" ON)
if(NOT ENABLE_DECKLINK)
  target_disable(decklink)
  return()
endif()

add_library(decklink MODULE)
add_library(OBS::decklink ALIAS decklink)

add_library(decklink-sdk INTERFACE)
add_library(Decklink::SDK ALIAS decklink-sdk)

target_sources(
  decklink-sdk
  INTERFACE
    "$<$<PLATFORM_ID:Darwin>:${CMAKE_CURRENT_SOURCE_DIR}/mac/decklink-sdk/DeckLinkAPI.h>"
    "$<$<PLATFORM_ID:Darwin>:${CMAKE_CURRENT_SOURCE_DIR}/mac/decklink-sdk/DeckLinkAPIConfiguration.h>"
    "$<$<PLATFORM_ID:Darwin>:${CMAKE_CURRENT_SOURCE_DIR}/mac/decklink-sdk/DeckLinkAPIDeckControl.h>"
    "$<$<PLATFORM_ID:Darwin>:${CMAKE_CURRENT_SOURCE_DIR}/mac/decklink-sdk/DeckLinkAPIDiscovery.h>"
    "$<$<PLATFORM_ID:Darwin>:${CMAKE_CURRENT_SOURCE_DIR}/mac/decklink-sdk/DeckLinkAPIDispatch.cpp>"
    "$<$<PLATFORM_ID:Darwin>:${CMAKE_CURRENT_SOURCE_DIR}/mac/decklink-sdk/DeckLinkAPIModes.h>"
    "$<$<PLATFORM_ID:Darwin>:${CMAKE_CURRENT_SOURCE_DIR}/mac/decklink-sdk/DeckLinkAPIStreaming.h>"
    "$<$<PLATFORM_ID:Darwin>:${CMAKE_CURRENT_SOURCE_DIR}/mac/decklink-sdk/DeckLinkAPITypes.h>"
    "$<$<PLATFORM_ID:Darwin>:${CMAKE_CURRENT_SOURCE_DIR}/mac/decklink-sdk/DeckLinkAPIVersion.h>"
    "$<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:${CMAKE_CURRENT_SOURCE_DIR}/linux/decklink-sdk/DeckLinkAPI.h>"
    "$<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:${CMAKE_CURRENT_SOURCE_DIR}/linux/decklink-sdk/DeckLinkAPIConfiguration.h>"
    "$<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:${CMAKE_CURRENT_SOURCE_DIR}/linux/decklink-sdk/DeckLinkAPIDeckControl.h>"
    "$<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:${CMAKE_CURRENT_SOURCE_DIR}/linux/decklink-sdk/DeckLinkAPIDiscovery.h>"
    "$<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:${CMAKE_CURRENT_SOURCE_DIR}/linux/decklink-sdk/DeckLinkAPIDispatch.cpp>"
    "$<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:${CMAKE_CURRENT_SOURCE_DIR}/linux/decklink-sdk/DeckLinkAPIModes.h>"
    "$<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:${CMAKE_CURRENT_SOURCE_DIR}/linux/decklink-sdk/DeckLinkAPITypes.h>"
    "$<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:${CMAKE_CURRENT_SOURCE_DIR}/linux/decklink-sdk/DeckLinkAPIVersion.h>"
    "$<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:${CMAKE_CURRENT_SOURCE_DIR}/linux/decklink-sdk/LinuxCOM.h>"
    "$<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/win/decklink-sdk/DeckLinkAPIVersion.h>"
)

target_sources(
  decklink
  PRIVATE
    $<$<PLATFORM_ID:Darwin>:mac/platform.cpp>
    $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:linux/platform.cpp>
    $<$<PLATFORM_ID:Windows>:win/platform.cpp>
    audio-repack.c
    audio-repack.h
    audio-repack.hpp
    const.h
    decklink-device-discovery.cpp
    decklink-device-discovery.hpp
    decklink-device-instance.cpp
    decklink-device-instance.hpp
    decklink-device-mode.cpp
    decklink-device-mode.hpp
    decklink-device.cpp
    decklink-device.hpp
    decklink-devices.cpp
    decklink-devices.hpp
    decklink-output.cpp
    decklink-source.cpp
    DecklinkBase.cpp
    DecklinkBase.h
    DecklinkInput.cpp
    DecklinkInput.hpp
    DecklinkOutput.cpp
    DecklinkOutput.hpp
    OBSVideoFrame.cpp
    OBSVideoFrame.h
    platform.hpp
    plugin-main.cpp
    util.cpp
    util.hpp
)

target_link_libraries(
  decklink
  PRIVATE
    OBS::libobs
    OBS::caption
    Decklink::SDK
    "$<$<PLATFORM_ID:Darwin>:$<LINK_LIBRARY:FRAMEWORK,CoreFoundation.framework>>"
)

if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in win-decklink.rc)
  target_sources(decklink PRIVATE win-decklink.rc)

  include(idlfilehelper)
  target_add_idl_files(decklink win/decklink-sdk/DeckLinkAPI.idl)
  set_source_files_properties(win/decklink-sdk/DeckLinkAPIVersion.h PROPERTIES GENERATED TRUE)
  target_sources(decklink-sdk INTERFACE win/decklink-sdk/DeckLinkAPIVersion.h)
endif()

set_target_properties_obs(decklink PROPERTIES FOLDER plugins/decklink PREFIX "")
