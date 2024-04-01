project(get-graphics-offsets)

add_executable(get-graphics-offsets)
target_link_libraries(get-graphics-offsets PRIVATE _get-graphics-offsets)

# cmake-format: off
set_target_properties(get-graphics-offsets PROPERTIES OUTPUT_NAME get-graphics-offsets32 MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
# cmake-format: on
