cmake_minimum_required(VERSION 3.28...3.30)

find_package(Qt6 REQUIRED Widgets)

add_library(asio-output-ui MODULE)
add_library(OBS::asio-output-ui ALIAS asio-output-ui)

target_sources(asio-output-ui PRIVATE asio-ui-main.cpp ASIOSettingsDialog.cpp ASIOSettingsDialog.h)

target_sources(asio-output-ui PRIVATE forms/output.ui)

target_link_libraries(asio-output-ui PRIVATE OBS::libobs OBS::frontend-api OBS::properties-view Qt::Widgets)

configure_file(cmake/windows/obs-module.rc.in asio-output-ui.rc)
target_sources(asio-output-ui PRIVATE asio-output-ui.rc)

set_property(TARGET asio-output-ui APPEND PROPERTY AUTORCC_OPTIONS --format-version 1)

set_target_properties_obs(
  asio-output-ui
  PROPERTIES FOLDER frontend
             PREFIX ""
             AUTOMOC ON
             AUTOUIC ON
             AUTORCC ON
             AUTOUIC_SEARCH_PATHS forms
)
