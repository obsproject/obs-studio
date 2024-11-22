#[=======================================================================[.rst
Findjansson
----------

FindModule for jansson and associated libraries

.. versionchanged:: 3.0
  Updated FindModule to CMake standards

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` target ``jansson::jansson``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``jansson_FOUND``
  True, if all required components and the core library were found.
``jansson_VERSION``
  Detected version of found jansson libraries.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``jansson_LIBRARY``
  Path to the library component of jansson.
``jansson_INCLUDE_DIR``
  Directory containing ``jansson.h``.

#]=======================================================================]

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_jansson QUIET jansson)
endif()

# jansson_set_soname: Set SONAME on imported library targets
macro(jansson_set_soname)
  if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
    execute_process(
      COMMAND sh -c "otool -D '${jansson_LIBRARY}' | grep -v '${jansson_LIBRARY}'"
      OUTPUT_VARIABLE _output
      RESULT_VARIABLE _result
    )

    if(_result EQUAL 0 AND _output MATCHES "^@rpath/")
      set_property(TARGET jansson::jansson PROPERTY IMPORTED_SONAME "${_output}")
    endif()
  elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
    execute_process(
      COMMAND sh -c "objdump -p '${jansson_LIBRARY}' | grep SONAME"
      OUTPUT_VARIABLE _output
      RESULT_VARIABLE _result
    )

    if(_result EQUAL 0)
      string(REGEX REPLACE "[ \t]+SONAME[ \t]+([^ \t]+)" "\\1" _soname "${_output}")
      set_property(TARGET jansson::jansson PROPERTY IMPORTED_SONAME "${_soname}")
      unset(_soname)
    endif()
  endif()
  unset(_output)
  unset(_result)
endmacro()

find_path(
  jansson_INCLUDE_DIR
  NAMES jansson.h
  HINTS ${PC_jansson_INCLUDE_DIR}
  PATHS /usr/include /usr/local/include
  PATH_SUFFIXES jansson-2.1
  DOC "jansson include directory"
)

if(PC_jansson_VERSION VERSION_GREATER 0)
  set(jansson_VERSION ${PC_jansson_VERSION})
elseif(EXISTS "${jansson_INCLUDE_DIR}/jansson.h")
  file(STRINGS "${jansson_INCLUDE_DIR}/jansson.h" _VERSION_STRING REGEX "#define[ \t]+JANSSON_VERSION[ \t]+.+")
  string(REGEX REPLACE ".*#define[ \t]+JANSSON_VERSION[ \t]+\"(.+)\".*" "\\1" jansson_VERSION "${_VERSION_STRING}")
else()
  if(NOT jansson_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find jansson version.")
  endif()
  set(jansson_VERSION 0.0.0)
endif()

find_library(
  jansson_LIBRARY
  NAMES jansson
  HINTS ${PC_jansson_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "jansson location"
)

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin|Windows")
  set(jansson_ERROR_REASON "Ensure that obs-deps is provided as part of CMAKE_PREFIX_PATH.")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
  set(jansson_ERROR_REASON "Ensure that jansson is installed on the system.")
endif()

find_package_handle_standard_args(
  jansson
  REQUIRED_VARS jansson_LIBRARY jansson_INCLUDE_DIR
  VERSION_VAR jansson_VERSION
  REASON_FAILURE_MESSAGE "${jansson_ERROR_REASON}"
)
mark_as_advanced(jansson_INCLUDE_DIR jansson_LIBRARY)
unset(jansson_ERROR_REASON)

if(jansson_FOUND)
  if(NOT TARGET jansson::jansson)
    if(IS_ABSOLUTE "${jansson_LIBRARY}")
      add_library(jansson::jansson UNKNOWN IMPORTED)
      set_property(TARGET jansson::jansson PROPERTY IMPORTED_LOCATION "${jansson_LIBRARY}")
    else()
      add_library(jansson::jansson INTERFACE IMPORTED)
      set_property(TARGET jansson::jansson PROPERTY IMPORTED_LIBNAME "${jansson_LIBRARY}")
    endif()

    jansson_set_soname()
    set_target_properties(
      jansson::jansson
      PROPERTIES
        INTERFACE_COMPILE_OPTIONS "${PC_jansson_CFLAGS_OTHER}"
        INTERFACE_INCLUDE_DIRECTORIES "${jansson_INCLUDE_DIR}"
        VERSION ${jansson_VERSION}
    )
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  jansson
  PROPERTIES
    URL "https://www.digip.org/jansson/"
    DESCRIPTION "A C library for encoding, decoding, and manipulating JSON data."
)
