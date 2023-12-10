project(get-graphics-offsets)

add_executable(get-graphics-offsets)
target_link_libraries(get-graphics-offsets PRIVATE _get-graphics-offsets)
set_property(TARGET get-graphics-offsets PROPERTY OUTPUT_NAME get-graphics-offsets32)
