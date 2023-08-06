project(obs-qsv-test)

include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/libobs)

add_executable(obs-qsv-test)

find_package(VPL 2.6 REQUIRED)

target_sources(obs-qsv-test PRIVATE obs-qsv-test.cpp)
target_link_libraries(obs-qsv-test d3d11 dxgi dxguid VPL::VPL)
target_link_options(obs-qsv-test PRIVATE /IGNORE:4099)

set_target_properties(obs-qsv-test PROPERTIES FOLDER "plugins/obs-qsv11")

setup_binary_target(obs-qsv-test)
