cmake_minimum_required(VERSION 3.24...3.25)

if(OBS_CMAKE_VERSION VERSION_GREATER_EQUAL 3.0.0)
  option(ENABLE_VIRTUALCAM "Enable Windows Virtual Camera" ON)
  if(NOT ENABLE_VIRTUALCAM)
    target_disable_feature(obs-virtualcam-module "Virtual Camera Support")
    target_disable(obs-virtualcam-module)
    return()
  endif()

  if(NOT VIRTUALCAM_GUID)
    # cmake-format: off
    set(VIRTUALCAM_GUID "" CACHE STRING "Virtual Camera GUID" FORCE)
    mark_as_advanced(VIRTUALCAM_GUID)
    # cmake-format: on

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

  add_library(obs-virtualcam-guid INTERFACE)
  add_library(OBS::virtualcam-guid ALIAS obs-virtualcam-guid)

  configure_file(virtualcam-guid.h.in virtualcam-guid.h)
  target_sources(obs-virtualcam-guid INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/virtualcam-guid.h")
  target_include_directories(obs-virtualcam-guid INTERFACE "${CMAKE_CURRENT_BINARY_DIR}")

  add_library(_virtualcam INTERFACE)

  configure_file(cmake/windows/obs-module.rc.in virtualcam-module.rc)
  target_sources(
    _virtualcam
    INTERFACE # cmake-format: sortable
              placeholder.cpp sleepto.c sleepto.h virtualcam-filter.cpp virtualcam-filter.hpp virtualcam-module.cpp)
  target_include_directories(_virtualcam INTERFACE "${CMAKE_CURRENT_BINARY_DIR}")
  target_compile_definitions(_virtualcam INTERFACE VIRTUALCAM_AVAILABLE)
  set_property(TARGET _virtualcam PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

  target_link_libraries(
    _virtualcam
    INTERFACE OBS::virtualcam-interface
              OBS::virtualcam-guid
              OBS::libdshowcapture
              OBS::winhandle
              gdiplus
              strmiids
              winmm)
endif()

legacy_check()

add_library(obs-virtualcam-module MODULE)
add_library(OBS::virtualcam ALIAS obs-virtualcam-module)

target_sources(obs-virtualcam-module PRIVATE cmake/windows/virtualcam-module64.def)
target_link_libraries(obs-virtualcam-module PRIVATE _virtualcam)

configure_file(virtualcam-install.bat.in virtualcam-install.bat)
target_add_resource(obs-virtualcam-module "${CMAKE_CURRENT_BINARY_DIR}/virtualcam-install.bat"
                    "${OBS_DATA_DESTINATION}/obs-plugins/win-dshow")

configure_file(virtualcam-uninstall.bat.in virtualcam-uninstall.bat)
target_add_resource(obs-virtualcam-module "${CMAKE_CURRENT_BINARY_DIR}/virtualcam-uninstall.bat"
                    "${OBS_DATA_DESTINATION}/obs-plugins/win-dshow")

include(cmake/32bit.cmake)

# cmake-format: off
set_target_properties_obs(obs-virtualcam-module PROPERTIES FOLDER plugins/win-dshow OUTPUT_NAME obs-virtualcam-module64)
# cmake-format: on
