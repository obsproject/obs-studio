project(linux-dmabuf)

include_directories(SYSTEM
	"${CMAKE_SOURCE_DIR}/libobs"
)

add_library(linux-dmabuf MODULE dmabuf.c)
target_link_libraries(linux-dmabuf libobs glad)
install_obs_plugin(linux-dmabuf)

find_package(PkgConfig)
pkg_check_modules(DRM libdrm)
if(NOT DRM_FOUND)
	message(FATAL_ERROR "libdrm not found")
endif()
add_executable(obs-drmsend drmsend.c)
target_include_directories(obs-drmsend PRIVATE ${DRM_INCLUDE_DIRS})
target_link_libraries(obs-drmsend PRIVATE ${DRM_LIBRARIES})

# this breaks building OBS under a regular user
# FIXME Make it a part of install process, which should run under root
#
#add_custom_command(TARGET drmsend POST_BUILD
#	COMMAND setcap cap_sys_admin+ep "$<TARGET_FILE:drmsend>")

install_obs_core(obs-drmsend)
