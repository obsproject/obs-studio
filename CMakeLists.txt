cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(obs-studio VERSION ${OBS_VERSION_CANONICAL})

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/windows/architecture.cmake")
  if(NOT OBS_PARENT_ARCHITECTURE STREQUAL CMAKE_VS_PLATFORM_NAME)
    return()
  endif()
endif()

include(compilerconfig)
include(defaults)
include(helpers)

option(ENABLE_FRONTEND "Enable building with UI (requires Qt)" ON)
option(ENABLE_PLUGINS "Enable building OBS plugins" ON)
option(ENABLE_SCRIPTING "Enable scripting support" ON)
option(ENABLE_HEVC "Enable HEVC encoders" ON)

set(CRYPTO_BACKEND
    "mbedtls"
    CACHE STRING "Crypto backend for OBS (mbedtls or openssl)"
)
set_property(CACHE CRYPTO_BACKEND PROPERTY STRINGS mbedtls openssl)

string(TOLOWER "${CRYPTO_BACKEND}" CRYPTO_BACKEND_U)

if(ENABLE_PLUGINS OR (OS_WINDOWS AND ENABLE_FRONTEND))
  if(CRYPTO_BACKEND_U STREQUAL "mbedtls")
    set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)
    find_package(MbedTLS REQUIRED)
    set(CMAKE_FIND_PACKAGE_PREFER_CONFIG FALSE)
  elseif(CRYPTO_BACKEND_U STREQUAL "openssl")
    if(NOT OS_LINUX)
      message(FATAL_ERROR "OpenSSL is only supported as a crypto backend on Linux.")
    endif()
    find_package(OpenSSL 1.1.0 REQUIRED)
  else()
    message(FATAL_ERROR "Unsupported CRYPTO_BACKEND value: ${CRYPTO_BACKEND}. Expected 'mbedtls' or 'openssl'.")
  endif()
endif()

add_subdirectory(libobs)
if(OS_WINDOWS)
  add_subdirectory(libobs-d3d11)
  add_subdirectory(libobs-winrt)
endif()
add_subdirectory(libobs-opengl)
if(OS_MACOS)
  add_subdirectory(libobs-metal)
endif()
add_subdirectory(plugins)

add_subdirectory(test/test-input)

add_subdirectory(frontend)

message_configuration()
