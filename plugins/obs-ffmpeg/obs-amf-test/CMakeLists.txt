cmake_minimum_required(VERSION 3.28...3.30)

find_package(AMF 1.4.29 REQUIRED)

add_executable(obs-amf-test)
add_executable(OBS::amf-test ALIAS obs-amf-test)

if(OS_WINDOWS)
  target_sources(obs-amf-test PRIVATE obs-amf-test.cpp ../amf_gpu.hpp ../amf_windows.cpp)
endif()

if(OS_LINUX)
  target_sources(obs-amf-test PRIVATE obs-amf-test.cpp ../amf_gpu.hpp ../amf_linux.cpp)
  target_link_libraries(obs-amf-test PRIVATE OpenGL::EGL OpenGL::GL)
endif()

target_link_libraries(obs-amf-test PRIVATE AMF::AMF OBS::libobs)

set_target_properties_obs(obs-amf-test PROPERTIES FOLDER plugins/obs-ffmpeg)
