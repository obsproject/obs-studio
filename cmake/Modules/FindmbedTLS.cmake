# Once done these will be defined:
#
#  LIBMBEDTLS_FOUND
#  LIBMBEDTLS_INCLUDE_DIRS
#  LIBMBEDTLS_LIBRARIES
#
# For use in OBS: 
#
#  MBEDTLS_INCLUDE_DIR

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(_MBEDTLS QUIET mbedtls)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(_lib_suffix 64)
else()
	set(_lib_suffix 32)
endif()

find_path(MBEDTLS_INCLUDE_DIR
	NAMES ssl.h
	HINTS
		ENV mbedtlsPath${_lib_suffix}
		ENV mbedtlsPath
		ENV DepsPath${_lib_suffix}
		ENV DepsPath
		${mbedtlsPath${_lib_suffix}}
		${mbedtlsPath}
		${DepsPath${_lib_suffix}}
		${DepsPath}
		${_MBEDTLS_INCLUDE_DIRS}
	PATHS
		/usr/include /usr/local/include /opt/local/include /sw/include
	PATH_SUFFIXES
		include include/mbedtls)

find_library(MBEDTLS_LIB
	NAMES ${_MBEDTLS_LIBRARIES} mbedtls libmbedtls
	HINTS
		ENV mbedtlsPath${_lib_suffix}
		ENV mbedtlsPath
		ENV DepsPath${_lib_suffix}
		ENV DepsPath
		${mbedtlsPath${_lib_suffix}}
		${mbedtlsPath}
		${DepsPath${_lib_suffix}}
		${DepsPath}
		${_MBEDTLS_LIBRARY_DIRS}
	PATHS
		/usr/lib /usr/local/lib /opt/local/lib /sw/lib
	PATH_SUFFIXES
		lib${_lib_suffix} lib
		libs${_lib_suffix} libs
		bin${_lib_suffix} bin
		../lib${_lib_suffix} ../lib
		../libs${_lib_suffix} ../libs
		../bin${_lib_suffix} ../bin)

find_library(MBEDCRYPTO_LIB
	NAMES ${_MBEDCRYPTO_LIBRARIES} mbedcrypto libmbedcrypto
	HINTS
		ENV mbedcryptoPath${_lib_suffix}
		ENV mbedcryptoPath
		ENV DepsPath${_lib_suffix}
		ENV DepsPath
		${mbedcryptoPath${_lib_suffix}}
		${mbedcryptoPath}
		${DepsPath${_lib_suffix}}
		${DepsPath}
		${_MBEDCRYPTO_LIBRARY_DIRS}
	PATHS
		/usr/lib /usr/local/lib /opt/local/lib /sw/lib
	PATH_SUFFIXES
		lib${_lib_suffix} lib
		libs${_lib_suffix} libs
		bin${_lib_suffix} bin
		../lib${_lib_suffix} ../lib
		../libs${_lib_suffix} ../libs
		../bin${_lib_suffix} ../bin)

find_library(MBEDX509_LIB
	NAMES ${_MBEDX509_LIBRARIES} mbedx509 libmbedx509
	HINTS
		ENV mbedx509Path${_lib_suffix}
		ENV mbedx509Path
		ENV DepsPath${_lib_suffix}
		ENV DepsPath
		${mbedx509Path${_lib_suffix}}
		${mbedx509Path}
		${DepsPath${_lib_suffix}}
		${DepsPath}
		${_MBEDX509_LIBRARY_DIRS}
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
find_package_handle_standard_args(Libmbedtls DEFAULT_MSG MBEDTLS_LIB MBEDCRYPTO_LIB MBEDX509_LIB MBEDTLS_INCLUDE_DIR)
mark_as_advanced(MBEDTLS_INCLUDE_DIR MBEDTLS_LIB MBEDCRYPTO_LIB MBEDX509_LIB)

if(LIBMBEDTLS_FOUND)
	set(LIBMBEDTLS_INCLUDE_DIRS ${MBEDTLS_INCLUDE_DIR})
	set(LIBMBEDTLS_LIBRARIES ${MBEDTLS_LIB} ${MBEDCRYPTO_LIB} ${MBEDX509_LIB})
endif()
