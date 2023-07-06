project(graphics-hook)

if(NOT TARGET OBS::obfuscate)
  add_subdirectory("${CMAKE_SOURCE_DIR}/libobs" "${CMAKE_BINARY_DIR}/libobs")
endif()

add_library(graphics-hook MODULE)
target_link_libraries(graphics-hook PRIVATE _graphics-hook)
set_property(TARGET graphics-hook PROPERTY OUTPUT_NAME graphics-hook32)
