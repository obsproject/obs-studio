#[=======================================================================[.rst
FindMbedTLS
-----------

FindModule for MbedTLS and associated libraries

.. versionchanged:: 3.0
  Updated FindModule to CMake standards

Components
^^^^^^^^^^

.. versionadded:: 1.0

This module contains provides several components:

``MbedCrypto``
``MbedTLS``
``MbedX509``

Import targets exist for each component.

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` targets:

``MbedTLS::MbedCrypto``
  Crypto component

``MbedTLS::MbedTLS``
  TLS component

``MbedTLS::MbedX509``
  X509 component

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``MbedTLS_FOUND``
  True, if all required components and the core library were found.
``MbedTLS_VERSION``
  Detected version of found MbedTLS libraries.

``MbedTLS_<COMPONENT>_VERSION``
  Detected version of found MbedTLS component library.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``MbedTLS_<COMPONENT>_LIBRARY``
  Path to the library component of MbedTLS.
``MbedTLS_<COMPONENT>_INCLUDE_DIR``
  Directory containing ``<COMPONENT>.h``.

#]=======================================================================]

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_MbedTLS QUIET mbedtls mbedcrypto mbedx509)
endif()

# MbedTLS_set_soname: Set SONAME on imported library targets
macro(MbedTLS_set_soname component)
  if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
    execute_process(
      COMMAND sh -c "otool -D '${Mbed${component}_LIBRARY}' | grep -v '${Mbed${component}_LIBRARY}'"
      OUTPUT_VARIABLE _output
      RESULT_VARIABLE _result
    )

    if(_result EQUAL 0 AND _output MATCHES "^@rpath/")
      set_property(TARGET MbedTLS::Mbed${component} PROPERTY IMPORTED_SONAME "${_output}")
    endif()
  elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
    execute_process(
      COMMAND sh -c "objdump -p '${Mbed${component}_LIBRARY}' | grep SONAME"
      OUTPUT_VARIABLE _output
      RESULT_VARIABLE _result
    )

    if(_result EQUAL 0)
      string(REGEX REPLACE "[ \t]+SONAME[ \t]+([^ \t]+)" "\\1" _soname "${_output}")
      set_property(TARGET MbedTLS::Mbed${component} PROPERTY IMPORTED_SONAME "${_soname}")
      unset(_soname)
    endif()
  endif()
  unset(_output)
  unset(_result)
endmacro()

find_path(
  MbedTLS_INCLUDE_DIR
  NAMES mbedtls/ssl.h
  HINTS "${PC_MbedTLS_INCLUDE_DIRS}"
  PATHS /usr/include /usr/local/include
  DOC "MbedTLS include directory"
)

if(PC_MbedTLS_VERSION VERSION_GREATER 0)
  set(MbedTLS_VERSION ${PC_MbedTLS_VERSION})
elseif(EXISTS "${MbedTLS_INCLUDE_DIR}/mbedtls/build_info.h")
  file(
    STRINGS
    "${MbedTLS_INCLUDE_DIR}/mbedtls/build_info.h"
    _VERSION_STRING
    REGEX "#define[ \t]+MBEDTLS_VERSION_STRING[ \t]+.+"
  )
  string(
    REGEX REPLACE
    ".*#define[ \t]+MBEDTLS_VERSION_STRING[ \t]+\"(.+)\".*"
    "\\1"
    MbedTLS_VERSION
    "${_VERSION_STRING}"
  )
elseif(EXISTS "${MbedTLS_INCLUDE_DIR}/mbedtls/version.h")
  file(
    STRINGS
    "${MbedTLS_INCLUDE_DIR}/mbedtls/version.h"
    _VERSION_STRING
    REGEX "#define[ \t]+MBEDTLS_VERSION_STRING[ \t]+.+"
  )
  string(
    REGEX REPLACE
    ".*#define[ \t]+MBEDTLS_VERSION_STRING[ \t]+\"(.+)\".*"
    "\\1"
    MbedTLS_VERSION
    "${_VERSION_STRING}"
  )
else()
  if(NOT MbedTLS_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find MbedTLS version.")
  endif()
  set(MbedTLS_VERSION 0.0.0)
endif()

find_library(
  MbedTLS_LIBRARY
  NAMES libmbedtls mbedtls
  HINTS "${PC_MbedTLS_LIBRARY_DIRS}"
  PATHS /usr/lib /usr/local/lib
  DOC "MbedTLS location"
)

find_library(
  MbedCrypto_LIBRARY
  NAMES libmbedcrypto mbedcrypto
  HINTS "${PC_MbedTLS_LIBRARY_DIRS}"
  PATHS /usr/lib /usr/local/lib
  DOC "MbedCrypto location"
)

find_library(
  MbedX509_LIBRARY
  NAMES libmbedx509 mbedx509
  HINTS "${PC_MbedTLS_LIBRARY_DIRS}"
  PATHS /usr/lib /usr/local/lib
  DOC "MbedX509 location"
)

if(MbedTLS_LIBRARY AND NOT MbedCrypto_LIBRARY AND NOT MbedX509_LIBRARY)
  set(CMAKE_REQUIRED_LIBRARIES "${MbedTLS_LIBRARY}")
  set(CMAKE_REQUIRED_INCLUDES "${MbedTLS_INCLUDE_DIR}")

  check_symbol_exists(mbedtls_x509_crt_init "mbedtls/x590_crt.h" MbedTLS_INCLUDES_X509)
  check_symbol_exists(mbedtls_sha256_init "mbedtls/sha256.h" MbedTLS_INCLUDES_CRYPTO)
  unset(CMAKE_REQUIRED_LIBRARIES)
  unset(CMAKE_REQUIRED_INCLUDES)
endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin|Windows")
  set(MbedTLS_ERROR_REASON "Ensure that obs-deps is provided as part of CMAKE_PREFIX_PATH.")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
  set(MbedTLS_ERROR_REASON "Ensure that MbedTLS is installed on the system.")
endif()

if(MbedTLS_INCLUDES_X509 AND MbedTLS_INCLUDES_CRYPTO)
  find_package_handle_standard_args(
    MbedTLS
    REQUIRED_VARS MbedTLS_LIBRARY MbedTLS_INCLUDE_DIR
    VERSION_VAR MbedTLS_VERSION
    REASON_FAILURE_MESSAGE "${MbedTLS_ERROR_REASON}"
  )
  mark_as_advanced(MbedTLS_LIBRARY MbedTLS_INCLUDE_DIR)
  list(APPEND _COMPONENTS TLS)
else()
  find_package_handle_standard_args(
    MbedTLS
    REQUIRED_VARS MbedTLS_LIBRARY MbedCrypto_LIBRARY MbedX509_LIBRARY MbedTLS_INCLUDE_DIR
    VERSION_VAR MbedTLS_VERSION
    REASON_FAILURE_MESSAGE "${MbedTLS_ERROR_REASON}"
  )
  mark_as_advanced(MbedTLS_LIBRARY MbedCrypto_LIBRARY MbedX509_LIBRARY MbedTLS_INCLUDE_DIR)
  list(APPEND _COMPONENTS TLS Crypto X509)
endif()
unset(MbedTLS_ERROR_REASON)

if(MbedTLS_FOUND)
  foreach(component IN LISTS _COMPONENTS)
    if(NOT TARGET MbedTLS::Mbed${component})
      if(IS_ABSOLUTE "${Mbed${component}_LIBRARY}")
        add_library(MbedTLS::Mbed${component} UNKNOWN IMPORTED)
        set_property(TARGET MbedTLS::Mbed${component} PROPERTY IMPORTED_LOCATION "${Mbed${component}_LIBRARY}")
      else()
        add_library(MbedTLS::Mbed${component} INTERFACE IMPORTED)
        set_property(TARGET MbedTLS::Mbed${component} PROPERTY IMPORTED_LIBNAME "${Mbed${component}_LIBRARY}")
      endif()

      mbedtls_set_soname(${component})
      set_target_properties(
        MbedTLS::MbedTLS
        PROPERTIES
          INTERFACE_COMPILE_OPTIONS "${PC_MbedTLS_CFLAGS_OTHER}"
          INTERFACE_INCLUDE_DIRECTORIES "${MbedTLS_INCLUDE_DIR}"
          INTERFACE_LINK_OPTIONS "$<$<AND:$<PLATFORM_ID:Windows>,$<CONFIG:DEBUG>>:/NODEFAULTLIB:MSVCRT>"
          VERSION ${MbedTLS_VERSION}
      )
    endif()
  endforeach()

  if(MbedTLS_INCLUDES_X509 AND MbedTLS_INCLUDES_CRYPTO)
    set(MbedTLS_LIBRARIES ${MbedTLS_LIBRARY})
  else()
    set(MbedTLS_LIBRARIES ${MbedTLS_LIBRARY} ${MbedCrypto_LIBRARY} ${MbedX509_LIBRARY})
    set_property(TARGET MbedTLS::MbedTLS PROPERTY INTERFACE_LINK_LIBRARIES MbedTLS::MbedCrypto MbedTLS::MbedX509)
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  MbedTLS
  PROPERTIES
    URL "https://www.trustedfirmware.org/projects/mbed-tls"
    DESCRIPTION
      "A C library implementing cryptographic primitives, X.509 certificate manipulation, and the SSL/TLS and DTLS protocols."
)
