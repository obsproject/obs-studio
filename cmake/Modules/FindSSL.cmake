# Once done these will be defined:
#
#  SSL_FOUND
#  SSL_INCLUDE_DIRS
#  SSL_LIBRARIES
#
# For use in OBS: 
#
#  SSL_INCLUDE_DIR

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(_CRYPTO QUIET libcrypto)
	pkg_check_modules(_SSL QUIET libssl)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(_lib_suffix 64)
else()
	set(_lib_suffix 32)
endif()

set(_SSL_BASE_HINTS
		ENV sslPath${_lib_suffix}
		ENV sslPath
		ENV DepsPath${_lib_suffix}
		ENV DepsPath
		${sslPath${_lib_suffix}}
		${sslPath}
		${DepsPath${_lib_suffix}}
		${DepsPath})

set(_SSL_LIB_SUFFIXES
		lib${_lib_suffix} lib
		libs${_lib_suffix} libs
		bin${_lib_suffix} bin
		../lib${_lib_suffix} ../lib
		../libs${_lib_suffix} ../libs
		../bin${_lib_suffix} ../bin)

find_path(SSL_INCLUDE_DIR
	NAMES openssl/ssl.h
	HINTS
		${_SSL_BASE_HINTS}
		${_CRYPTO_INCLUDE_DIRS}
		${_SSL_INCLUDE_DIRS}
	PATHS
		/usr/include /usr/local/include /opt/local/include /sw/include
	PATH_SUFFIXES
		include)

find_library(_SSL_LIB
	NAMES ${_SSL_LIBRARIES} ssleay32 ssl
	HINTS
		${_SSL_BASE_HINTS}
		${_SSL_LIBRARY_DIRS}
	PATHS
		/usr/lib /usr/local/lib /opt/local/lib /sw/lib
	PATH_SUFFIXES ${_SSL_LIB_SUFFIXES})

find_library(_CRYPTO_LIB
	NAMES ${_CRYPTO_LIBRARIES} libeay32 crypto
	HINTS
		${_SSL_BASE_HINTS}
		${_CRYPTO_LIBRARY_DIRS}
	PATHS
		/usr/lib /usr/local/lib /opt/local/lib /sw/lib
	PATH_SUFFIXES ${_SSL_LIB_SUFFIXES})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ssl DEFAULT_MSG _SSL_LIB _CRYPTO_LIB SSL_INCLUDE_DIR)
mark_as_advanced(SSL_INCLUDE_DIR _SSL_LIB _CRYPTO_LIB)

if(SSL_FOUND)
	set(SSL_INCLUDE_DIRS ${SSL_INCLUDE_DIR})
	set(SSL_LIBRARIES ${_SSL_LIB} ${_CRYPTO_LIB})
endif()
