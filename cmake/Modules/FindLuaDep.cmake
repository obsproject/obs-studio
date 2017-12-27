# Once done these will be defined:
#
#  LUA_FOUND
#  LUA_INCLUDE_DIRS
#  LUA_LIBRARIES
#
# For use in OBS: 
#
#  LUA_INCLUDE_DIR

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
	find_package(Lua QUIET 5.2)
	if (LUA_FOUND)
		return()
	endif()
	pkg_check_modules(_LUA QUIET LUA>=5.2 LIBLUA>=5.2)
ENDIF()

IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
	SET(_LIB_SUFFIX 64)
ELSE()
	SET(_LIB_SUFFIX 32)
ENDIF()

FIND_PATH(LUA_INCLUDE_DIR
	NAMES lua.h
	HINTS
		ENV LuaPath${_LIB_SUFFIX}
		ENV LuaPath
		ENV DepsPath${_LIB_SUFFIX}
		ENV DepsPath
		${LuaPath${_LIB_SUFFIX}}
		${LuaPath}
		${DepsPath${_LIB_SUFFIX}}
		${DepsPath}
		${_LUA_INCLUDE_DIRS}
	PATHS
		/usr/include /usr/local/include /opt/local/include /sw/include
	PATH_SUFFIXES
		include lua lua/src include/lua include/lua/src)

find_library(LUA_LIB
	NAMES ${_LUA_LIBRARIES} lua liblua
	HINTS
		ENV LuaPath${_lib_suffix}
		ENV LuaPath
		ENV DepsPath${_lib_suffix}
		ENV DepsPath
		${LuaPath${_lib_suffix}}
		${LuaPath}
		${DepsPath${_lib_suffix}}
		${DepsPath}
		${_LUA_LIBRARY_DIRS}
	PATHS
		/usr/lib /usr/local/lib /opt/local/lib /sw/lib
	PATH_SUFFIXES
		lib${_lib_suffix} lib
		libs${_lib_suffix} libs
		bin${_lib_suffix} bin
		../lib${_lib_suffix} ../lib
		../libs${_lib_suffix} ../libs
		../bin${_lib_suffix} ../bin)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Lua DEFAULT_MSG LUA_LIB LUA_INCLUDE_DIR)
mark_as_advanced(LUA_INCLUDE_DIR LUA_LIB)

if(LUA_FOUND)
	set(LUA_INCLUDE_DIRS ${LUA_INCLUDE_DIR})
	set(LUA_LIBRARIES ${LUA_LIB})
endif()
