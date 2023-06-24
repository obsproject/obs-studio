#[=======================================================================[.rst
FindLibsecret
-------------

FindModule for libsecret and the associated library

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` target ``Libsecret::Libsecret``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``Libsecret_FOUND``
  True, if the library was found.
``Libsecret_VERSION``
  Detected version of found Libsecret library.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Libsecret_INCLUDE_DIR``
  Directory containing ``secret.h``.

#]=======================================================================]

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_Libsecret QUIET libsecret-1)
endif()

# Libsecret_set_soname: Set SONAME on imported library target
macro(Libsecret_set_soname)
  if(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
    execute_process(
      COMMAND sh -c "${CMAKE_OBJDUMP} -p '${Libsecret_LIBRARY}' | grep SONAME"
      OUTPUT_VARIABLE _output
      RESULT_VARIABLE _result)

    if(_result EQUAL 0)
      string(REGEX REPLACE "[ \t]+SONAME[ \t]+([^ \t]+)" "\\1" _soname "${_output}")
      set_property(TARGET Libsecret::Libsecret PROPERTY IMPORTED_SONAME "${_soname}")
      unset(_soname)
    endif()
  endif()
  unset(_output)
  unset(_result)
endmacro()

find_path(
  Libsecret_INCLUDE_DIR
  NAMES libsecret/secret.h
  HINTS ${PC_Libsecret_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include
  PATH_SUFFIXES libsecret-1
  DOC "Libsecret include directory")

find_library(
  Libsecret_LIBRARY
  NAMES secret secret-1 libsecret libsecret-1
  HINTS ${PC_Libsecret_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "Libsecret location")

if(PC_Libsecret_VERSION VERSION_GREATER 0)
  set(Libsecret_VERSION ${PC_Libsecret_VERSION})
elseif(EXISTS "${Libsecret_INCLUDE_DIR}/secret-version.h")
  file(STRINGS "${Libsecret_INCLUDE_DIR}/secret-version.h" _version_string)
  string(REGEX REPLACE "SECRET_[A-Z]+_VERSION \\(([0-9]+)\\)" "\\1\\.\\2\\.\\3" Libsecret_VERSION "${_version_string}")
else()
  if(NOT Libsecret_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find Libsecret version.")
  endif()
  set(Libsecret_VERSION 0.0.0)
endif()

find_package_handle_standard_args(
  Libsecret
  REQUIRED_VARS Libsecret_INCLUDE_DIR Libsecret_LIBRARY
  VERSION_VAR Libsecret_VERSION REASON_FAILURE_MESSAGE "Ensure libsecret-1 is installed on the system.")
mark_as_advanced(Libsecret_INCLUDE_DIR Libsecret_LIBRARY)

if(Libsecret_FOUND)
  if(NOT TARGET Libsecret::Libsecret)
    if(IS_ABSOLUTE "${Libsecret_LIBRARY}")
      add_library(Libsecret::Libsecret UNKNOWN IMPORTED)
      set_property(TARGET Libsecret::Libsecret PROPERTY IMPORTED_LOCATION "${Libsecret_LIBRARY}")
    else()
      add_library(Libsecret::Libsecret INTERFACE IMPORTED)
      set_property(TARGET Libsecret::Libsecret PROPERTY IMPORTED_LIBNAME "${Libsecret_LIBRARY}")
    endif()

    libsecret_set_soname()
    set_target_properties(Libsecret::Libsecret PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${Libsecret_INCLUDE_DIR}")
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  Libsecret PROPERTIES
  URL "https://gnome.pages.gitlab.gnome.org/libsecret/index.html"
  DESCRIPTION "Secret Service D-Bus client library")
