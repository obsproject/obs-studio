cmake_minimum_required(VERSION 3.22...3.25)

legacy_check()

option(ENABLE_FREETYPE "Enable FreeType text plugin" ON)

if(NOT ENABLE_FREETYPE)
  target_disable(text-freetype2)
  return()
endif()

find_package(Freetype REQUIRED)

if(OS_MACOS)
  find_package(Iconv REQUIRED)
elseif(OS_LINUX OR OS_FREEBSD OR OS_OPENBSD)
  find_package(Fontconfig REQUIRED)
endif()

add_library(text-freetype2 MODULE)
add_library(OBS::text-freetype2 ALIAS text-freetype2)

target_sources(
  text-freetype2
  PRIVATE
    $<$<PLATFORM_ID:Darwin>:find-font-cocoa.m>
    $<$<PLATFORM_ID:Darwin>:find-font-iconv.c>
    $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:find-font-unix.c>
    $<$<PLATFORM_ID:Windows,Darwin>:find-font.c>
    $<$<PLATFORM_ID:Windows>:find-font-windows.c>
    find-font.h
    obs-convenience.c
    obs-convenience.h
    text-freetype2.c
    text-freetype2.h
    text-functionality.c
)

target_link_libraries(
  text-freetype2
  PRIVATE
    OBS::libobs
    Freetype::Freetype
    $<$<PLATFORM_ID:Darwin>:Iconv::Iconv>
    "$<$<PLATFORM_ID:Darwin>:$<LINK_LIBRARY:FRAMEWORK,Cocoa.framework>>"
    $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:Fontconfig::Fontconfig>
)

# FreeType is hard-coded with /DEFAULTLIB:MSVCRT
target_link_options(text-freetype2 PRIVATE $<$<PLATFORM_ID:Windows>:/IGNORE:4098>)

if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in text-freetype2.rc)
  target_sources(text-freetype2 PRIVATE text-freetype2.rc)
endif()

set_target_properties_obs(text-freetype2 PROPERTIES FOLDER plugins PREFIX "")
