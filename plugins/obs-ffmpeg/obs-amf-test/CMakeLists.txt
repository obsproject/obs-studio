project(obs-amf-test)

include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/libobs)

add_executable(obs-amf-test)
target_sources(obs-amf-test PRIVATE obs-amf-test.cpp)
target_link_libraries(obs-amf-test d3d11 dxgi dxguid)

set_target_properties(obs-amf-test PROPERTIES FOLDER "plugins/obs-ffmpeg")

setup_binary_target(obs-amf-test)
