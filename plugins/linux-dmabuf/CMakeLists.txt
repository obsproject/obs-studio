project(linux-dmabuf)

include_directories(SYSTEM
	"${CMAKE_SOURCE_DIR}/libobs"
)

set(SOURCES dmabuf.c)

add_library(linux-dmabuf MODULE ${SOURCES})

target_link_libraries(linux-dmabuf libobs glad)

install_obs_plugin(linux-dmabuf)
