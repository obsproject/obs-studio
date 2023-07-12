project(win-capture)

option(ENABLE_COMPAT_UPDATES "Checks for capture compatibility data updates" ON)

set(COMPAT_URL
    "https://obsproject.com/obs2_update/win-capture"
    CACHE STRING "Default services package URL")

mark_as_advanced(COMPAT_URL)

add_library(win-capture MODULE)
add_library(OBS::capture ALIAS win-capture)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/compat-config.h.in ${CMAKE_BINARY_DIR}/config/compat-config.h)

target_sources(
  win-capture
  PRIVATE plugin-main.c
          app-helpers.c
          app-helpers.h
          cursor-capture.c
          cursor-capture.h
          dc-capture.c
          dc-capture.h
          duplicator-monitor-capture.c
          game-capture.c
          game-capture-file-init.c
          graphics-hook-info.h
          graphics-hook-ver.h
          hook-helpers.h
          inject-library.c
          inject-library.h
          load-graphics-offsets.c
          monitor-capture.c
          nt-stuff.c
          nt-stuff.h
          window-capture.c
          compat-helpers.c
          compat-helpers.h
          compat-format-ver.h
          ../../libobs/util/windows/obfuscate.c
          ../../libobs/util/windows/obfuscate.h
          ${CMAKE_BINARY_DIR}/config/compat-config.h)

target_link_libraries(win-capture PRIVATE OBS::libobs OBS::ipc-util OBS::file-updater Jansson::Jansson)

set_target_properties(win-capture PROPERTIES FOLDER "plugins/win-capture")

if(MSVC)
  target_link_libraries(win-capture PRIVATE OBS::w32-pthreads)
  target_link_options(win-capture PRIVATE "LINKER:/IGNORE:4098")
endif()

target_compile_definitions(win-capture PRIVATE UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS
                                               OBS_VERSION="${OBS_VERSION_CANONICAL}" OBS_LEGACY)

set_property(GLOBAL APPEND PROPERTY OBS_MODULE_LIST "win-capture")

setup_plugin_target(win-capture)

add_subdirectory(graphics-hook)
add_subdirectory(get-graphics-offsets)
add_subdirectory(inject-helper)
