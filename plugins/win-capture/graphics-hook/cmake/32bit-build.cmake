project(graphics-hook)

if(NOT TARGET OBS::obfuscate)
  add_subdirectory("${CMAKE_SOURCE_DIR}/libobs" "${CMAKE_BINARY_DIR}/libobs")
endif()

add_library(graphics-hook MODULE)
target_link_libraries(graphics-hook PRIVATE _graphics-hook)

# cmake-format: off
set_target_properties(graphics-hook PROPERTIES OUTPUT_NAME graphics-hook32 MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
# cmake-format: on
