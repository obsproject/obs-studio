project(inject-helper)

add_executable(inject-helper)
target_link_libraries(inject-helper PRIVATE _inject-helper)
set_property(TARGET inject-helper PROPERTY OUTPUT_NAME inject-helper32)
