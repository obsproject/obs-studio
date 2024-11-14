cmake_minimum_required(VERSION 3.24...3.25)

legacy_check()

find_package(FFnvcodec 12 REQUIRED)

add_executable(obs-nvenc-test)

target_sources(obs-nvenc-test PRIVATE obs-nvenc-test.c)
target_link_libraries(obs-nvenc-test OBS::obs-nvenc-version FFnvcodec::FFnvcodec d3d11 dxgi dxguid)

# cmake-format: off
set_target_properties_obs(obs-nvenc-test PROPERTIES FOLDER plugins/obs-ffmpeg)
# cmake-format: on
