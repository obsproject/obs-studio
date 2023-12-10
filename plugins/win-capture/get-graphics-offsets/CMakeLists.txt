cmake_minimum_required(VERSION 3.24...3.25)

add_library(_get-graphics-offsets INTERFACE)
target_sources(_get-graphics-offsets INTERFACE d3d8-offsets.cpp d3d9-offsets.cpp dxgi-offsets.cpp
                                               get-graphics-offsets.c get-graphics-offsets.h)

target_compile_options(_get-graphics-offsets INTERFACE $<IF:$<CONFIG:Debug>,/MTd,/MT>)

target_link_libraries(_get-graphics-offsets INTERFACE OBS::hook-config OBS::d3d8-api d3d9.lib dxgi.lib d3d11.lib)

legacy_check()

add_executable(get-graphics-offsets)
target_link_libraries(get-graphics-offsets PRIVATE _get-graphics-offsets)

include(cmake/32bit.cmake)

# cmake-format: off
set_target_properties_obs(get-graphics-offsets PROPERTIES FOLDER plugins/win-capture OUTPUT_NAME get-graphics-offsets64)
# cmake-format: on

add_dependencies(win-capture get-graphics-offsets)
