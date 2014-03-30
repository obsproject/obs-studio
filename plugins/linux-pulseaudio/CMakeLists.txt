project(linux-pulseaudio)

find_package(PulseAudio REQUIRED)

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/libobs")

set(linux-pulseaudio_SOURCES
	linux-pulseaudio.c
	pulse-input.c
)

add_library(linux-pulseaudio MODULE
	${linux-pulseaudio_SOURCES}
)
target_link_libraries(linux-pulseaudio
	libobs
	${PULSEAUDIO_LIBRARY}
)

install_obs_plugin(linux-pulseaudio)
