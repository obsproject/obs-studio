project(uthash)

add_library(uthash INTERFACE)
add_library(OBS::uthash ALIAS uthash)
set_target_properties(uthash PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                        "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(uthash PROPERTIES FOLDER "deps")
