cmake_minimum_required(VERSION 3.24...3.25)

legacy_check()

add_executable(obs-qsv-test)

target_sources(obs-qsv-test PRIVATE obs-qsv-test.cpp)
target_link_libraries(obs-qsv-test d3d11 dxgi dxguid OBS::libmfx OBS::COMutils)

# cmake-format: off
set_target_properties_obs(obs-qsv-test PROPERTIES FOLDER plugins/obs-qsv11 PREFIX "")
# cmake-format: on
