option(ENABLE_VIRTUALCAM "Enable Windows Virtual Camera" ON)
if(NOT ENABLE_VIRTUALCAM)
  return()
endif()

if(NOT VIRTUALCAM_GUID)
  set(VIRTUALCAM_GUID
      ""
      CACHE STRING "Virtual Camera GUID" FORCE)
  mark_as_advanced(VIRTUALCAM_GUID)

  message(WARNING "Empty Virtual Camera GUID set.")
  return()
else()
  set(VALID_GUID FALSE)
  check_uuid(${VIRTUALCAM_GUID} VALID_GUID)

  if(NOT VALID_GUID)
    message(WARNING "Invalid Virtual Camera GUID set.")
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

add_library(obs-virtualcam-module MODULE)
add_library(OBS::virtualcam ALIAS obs-virtualcam-module)

target_sources(obs-virtualcam-module PRIVATE cmake/windows/virtualcam-module32.def)
target_link_libraries(obs-virtualcam-module PRIVATE _virtualcam)

set_property(TARGET obs-virtualcam-module PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set_property(TARGET obs-virtualcam-module PROPERTY OUTPUT_NAME obs-virtualcam-module32)
