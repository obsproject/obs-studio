#[=======================================================================[.rst
FindLibrist
----------

FindModule for Librist and associated libraries

.. versionchanged:: 3.0
  Updated FindModule to CMake standards

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` target ``Librist::Librist``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``Librist_FOUND``
  True, if all required components and the core library were found.
``Librist_VERSION``
  Detected version of found Librist libraries.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Librist_LIBRARY``
  Path to the library component of Librist.
``Librist_INCLUDE_DIR``
  Directory containing ``librist.h``.

#]=======================================================================]

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-format: on

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_Librist QUIET librist)
endif()

# Librist_set_soname: Set SONAME on imported library target
macro(Librist_set_soname)
  if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
    execute_process(
      COMMAND sh -c "otool -D '${Librist_LIBRARY}' | grep -v '${Librist_LIBRARY}'"
      OUTPUT_VARIABLE _output
      RESULT_VARIABLE _result)

    if(_result EQUAL 0 AND _output MATCHES "^@rpath/")
      set_property(TARGET Librist::Librist PROPERTY IMPORTED_SONAME "${_output}")
    endif()
  elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
    execute_process(
      COMMAND sh -c "objdump -p '${Librist_LIBRARY}' | grep SONAME"
      OUTPUT_VARIABLE _output
      RESULT_VARIABLE _result)

    if(_result EQUAL 0)
      string(REGEX REPLACE "[ \t]+SONAME[ \t]+([^ \t]+)" "\\1" _soname "${_output}")
      set_property(TARGET Librist::Librist PROPERTY IMPORTED_SONAME "${_soname}")
      unset(_soname)
    endif()
  endif()
  unset(_output)
  unset(_result)
endmacro()

find_path(
  Librist_INCLUDE_DIR
  NAMES librist.h librist/librist.h
  HINTS ${PC_Librist_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include
  DOC "Librist include directory")

if(PC_Librist_VERSION VERSION_GREATER 0)
  set(Librist_VERSION ${PC_Librist_VERSION})
elseif(EXISTS "${Librist_INCLUDE_DIR}/version.h")
  file(STRINGS "${_VERSION_FILE}" _VERSION_STRING REGEX "^.*VERSION_(MAJOR|MINOR|PATCH)[ \t]+[0-9]+[ \t]*$")
  string(REGEX REPLACE ".*VERSION_MAJOR[ \t]+([0-9]+).*" "\\1" _VERSION_MAJOR "${_VERSION_STRING}")
  string(REGEX REPLACE ".*VERSION_MINOR[ \t]+([0-9]+).*" "\\1" _VERSION_MINOR "${_VERSION_STRING}")
  string(REGEX REPLACE ".*VERSION_PATCH[ \t]+([0-9]+).*" "\\1" _VERSION_PATCH "${_VERSION_STRING}")
  set(Librist_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_MICRO}")
else()
  if(NOT Librist_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find Librist version.")
  endif()
  set(Librist_VERSION 0.0.0)
endif()

find_library(
  Librist_LIBRARY
  NAMES librist rist
  HINTS ${PC_Librist_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "Librist location")

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin|Windows")
  set(Librist_ERROR_REASON "Ensure that obs-deps is provided as part of CMAKE_PREFIX_PATH.")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
  set(Librist_ERROR_REASON "Ensure librist libraries are available in local library paths.")
endif()

find_package_handle_standard_args(
  Librist
  REQUIRED_VARS Librist_LIBRARY Librist_INCLUDE_DIR
  VERSION_VAR Librist_VERSION REASON_FAILURE_MESSAGE "${Librist_ERROR_REASON}")
mark_as_advanced(Librist_INCLUDE_DIR Librist_LIBRARY)
unset(Librist_ERROR_REASON)

if(Librist_FOUND)
  if(NOT TARGET Librist::Librist)
    if(IS_ABSOLUTE "${Librist_LIBRARY}")
      add_library(Librist::Librist UNKNOWN IMPORTED)
      set_property(TARGET Librist::Librist PROPERTY IMPORTED_LOCATION "${Librist_LIBRARY}")
    else()
      add_library(Librist::Librist INTERFACE IMPORTED)
      set_property(TARGET Librist::Librist PROPERTY IMPORTED_LIBNAME "${Librist_LIBRARY}")
    endif()

    librist_set_soname()
    set_target_properties(
      Librist::Librist
      PROPERTIES INTERFACE_COMPILE_OPTIONS "${PC_Librist_CFLAGS_OTHER}"
                 INTERFACE_INCLUDE_DIRECTORIES "${Librist_INCLUDE_DIR}"
                 VERSION ${Librist_VERSION})
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  Librist PROPERTIES
  URL "https://code.videolan.org/rist/librist"
  DESCRIPTION "A library that can be used to easily add the RIST protocol to your application.")
