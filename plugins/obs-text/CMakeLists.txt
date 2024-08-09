cmake_minimum_required(VERSION 3.24...3.25)

add_library(obs-text MODULE)
add_library(OBS::text ALIAS obs-text)

configure_file(cmake/windows/obs-module.rc.in obs-text.rc)

target_sources(obs-text PRIVATE gdiplus/obs-text.cpp obs-text.rc)

target_link_libraries(obs-text PRIVATE OBS::libobs gdiplus)

set_target_properties_obs(obs-text PROPERTIES FOLDER plugins PREFIX "")
