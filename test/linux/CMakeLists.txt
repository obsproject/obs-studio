project(linux)

find_package(X11 REQUIRED)

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/libobs")
#include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/libobs-opengl")

set(linux_SOURCES
    linux.c
    xshm-input.c
)
set(linux_HEADERS
    linux.h
    xshm-input.h
)
    
add_library(linux MODULE
    ${linux_SOURCES}
    ${linux_HEADERS}
)
target_link_libraries(linux
    libobs
    ${X11_LIBRARIES}
    ${X11_XShm_LIB}
)

install_obs_plugin(linux)

#obs_fixup_install_target(xshm-input PATH ${Libx264_LIBRARIES})
