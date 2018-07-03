# Once done these will be defined:
#
#  MBEDTLS_LIB
#  MBEDTLS_INCLUDE_DIRS
#  MBEDTLS_LIBRARIES

find_path(MBEDTLS_INCLUDE_DIR
	NAMES mbedtls/ssl.h
	HINTS
		${_MBEDTLS_INCLUDE_DIRS}
	PATHS
		/usr/include /usr/local/include /opt/local/include)

find_library(MBEDTLS_LIB
	NAMES libmbedtls.a
	HINTS
		${_MBEDTLS_LIBRARY_DIRS}
	PATHS
		/usr/lib /usr/local/lib /opt/local/lib)

find_library(MBEDCRYPTO_LIB
	NAMES libmbedcrypto.a
	HINTS
		${_MBEDTLS_LIBRARY_DIRS}
	PATHS
		/usr/lib /usr/local/lib /opt/local/lib)

find_library(MBEDX509_LIB
	NAMES libmbedx509.a
	HINTS
		${_MBEDTLS_LIBRARY_DIRS}
	PATHS
		/usr/lib /usr/local/lib /opt/local/lib)

mark_as_advanced(MBEDTLS_INCLUDE_DIR MBEDTLS_ARCH_INCLUDE_DIR MBEDTLS_LIB MBEDCRYPTO_LIB MBEDX509_LIB)

if(MBEDTLS_LIB AND MBEDCRYPTO_LIB AND MBEDX509_LIB)
	set(MBEDTLS_INCLUDE_DIRS ${MBEDTLS_INCLUDE_DIR} ${MBEDTLS_ARCH_INCLUDE_DIR})
	set(MBEDTLS_LIBRARIES ${MBEDTLS_LIB} ${MBEDCRYPTO_LIB} ${MBEDX509_LIB})
	set(MBEDTLS_FOUND 1)
else()
  message (WARNING "Some mbed libs not found. TLS: ${MBEDTLS_LIB}, CRYPTO: ${MBEDCRYPTO_LIB}, x509: ${MBEDX509_LIB}")
	set(MBEDTLS_FOUND 0)
endif()
