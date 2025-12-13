add_library(obs-websocket-api INTERFACE)
add_library(OBS::websocket-api ALIAS obs-websocket-api)

target_sources(obs-websocket-api INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/obs-websocket-api.h>
                                           $<INSTALL_INTERFACE:${OBS_INCLUDE_DESTINATION}/obs-websocket-api.h>)

target_link_libraries(obs-websocket-api INTERFACE OBS::libobs)

target_include_directories(obs-websocket-api INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib>"
                                                       "$<INSTALL_INTERFACE:${OBS_INCLUDE_DESTINATION}>")

set_target_properties(obs-websocket-api PROPERTIES PREFIX "" PUBLIC_HEADER lib/obs-websocket-api.h)

target_export(obs-websocket-api)
