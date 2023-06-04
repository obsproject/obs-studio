project(inject-helper)

add_executable(inject-helper)

target_sources(inject-helper PRIVATE inject-helper.c ../inject-library.c ../inject-library.h
                                     ../../../libobs/util/windows/obfuscate.c ../../../libobs/util/windows/obfuscate.h)

target_include_directories(inject-helper PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

if(MSVC)
  target_compile_options(inject-helper PRIVATE "$<IF:$<CONFIG:Debug>,/MTd,/MT>")
endif()

set_target_properties(inject-helper PROPERTIES FOLDER "plugins/win-capture"
                                               OUTPUT_NAME "inject-helper$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,64,32>")

set(OBS_PLUGIN_DESTINATION "${OBS_DATA_DESTINATION}/obs-plugins/win-capture/")
setup_plugin_target(inject-helper)

add_dependencies(win-capture inject-helper)
