project(obs-nvenc-test)

add_executable(obs-nvenc-test)

find_package(FFnvcodec 12 REQUIRED)

target_sources(obs-nvenc-test PRIVATE jim-nvenc-test.c ../jim-nvenc-ver.h)
target_link_libraries(obs-nvenc-test d3d11 dxgi dxguid FFnvcodec::FFnvcodec)

set_target_properties(obs-nvenc-test PROPERTIES FOLDER "plugins/obs-ffmpeg")

setup_binary_target(obs-nvenc-test)
