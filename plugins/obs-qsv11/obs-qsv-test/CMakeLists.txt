cmake_minimum_required(VERSION 3.28...3.30)

add_executable(obs-qsv-test)

find_package(VPL 2.9 REQUIRED)

target_sources(obs-qsv-test PRIVATE obs-qsv-test.cpp)

target_compile_definitions(obs-qsv-test PRIVATE "$<$<BOOL:${ENABLE_HEVC}>:ENABLE_HEVC>")

target_link_libraries(
  obs-qsv-test
  d3d11
  dxgi
  dxguid
  VPL::VPL
  OBS::COMutils
)

target_link_options(obs-qsv-test PRIVATE /IGNORE:4099)

set_target_properties_obs(obs-qsv-test PROPERTIES FOLDER plugins/obs-qsv11 PREFIX "")
