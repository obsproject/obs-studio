project(win-mf)

set(
  win-mf_SOURCES
  mf-plugin.cpp
  mf-common.cpp
  mf-encoder-descriptor.cpp
)

set(
  win-mf_HEADERS
  mf-common.hpp
  mf-encoder-descriptor.hpp
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
