#[=======================================================================[.rst
FindLibsrt
----------

FindModule for Libsrt and associated libraries

.. versionchanged:: 3.0
  Updated FindModule to CMake standards

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` target ``Libsrt::Libsrt``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``Libsrt_FOUND``
  True, if all required components and the core library were found.
``Libsrt_VERSION``
  Detected version of found Libsrt libraries.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Libsrt_LIBRARY``
  Path to the library component of Libsrt.
``Libsrt_INCLUDE_DIR``
  Directory containing ``srt.h``.

#]=======================================================================]

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_Libsrt QUIET libsrt)
endif()

# libsrt_set_soname: Set SONAME on imported library target
macro(libsrt_set_soname)
  if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
    execute_process(
      COMMAND sh -c "otool -D '${Libsrt_LIBRARY}' | grep -v '${Libsrt_LIBRARY}'"
      OUTPUT_VARIABLE _output
      RESULT_VARIABLE _result
    )

    if(_result EQUAL 0 AND _output MATCHES "^@rpath/")
      set_property(TARGET Libsrt::Libsrt PROPERTY IMPORTED_SONAME "${_output}")
    endif()
  elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
    execute_process(
      COMMAND sh -c "objdump -p '${Libsrt_LIBRARY}' | grep SONAME"
      OUTPUT_VARIABLE _output
      RESULT_VARIABLE _result
    )

    if(_result EQUAL 0)
      string(REGEX REPLACE "[ \t]+SONAME[ \t]+([^ \t]+)" "\\1" _soname "${_output}")
      set_property(TARGET Libsrt::Libsrt PROPERTY IMPORTED_SONAME "${_soname}")
      unset(_soname)
    endif()
  endif()
  unset(_output)
  unset(_result)
endmacro()

find_path(
  Libsrt_INCLUDE_DIR
  NAMES srt.h srt/srt.h
  HINTS ${PC_Libsrt_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include
  DOC "Libsrt include directory"
)

if(PC_Libsrt_VERSION VERSION_GREATER 0)
  set(Libsrt_VERSION ${PC_Libsrt_VERSION})
elseif(EXISTS "${Libsrt_INCLUDE_DIR}/version.h")
  file(STRINGS "${Libsrt_INCLUDE_DIR}/version.h" _VERSION_STRING REGEX "#define[ \t]+SRT_VERSION_STRING[ \t]+.+")
  string(REGEX REPLACE ".*#define[ \t]+SRT_VERSION_STRING[ \t]+\"(.+)\".*" "\\1" Libsrt_VERSION "${_VERSION_STRING}")
else()
  if(NOT Libsrt_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find Libsrt version.")
  endif()
  set(Libsrt_VERSION 0.0.0)
endif()

find_library(
  Libsrt_LIBRARY
  NAMES srt libsrt
  HINTS ${PC_Libsrt_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "Libsrt location"
)

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin|Windows")
  set(Libsrt_ERROR_REASON "Ensure that obs-deps is provided as part of CMAKE_PREFIX_PATH.")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
  set(Libsrt_ERROR_REASON "Ensure libsrt libraries are available in local library paths.")
endif()

find_package_handle_standard_args(
  Libsrt
  REQUIRED_VARS Libsrt_LIBRARY Libsrt_INCLUDE_DIR
  VERSION_VAR Libsrt_VERSION
  REASON_FAILURE_MESSAGE "${Libsrt_ERROR_REASON}"
)
mark_as_advanced(Libsrt_INCLUDE_DIR Libsrt_LIBRARY)
unset(Libsrt_ERROR_REASON)

if(Libsrt_FOUND)
  if(NOT TARGET Libsrt::Libsrt)
    if(IS_ABSOLUTE "${Libsrt_LIBRARY}")
      add_library(Libsrt::Libsrt UNKNOWN IMPORTED)
      libsrt_set_soname()
      set_property(TARGET Libsrt::Libsrt PROPERTY IMPORTED_LOCATION "${Libsrt_LIBRARY}")
    else()
      add_library(Libsrt::Libsrt INTERFACE IMPORTED)
      set_property(TARGET Libsrt::Libsrt PROPERTY IMPORTED_LIBNAME "${Libsrt_LIBRARY}")
    endif()

    set_target_properties(
      Libsrt::Libsrt
      PROPERTIES
        INTERFACE_COMPILE_OPTIONS "${PC_Libsrt_CFLAGS_OTHER}"
        INTERFACE_INCLUDE_DIRECTORIES "${Libsrt_INCLUDE_DIR}"
    )
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  Libsrt
  PROPERTIES
    URL "https://www.srtalliance.org"
    DESCRIPTION
      "Secure Reliable Transport (SRT) is a transport protocol for ultra low (sub-second) latency live video and audio streaming, as well as for generic bulk data transfer."
)
