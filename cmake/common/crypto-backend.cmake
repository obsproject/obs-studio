include_guard(GLOBAL)

set(CRYPTO_BACKEND "mbedtls" CACHE STRING
    "Crypto backend for OBS (mbedtls or gnutls)")

set_property(CACHE CRYPTO_BACKEND PROPERTY STRINGS mbedtls gnutls)

string(TOLOWER "${CRYPTO_BACKEND}" CRYPTO_BACKEND_U)

if(CRYPTO_BACKEND_U STREQUAL "mbedtls")
  set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)
  find_package(MbedTLS 3...<4 REQUIRED)
  set(CMAKE_FIND_PACKAGE_PREFER_CONFIG FALSE)

  set(crypto_helpers_source "${PROJECT_SOURCE_DIR}/frontend/utility/crypto-helpers-mbedtls.cpp")
  set(crypto_helpers_library MbedTLS::mbedtls)
  set(crypto_rtmp_definition USE_MBEDTLS)

elseif(CRYPTO_BACKEND_U STREQUAL "gnutls")
  if(NOT OS_LINUX)
    message(FATAL_ERROR
      "GnuTLS is only supported as a crypto backend on Linux.")
  endif()

  find_package(GnuTLS REQUIRED)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(Nettle REQUIRED IMPORTED_TARGET nettle)

  set(crypto_helpers_source "${PROJECT_SOURCE_DIR}/frontend/utility/crypto-helpers-gnutls.cpp")
  set(crypto_helpers_library GnuTLS::GnuTLS PkgConfig::Nettle)
  set(crypto_rtmp_definition USE_GNUTLS)

else()
  message(FATAL_ERROR
    "Unknown crypto backend: ${CRYPTO_BACKEND}")
endif()

add_library(obs-crypto INTERFACE)
add_library(OBS::crypto ALIAS obs-crypto)

target_link_libraries(obs-crypto INTERFACE
  ${crypto_helpers_library}
)

set_property(TARGET obs-crypto PROPERTY
  OBS_CRYPTO_HELPERS_SOURCE "${crypto_helpers_source}"
)

set_property(TARGET obs-crypto PROPERTY
  OBS_CRYPTO_RTMP_DEFINITION "${crypto_rtmp_definition}"
)
