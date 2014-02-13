project(linux)

find_package(X11 REQUIRED)
find_package(PulseAudio REQUIRED)

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/libobs")

set(linux_SOURCES
    linux.c
    xcursor.c
    xshm-input.c
    pulse-input.c
)
set(linux_HEADERS
    xcursor.h
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
    ${PULSEAUDIO_LIBRARY}
)

install_obs_plugin(linux)
