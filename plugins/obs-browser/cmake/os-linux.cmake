find_package(X11 REQUIRED)
find_package(Libdrm REQUIRED)
get_target_property(libdrm_include_directories Libdrm::Libdrm INTERFACE_INCLUDE_DIRECTORIES)

target_include_directories(obs-browser PRIVATE ${libdrm_include_directories})

target_link_libraries(obs-browser PRIVATE CEF::Wrapper CEF::Library X11::X11)
set_target_properties(obs-browser PROPERTIES BUILD_RPATH "$ORIGIN/" INSTALL_RPATH "$ORIGIN/")

target_sources(obs-browser PRIVATE drm-format.cpp drm-format.hpp)

add_executable(browser-helper)
add_executable(OBS::browser-helper ALIAS browser-helper)

target_sources(
  browser-helper PRIVATE # cmake-format: sortable
                         browser-app.cpp browser-app.hpp cef-headers.hpp obs-browser-page/obs-browser-page-main.cpp)

target_include_directories(browser-helper PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/deps"
                                                  "${CMAKE_CURRENT_SOURCE_DIR}/obs-browser-page")

target_link_libraries(browser-helper PRIVATE CEF::Wrapper CEF::Library)

target_sources(obs-browser PRIVATE deps/ip-string-posix.cpp)

set(OBS_EXECUTABLE_DESTINATION "${OBS_PLUGIN_DESTINATION}")

# cmake-format: off
set_target_properties_obs(
  browser-helper
  PROPERTIES FOLDER plugins/obs-browser
             BUILD_RPATH "$ORIGIN/"
             INSTALL_RPATH "$ORIGIN/"
             PREFIX ""
             OUTPUT_NAME obs-browser-page)
# cmake-format: on
