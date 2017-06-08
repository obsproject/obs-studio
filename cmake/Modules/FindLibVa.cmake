# Once done these will be defined:
#
#  LIBVA_FOUND
#  LIBVA_INCLUDE_DIRS
#  LIBVA_LIBRARIES

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(_VA QUIET va)
	pkg_check_modules(_VA_X11 QUIET va-x11)
	pkg_check_modules(_VA_DRM QUIET va-drm)
endif()

find_path(VA_INCLUDE_DIR
	NAMES va/va.h
	HINTS
		${_VA_INCLUDE_DIRS}
	PATHS
		/usr/include /usr/local/include /opt/local/include)

find_library(VA_LIB
	NAMES va
	HINTS
		${_VA_LIBRARY_DIRS}
	PATHS
		/usr/lib /usr/local/lib /opt/local/lib)

find_library(VA_X11_LIB
	NAMES va-x11
	HINTS
		${_VA_X11_LIBRARY_DIRS}
	PATHS
		/usr/lib /usr/local/lib /opt/local/lib)

find_library(VA_DRM_LIB
	NAMES va-drm
	HINTS
		${_VA_DRM_LIBRARY_DIRS}
	PATHS
		/usr/lib /usr/local/lib /opt/local/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
	LibVa DEFAULT_MSG 
	VA_LIB 
	VA_INCLUDE_DIR 
	VA_X11_LIB
	VA_DRM_LIB)

mark_as_advanced(VA_INCLUDE_DIR
	VA_LIB
	VA_X11_LIB
	VA_DRM_LIB)

if(LIBVA_FOUND)
	set(LIBVA_INCLUDE_DIRS ${VA_INCLUDE_DIR})
	set(LIBVA_LIBRARIES 
		${VA_LIB} 
		${VA_X11_LIB}
		${VA_DRM_LIB})
endif()
