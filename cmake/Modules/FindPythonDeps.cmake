# Once done these will be defined:
#
#  PYTHON_FOUND
#  PYTHON_INCLUDE_DIRS
#  PYTHON_LIBRARIES
#
# For use in OBS:
#
#  PYTHON_INCLUDE_DIR

if(NOT WIN32)
	set(Python_ADDITIONAL_VERSIONS 3.4)
	find_package(PythonLibs QUIET 3.4)
	return()
endif()

IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
	SET(_LIB_SUFFIX 64)
ELSE()
	SET(_LIB_SUFFIX 32)
ENDIF()

FIND_PATH(PYTHON_INCLUDE_DIR
	NAMES Python.h
	HINTS
		ENV PythonPath${_LIB_SUFFIX}
		ENV PythonPath
		ENV DepsPath${_LIB_SUFFIX}
		ENV DepsPath
		${PythonPath${_LIB_SUFFIX}}
		${PythonPath}
		${DepsPath${_LIB_SUFFIX}}
		${DepsPath}
		${_PYTHON_INCLUDE_DIRS}
	PATH_SUFFIXES
		include
		include/python
		)

find_library(PYTHON_LIB
	NAMES ${_PYTHON_LIBRARIES} python36
	HINTS
		ENV PythonPath${_lib_suffix}
		ENV PythonPath
		ENV DepsPath${_lib_suffix}
		ENV DepsPath
		${PythonPath${_lib_suffix}}
		${PythonPath}
		${DepsPath${_lib_suffix}}
		${DepsPath}
		${_PYTHON_LIBRARY_DIRS}
	PATH_SUFFIXES
		lib${_lib_suffix} lib
		libs${_lib_suffix} libs
		bin${_lib_suffix} bin
		../lib${_lib_suffix} ../lib
		../libs${_lib_suffix} ../libs
		../bin${_lib_suffix} ../bin)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Python DEFAULT_MSG PYTHON_LIB PYTHON_INCLUDE_DIR)
mark_as_advanced(PYTHON_INCLUDE_DIR PYTHON_LIB)

if(PYTHON_FOUND)
	set(PYTHON_INCLUDE_DIRS ${PYTHON_INCLUDE_DIR})
	set(PYTHON_LIBRARIES ${PYTHON_LIB})
	set(PYTHONLIBS_FOUND TRUE)
endif()
