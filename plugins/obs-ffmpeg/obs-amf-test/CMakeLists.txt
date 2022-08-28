project(obs-amf-test)

add_executable(obs-amf-test)

find_package(AMF 1.4.29 REQUIRED)

target_include_directories(obs-amf-test PRIVATE ${CMAKE_SOURCE_DIR}/libobs)

if(OS_WINDOWS)
  target_sources(obs-amf-test PRIVATE obs-amf-test.cpp)
  target_link_libraries(obs-amf-test d3d11 dxgi dxguid AMF::AMF)
elseif(OS_POSIX AND NOT OS_MACOS)
  find_package(Vulkan REQUIRED)
  target_sources(obs-amf-test PRIVATE obs-amf-test-linux.cpp)
  target_link_libraries(obs-amf-test dl Vulkan::Vulkan AMF::AMF)
endif()

set_target_properties(obs-amf-test PROPERTIES FOLDER "plugins/obs-ffmpeg")

setup_binary_target(obs-amf-test)
