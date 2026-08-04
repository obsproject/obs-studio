cmake_minimum_required(VERSION 3.28...3.30)

option(ENABLE_VIRTUALCAM "Enable Windows Virtual Camera" ON)
if(NOT ENABLE_VIRTUALCAM)
  target_disable_feature(obs-virtualcam-module "Virtual Camera Support")
  target_disable(obs-virtualcam-module)
  return()
endif()

if(NOT VIRTUALCAM_GUID)
  set(VIRTUALCAM_GUID "" CACHE STRING "Virtual Camera GUID" FORCE)
  mark_as_advanced(VIRTUALCAM_GUID)

  message(WARNING "Empty Virtual Camera GUID set.")
  target_disable_feature(obs-virtualcam-module "Virtual Camera Support (empty GUID)")
  return()
else()
  set(VALID_GUID FALSE)
  check_uuid(${VIRTUALCAM_GUID} VALID_GUID)

  if(NOT VALID_GUID)
    message(WARNING "Invalid Virtual Camera GUID set.")
    target_disable_feature(obs-virtualcam-module "Virtual Camera Support (invalid GUID)")
    return()
  endif()

  # DirectShow API requires separate GUID tokens
  string(REPLACE "-" ";" GUID_VALS ${VIRTUALCAM_GUID})
  list(GET GUID_VALS 0 GUID_VALS_DATA1)
  list(GET GUID_VALS 1 GUID_VALS_DATA2)
  list(GET GUID_VALS 2 GUID_VALS_DATA3)
  list(GET GUID_VALS 3 GUID_VALS_DATA4)
  list(GET GUID_VALS 4 GUID_VALS_DATA5)
  set(GUID_VAL01 ${GUID_VALS_DATA1})
  set(GUID_VAL02 ${GUID_VALS_DATA2})
  set(GUID_VAL03 ${GUID_VALS_DATA3})
  string(SUBSTRING ${GUID_VALS_DATA4} 0 2 GUID_VAL04)
  string(SUBSTRING ${GUID_VALS_DATA4} 2 2 GUID_VAL05)
  string(SUBSTRING ${GUID_VALS_DATA5} 0 2 GUID_VAL06)
  string(SUBSTRING ${GUID_VALS_DATA5} 2 2 GUID_VAL07)
  string(SUBSTRING ${GUID_VALS_DATA5} 4 2 GUID_VAL08)
  string(SUBSTRING ${GUID_VALS_DATA5} 6 2 GUID_VAL09)
  string(SUBSTRING ${GUID_VALS_DATA5} 8 2 GUID_VAL10)
  string(SUBSTRING ${GUID_VALS_DATA5} 10 2 GUID_VAL11)
endif()

if(NOT TARGET OBS::winhandle)
  add_subdirectory("${CMAKE_SOURCE_DIR}/libobs" "${CMAKE_BINARY_DIR}/libobs")
endif()

if(NOT TARGET OBS::threading-windows)
  add_subdirectory("${CMAKE_SOURCE_DIR}/libobs" "${CMAKE_BINARY_DIR}/libobs")
endif()

if(NOT TARGET OBS::tiny-nv12-scale)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/obs-tiny-nv12-scale" obs-tiny-nv12-scale)
endif()

if(NOT TARGET OBS::shared-memory-queue)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/obs-shared-memory-queue" obs-shared-memory-queue)
endif()

if(NOT TARGET OBS::libdshowcapture)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/libdshowcapture" libdshowcapture)
endif()

add_library(obs-virtualcam-guid INTERFACE)
add_library(OBS::virtualcam-guid ALIAS obs-virtualcam-guid)

configure_file(virtualcam-guid.h.in virtualcam-guid.h)
target_sources(obs-virtualcam-guid INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/virtualcam-guid.h")
target_include_directories(obs-virtualcam-guid INTERFACE "${CMAKE_CURRENT_BINARY_DIR}")

add_library(obs-virtualcam-module MODULE)
add_library(OBS::virtualcam ALIAS obs-virtualcam-module)

configure_file(cmake/windows/obs-module.rc.in virtualcam-module.rc)
target_sources(
  obs-virtualcam-module
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/virtualcam-module.rc
    placeholder.cpp
    sleepto.c
    sleepto.h
    virtualcam-filter.cpp
    virtualcam-filter.hpp
    virtualcam-module.cpp
)

target_include_directories(obs-virtualcam-module PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_definitions(obs-virtualcam-module PRIVATE VIRTUALCAM_AVAILABLE)

target_link_libraries(
  obs-virtualcam-module
  PRIVATE
    OBS::tiny-nv12-scale
    OBS::shared-memory-queue
    OBS::virtualcam-guid
    OBS::libdshowcapture
    OBS::winhandle
    OBS::threading-windows
    gdiplus
    strmiids
    winmm
)

if(OBS_PARENT_ARCHITECTURE STREQUAL CMAKE_VS_PLATFORM_NAME)
  if(CMAKE_VS_PLATFORM_NAME STREQUAL ARM64)
    add_custom_command(
      TARGET obs-virtualcam-module
      POST_BUILD
      COMMAND "${CMAKE_COMMAND}" --build ${CMAKE_SOURCE_DIR}/build_x64 --config $<CONFIG> -t obs-virtualcam-module
      COMMENT "Build x64 obs-virtualcam-module"
    )
  endif()

  if(CMAKE_VS_PLATFORM_NAME MATCHES "(ARM64|x64)")
    add_custom_command(
      TARGET obs-virtualcam-module
      POST_BUILD
      COMMAND "${CMAKE_COMMAND}" --build ${CMAKE_SOURCE_DIR}/build_x86 --config $<CONFIG> -t obs-virtualcam-module
      COMMENT "Build x86 obs-virtualcam-module"
    )
  endif()

  # On ARM64, rebuild the module as an ARM64X hybrid image, replacing the plain
  # ARM64 DLL in place. Native-ARM64 and emulated-x64 processes share a single
  # 64-bit COM registration view, so the one InprocServer32 slot can point at
  # only one architecture. An ARM64X image holds both an arm64 and an arm64ec
  # slice, letting that single registration serve both kinds of consumer.
  # See cmake/windows/build-arm64x.cmake for the mechanism.
  if(CMAKE_VS_PLATFORM_NAME STREQUAL ARM64)
    # Derive the ARM64 cross tools and vcvarsall from the configured linker:
    #   <VC>/Tools/MSVC/<ver>/bin/Hostx64/arm64/link.exe
    cmake_path(GET CMAKE_LINKER PARENT_PATH _arm64x_bindir)
    set(ARM64X_LINK "${CMAKE_LINKER}")
    set(ARM64X_CL "${_arm64x_bindir}/cl.exe")
    string(REGEX REPLACE "/VC/Tools/MSVC/.*$" "/VC/Auxiliary/Build/vcvarsall.bat" VCVARSALL "${CMAKE_LINKER}")

    # configure_file() cannot expand $<CONFIG>, so the wrapper takes the real
    # configuration as argument 1 and substitutes it for %CONFIG% at build time.
    set(ARM64X_NATIVE_OBJ_DIR "${CMAKE_CURRENT_BINARY_DIR}/obs-virtualcam-module.dir/%CONFIG%")
    set(ARM64X_EC_OBJ_DIR "${CMAKE_CURRENT_BINARY_DIR}/arm64ec.dir/%CONFIG%")
    set(ARM64X_WORK_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    set(ARM64X_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/windows/build-arm64x.cmake")
    # Reuse the native ARM64 exports: an ARM64X image needs the same six COM
    # entry points in both of its views.
    set(ARM64X_DEF "${CMAKE_CURRENT_SOURCE_DIR}/cmake/windows/virtualcam-module-arm64.def")
    # Overwrite the module the native link just produced, PDB included: the native
    # PDB does not describe the hybrid image, and it is packaged next to the DLL.
    set(ARM64X_OUT_DLL "${CMAKE_CURRENT_BINARY_DIR}/%CONFIG%/obs-virtualcam-module-arm64.dll")
    set(ARM64X_OUT_PDB "${CMAKE_CURRENT_BINARY_DIR}/%CONFIG%/obs-virtualcam-module-arm64.pdb")

    # vcvarsall takes <host>_<target>; on an ARM64 host this is a native build.
    if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(ARM64|arm64|aarch64)$")
      set(ARM64X_VCVARS_ARG arm64)
    else()
      set(ARM64X_VCVARS_ARG amd64_arm64)
    endif()
    # Mirrors the native link: the libraries linked above plus the Win32 defaults.
    set(ARM64X_SYSTEM_LIBS "gdiplus.lib strmiids.lib winmm.lib ${CMAKE_CXX_STANDARD_LIBRARIES}")

    configure_file(cmake/windows/build-arm64x.bat.in build-arm64x.bat @ONLY)

    # Under the VS generator the command runs inside a .rule via cmd, which
    # resolves a bare .bat first token as a relative command and fails to find
    # it. Go through `cmd /c call "<native abs path>"` instead.
    cmake_path(NATIVE_PATH CMAKE_CURRENT_BINARY_DIR _arm64x_bindir_native)
    add_custom_command(
      TARGET obs-virtualcam-module
      POST_BUILD
      COMMAND cmd /c call "${_arm64x_bindir_native}\\build-arm64x.bat" $<CONFIG>
      COMMENT "Build ARM64X obs-virtualcam-module"
      VERBATIM
    )
  endif()

  add_dependencies(win-dshow obs-virtualcam-module)
endif()

if(CMAKE_VS_PLATFORM_NAME STREQUAL ARM64)
  set(_OUTPUT_NAME virtualcam-module-arm64)
elseif(CMAKE_VS_PLATFORM_NAME STREQUAL x64)
  set(_OUTPUT_NAME virtualcam-module64)
else()
  set(_OUTPUT_NAME virtualcam-module32)
endif()

target_sources(obs-virtualcam-module PRIVATE cmake/windows/${_OUTPUT_NAME}.def)

configure_file(virtualcam-install.bat.in virtualcam-install.bat)
target_add_resource(obs-virtualcam-module "${CMAKE_CURRENT_BINARY_DIR}/virtualcam-install.bat"
                    "${OBS_DATA_DESTINATION}/obs-plugins/win-dshow"
)

configure_file(virtualcam-uninstall.bat.in virtualcam-uninstall.bat)
target_add_resource(obs-virtualcam-module "${CMAKE_CURRENT_BINARY_DIR}/virtualcam-uninstall.bat"
                    "${OBS_DATA_DESTINATION}/obs-plugins/win-dshow"
)

set_target_properties_obs(
  obs-virtualcam-module PROPERTIES
                        FOLDER plugins/win-dshow
                        OUTPUT_NAME obs-${_OUTPUT_NAME}
                        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)
