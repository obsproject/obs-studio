cmake_minimum_required(VERSION 3.16...3.25)

legacy_check()

option(ENABLE_FREETYPE "Enable FreeType text plugin" ON)

if(NOT ENABLE_FREETYPE)
  target_disable(text-freetype2)
  return()
endif()

find_package(Freetype REQUIRED)

add_library(text-freetype2 MODULE)
add_library(OBS::text-freetype2 ALIAS text-freetype2)

target_sources(text-freetype2 PRIVATE find-font.h obs-convenience.c text-functionality.c text-freetype2.c
                                      obs-convenience.h text-freetype2.h)
target_link_libraries(text-freetype2 PRIVATE OBS::libobs Freetype::Freetype)

if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in text-freetype2.rc)
  target_sources(text-freetype2 PRIVATE find-font.c find-font-windows.c text-freetype2.rc)
  target_link_options(text-freetype2 PRIVATE /IGNORE:4098 /IGNORE:4099)
elseif(OS_MACOS)
  find_package(Iconv REQUIRED)

  target_sources(text-freetype2 PRIVATE find-font.c find-font-cocoa.m find-font-iconv.c)
  target_link_libraries(text-freetype2 PRIVATE Iconv::Iconv "$<LINK_LIBRARY:FRAMEWORK,Cocoa.framework>")
elseif(OS_LINUX OR OS_FREEBSD)
  find_package(Fontconfig REQUIRED)

  target_sources(text-freetype2 PRIVATE find-font-unix.c)
  target_link_libraries(text-freetype2 PRIVATE Fontconfig::Fontconfig)
endif()

set_target_properties_obs(text-freetype2 PROPERTIES FOLDER plugins PREFIX "")
