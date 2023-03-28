cmake_minimum_required(VERSION 3.24...3.25)

legacy_check()

find_package(AMF 1.4.29 REQUIRED)

add_executable(obs-amf-test)
add_executable(OBS::amf-test ALIAS obs-amf-test)

target_sources(obs-amf-test PRIVATE obs-amf-test.cpp)
target_link_libraries(obs-amf-test PRIVATE OBS::COMutils AMF::AMF d3d11 dxgi dxguid)

# cmake-format: off
set_target_properties_obs(obs-amf-test PROPERTIES FOLDER plugins/obs-ffmpeg)
# cmake-format: on
