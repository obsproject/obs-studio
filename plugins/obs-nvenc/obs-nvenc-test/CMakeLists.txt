cmake_minimum_required(VERSION 3.24...3.25)

find_package(FFnvcodec 12 REQUIRED)

add_executable(obs-nvenc-test)

target_sources(obs-nvenc-test PRIVATE obs-nvenc-test.cpp)
target_link_libraries(obs-nvenc-test FFnvcodec::FFnvcodec)

set_target_properties_obs(obs-nvenc-test PROPERTIES FOLDER plugins/obs-nvenc)
