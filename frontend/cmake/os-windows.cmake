if(NOT TARGET OBS::blake2)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/blake2" "${CMAKE_BINARY_DIR}/deps/blake2")
endif()

if(NOT TARGET OBS::w32-pthreads)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/w32-pthreads" "${CMAKE_BINARY_DIR}/deps/w32-pthreads")
endif()

set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)
find_package(MbedTLS REQUIRED)
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG FALSE)
find_package(Detours REQUIRED)
find_package(nlohmann_json 3.11 REQUIRED)

configure_file(cmake/windows/obs.rc.in obs.rc)

target_sources(
  obs-studio
  PRIVATE
    cmake/windows/obs.manifest
    dialogs/OBSUpdate.cpp
    dialogs/OBSUpdate.hpp
    forms/OBSUpdate.ui
    obs.rc
    utility/AutoUpdateThread.cpp
    utility/AutoUpdateThread.hpp
    utility/CrashHandler_Windows.cpp
    utility/crypto-helpers-mbedtls.cpp
    utility/crypto-helpers.hpp
    utility/models/branches.hpp
    utility/models/whatsnew.hpp
    utility/platform-windows.cpp
    utility/system-info-windows.cpp
    utility/update-helpers.cpp
    utility/update-helpers.hpp
    utility/WhatsNewBrowserInitThread.cpp
    utility/WhatsNewBrowserInitThread.hpp
    utility/WhatsNewInfoThread.cpp
    utility/WhatsNewInfoThread.hpp
    utility/win-dll-blocklist.c
)

add_library(obs-updater-manifest INTERFACE)
add_library(OBS::updater-manifest ALIAS obs-updater-manifest)

target_sources(obs-updater-manifest INTERFACE updater/manifest.hpp)

target_link_libraries(
  obs-studio
  PRIVATE
    crypt32
    OBS::blake2
    OBS::updater-manifest
    OBS::w32-pthreads
    MbedTLS::mbedtls
    nlohmann_json::nlohmann_json
    Detours::Detours
)

target_compile_definitions(obs-studio PRIVATE PSAPI_VERSION=2)

target_link_options(obs-studio PRIVATE /IGNORE:4099 $<$<CONFIG:DEBUG>:/NODEFAULTLIB:MSVCRT>)

# Set commit for untagged version comparisons in the Windows updater
if(OBS_VERSION MATCHES ".+g[a-f0-9]+.*")
  string(REGEX REPLACE ".+g([a-f0-9]+).*$" "\\1" OBS_COMMIT ${OBS_VERSION})
else()
  set(OBS_COMMIT "")
endif()

set_source_files_properties(utility/AutoUpdateThread.cpp PROPERTIES COMPILE_DEFINITIONS OBS_COMMIT="${OBS_COMMIT}")

add_subdirectory(updater)

set_property(TARGET obs-studio APPEND PROPERTY AUTORCC_OPTIONS --format-version 1)

set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT obs-studio)
set_target_properties(
  obs-studio
  PROPERTIES
    WIN32_EXECUTABLE TRUE
    VS_DEBUGGER_COMMAND "${CMAKE_BINARY_DIR}/rundir/$<CONFIG>/bin/64bit/$<TARGET_FILE_NAME:obs-studio>"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/rundir/$<CONFIG>/bin/64bit"
)
