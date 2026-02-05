cmake_minimum_required(VERSION 3.28...3.30)

add_library(win-asio MODULE)
add_library(OBS::asio ALIAS win-asio)
set(MODULE_DESCRIPTION "OBS ASIO module")

target_sources(
  win-asio
  PRIVATE asio-device.c asio-format.c asio-device-list.c asio-callbacks.c plugin-main.c win-asio.c
  PUBLIC asio-device.h asio-format.h asio-device-list.h asio-callbacks.h iasiodrv.h byteorder.h asio.h
)

target_link_libraries(win-asio PRIVATE OBS::libobs OBS::frontend-api)

configure_file(cmake/windows/obs-module.rc.in win-asio.rc)
target_sources(win-asio PRIVATE win-asio.rc)

set_property(TARGET win-asio APPEND PROPERTY AUTORCC_OPTIONS --format-version 1)
set_target_properties_obs(win-asio PROPERTIES FOLDER plugins PREFIX "")
