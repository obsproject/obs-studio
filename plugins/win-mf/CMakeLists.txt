project(win-mf)

set(
  win-mf_SOURCES
  mf-plugin.cpp
  mf-common.cpp
  mf-encoder-descriptor.cpp
  mf-h264.cpp
  mf-h264-encoder.cpp
  mf-hevc.cpp
  mf-hevc-encoder.cpp
  mf-aac.cpp
  mf-aac-encoder.cpp
)

set(
  win-mf_HEADERS
  mf-common.hpp
  mf-encoder-descriptor.hpp
  mf-h264-encoder.hpp
  mf-hevc-encoder.hpp
  mf-aac-encoder.hpp
)

add_library(win-mf MODULE ${win-mf_SOURCES} ${win-mf_HEADERS})

target_link_libraries(
  win-mf
  d3d9
  dxva2
  uuid
  mfplat
  mfuuid
  mf
  wmcodecdspuuid
  libobs
)
set_target_properties_obs(win-mf PROPERTIES FOLDER "plugins" PREFIX "")
