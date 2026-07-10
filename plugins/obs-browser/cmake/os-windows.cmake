target_compile_options(obs-browser PRIVATE $<IF:$<CONFIG:DEBUG>,/MTd,/MT>)
target_compile_definitions(obs-browser PRIVATE ENABLE_BROWSER_SHARED_TEXTURE)

target_link_libraries(obs-browser PRIVATE CEF::Wrapper CEF::Library ${CEF_STANDARD_LIBS} d3d11 dxgi)
target_link_options(obs-browser PRIVATE /IGNORE:4099)

configure_file(cmake/windows/obs-module.rc.in obs-browser.rc)
target_sources(obs-browser PRIVATE obs-browser.rc)

add_library(obs-browser-helper SHARED EXCLUDE_FROM_ALL)
add_library(OBS::browser-helper ALIAS obs-browser-helper)

target_sources(
  obs-browser-helper
  PRIVATE # cmake-format: sortable
          browser-app.cpp browser-app.hpp cef-headers.hpp obs-browser-page.manifest
          obs-browser-page/obs-browser-page-main.cpp)

configure_file(cmake/windows/obs-module-helper.rc.in obs-browser-page.rc)
target_sources(obs-browser-helper PRIVATE obs-browser-page.rc)

target_include_directories(obs-browser-helper PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/deps"
                                                      "${CMAKE_CURRENT_SOURCE_DIR}/obs-browser-page")

target_compile_options(obs-browser-helper PRIVATE $<IF:$<CONFIG:DEBUG>,/MTd,/MT>)
target_compile_features(obs-browser-helper PRIVATE cxx_std_20)
target_compile_definitions(obs-browser-helper PRIVATE ENABLE_BROWSER_SHARED_TEXTURE)
target_compile_definitions(obs-browser-helper PRIVATE CEF_USE_BOOTSTRAP)

target_link_libraries(obs-browser-helper PRIVATE CEF::Wrapper CEF::Library ${CEF_STANDARD_LIBS} nlohmann_json::nlohmann_json)
target_link_options(obs-browser-helper PRIVATE /IGNORE:4099 /SUBSYSTEM:WINDOWS)

target_link_libraries(obs-browser PRIVATE Ws2_32)
target_sources(obs-browser PRIVATE deps/ip-string-windows.cpp)

set(OBS_EXECUTABLE_DESTINATION "${OBS_PLUGIN_DESTINATION}")
set_target_properties_obs(
  obs-browser-helper
  PROPERTIES FOLDER plugins/obs-browser
             PREFIX ""
             OUTPUT_NAME obs-browser-page)

add_custom_command(
  TARGET obs-browser-helper
  POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CEF_ROOT_DIR}/Release/bootstrap.exe"
          "$<TARGET_FILE_DIR:obs-browser-helper>/obs-browser-page.exe"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${OBS_OUTPUT_DIR}/$<CONFIG>/${OBS_PLUGIN_DESTINATION}"
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:obs-browser-helper>"
          "${OBS_OUTPUT_DIR}/$<CONFIG>/${OBS_PLUGIN_DESTINATION}/obs-browser-page.dll"
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CEF_ROOT_DIR}/Release/bootstrap.exe"
          "${OBS_OUTPUT_DIR}/$<CONFIG>/${OBS_PLUGIN_DESTINATION}/obs-browser-page.exe"
)
