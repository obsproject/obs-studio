find_package(Qt6 REQUIRED Widgets)

add_library(browser-panels INTERFACE)
add_library(OBS::browser-panels ALIAS browser-panels)

target_sources(browser-panels INTERFACE panel/browser-panel.hpp)

target_include_directories(browser-panels INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/panel")

target_compile_definitions(browser-panels INTERFACE BROWSER_AVAILABLE)

target_sources(
  obs-browser
  PRIVATE # cmake-format: sortable
          panel/browser-panel-client.cpp panel/browser-panel-client.hpp panel/browser-panel-internal.hpp
          panel/browser-panel.cpp)

target_link_libraries(obs-browser PRIVATE OBS::browser-panels Qt::Widgets)

set_target_properties(
  obs-browser
  PROPERTIES AUTOMOC ON
             AUTOUIC ON
             AUTORCC ON)

if(OS_WINDOWS)
  set_property(SOURCE browser-app.hpp PROPERTY SKIP_AUTOMOC TRUE)
endif()
