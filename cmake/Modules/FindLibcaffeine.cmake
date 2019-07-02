# find_package handler for libcaffeine
#
# Parameters
# - LIBCAFFEINE_DIR: Path to libcaffeine source or cpack package
#
# Variables:
# - LIBCAFFEINE_FOUND: Boolean, TRUE if libcaffeine was FOUND_VAR
# - All variables defined by libcaffeineConfig.cmake
#
# Defined Targets:
# - libcaffeine
#

set(LIBCAFFEINE_DIR "" CACHE PATH "Path to libcaffeine")
set(LIBCAFFEINE_FOUND FALSE)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	set(LIBCAFFEINE_REQUIRES_LIBRARY FALSE)
else()
	set(LIBCAFFEINE_REQUIRES_LIBRARY TRUE)
endif()

function(find_libcaffeine_cpack)
	math(EXPR BITS "8*${CMAKE_SIZEOF_VOID_P}")

	find_file(LIBCAFFEINE_CMAKE_FILE
		NAMES
			libcaffeineConfig.cmake
		HINTS
			${DepsPath}
			ENV DepsPath
			${DepsPath${BITS}}
			ENV DepsPath${BITS}
			${LIBCAFFEINE_DIR}
			ENV LIBCAFFEINE_DIR
			${LIBCAFFEINE_DIR${BITS}}
			ENV LIBCAFFEINE_DIR${BITS}			
		PATHS
			/usr/include
			/usr/local/include
			/opt/local/include
			/opt/local
			/sw/include
			~/Library/Frameworks
			/Library/Frameworks
		PATH_SUFFIXES
			lib
			lib${BITS}
			lib/cmake
			lib${BITS}/cmake
			lib/cmake${BITS}
			lib${BITS}/cmake${BITS}
			cmake
			cmake${BITS}
	)

	if(EXISTS "${LIBCAFFEINE_CMAKE_FILE}")
		set(LIBCAFFEINE_FOUND TRUE PARENT_SCOPE)
		
		return()
	endif()
endfunction()
include(FindPackageHandleStandardArgs)

# Find by CPack
find_libcaffeine_cpack()
find_package_handle_standard_args(
	LIBCAFFEINE
	FOUND_VAR LIBCAFFEINE_FOUND
	REQUIRED_VARS
		LIBCAFFEINE_CMAKE_FILE
)

include("${LIBCAFFEINE_CMAKE_FILE}")
