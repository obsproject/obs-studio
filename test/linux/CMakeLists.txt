project(linux)

find_package(X11 REQUIRED)

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/libobs")

set(linux_SOURCES
    linux.c
    xcursor.c
    xshm-input.c
)
set(linux_HEADERS
    linux.h
    xcursor.h
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
    ${X11_Xfixes_LIB}
)

install_obs_plugin(linux)
