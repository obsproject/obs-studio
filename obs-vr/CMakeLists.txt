project(obs-vr)

add_executable(obs-vr
    main.cpp
)

target_link_libraries(obs-vr PRIVATE
    libvr
    nlohmann_json::nlohmann_json
)

set_target_properties(obs-vr PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
