project(obs-x265)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/source)

add_library(obs-x265 MODULE)
add_executable(obs-x265-test)

target_sources(obs-x265-test PRIVATE obs-x265-test.c)

target_link_libraries(obs-x265-test PRIVATE OBS::opts-parser)

target_sources(obs-x265 PRIVATE obs-x265.c obs-x265-plugin-main.c)

target_link_libraries(obs-x265 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source/Release/x265-static.lib OBS::opts-parser)

set_target_properties(obs-x265 PROPERTIES FOLDER "plugins" PREFIX "")

target_compile_options(
  obs-x265
  PRIVATE
    $<$<OR:$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:AppleClang>,$<C_COMPILER_ID:GNU>>:-Wno-switch>
)

if(OS_WINDOWS)
  set(MODULE_DESCRIPTION "OBS x265 encoder")
  configure_file(${CMAKE_SOURCE_DIR}/cmake/bundle/windows/obs-module.rc.in
                 obs-x265.rc)

  target_sources(obs-x265 PRIVATE obs-x265.rc)
endif()

set_target_properties(obs-x265-test PROPERTIES FOLDER "plugins")
add_test(NAME obs-x265-test COMMAND obs-x265-test)

setup_plugin_target(obs-x265)
