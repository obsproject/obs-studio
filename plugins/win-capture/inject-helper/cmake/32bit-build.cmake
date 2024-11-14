project(inject-helper)

add_executable(inject-helper)
target_link_libraries(inject-helper PRIVATE _inject-helper)

# cmake-format: off
set_target_properties(inject-helper PROPERTIES OUTPUT_NAME inject-helper32 MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
# cmake-format: on
