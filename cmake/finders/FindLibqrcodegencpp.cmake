#[=======================================================================[.rst
FindLibqrcodegencpp
-------------------

FindModule for Libqrcodegencpp and associated libraries

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 3.0

This module defines the :prop_tgt:`IMPORTED` target ``Libqrcodegencpp::Libqrcodegencpp``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``Libqrcodegencpp_FOUND``
  True, if all required components and the core library were found.
``Libqrcodegencpp_VERSION``
  Detected version of found Libqrcodegencpp libraries.
``Libqrcodegencpp_LIBRARIES``
  Libraries needed to link to Libqrcodegencpp.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Libqrcodegencpp_LIBRARY_RELEASE``
  Path to the library component of Libqrcodegencpp in non-debug configuration.
``Libqrcodegencpp_LIBRARY_DEBUG``
  Optional path to the library component of Libqrcodegencpp in debug configuration.
``Libqrcodegencpp_INCLUDE_DIR``
  Directory containing ``qrcodegen.hpp``.

#]=======================================================================]

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-lint: disable=C0301
# cmake-format: on

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_Libqrcodegencpp QUIET qrcodegencpp)
endif()

# Libqrcodegencpp_set_soname: Set SONAME on imported library target
macro(Libqrcodegencpp_set_soname)
  if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
    execute_process(
      COMMAND sh -c "otool -D '${Libqrcodegencpp_LIBRARY}' | grep -v '${Libqrcodegencpp_LIBRARY}'"
      OUTPUT_VARIABLE _output
      RESULT_VARIABLE _result)

    if(_result EQUAL 0 AND _output MATCHES "^@rpath/")
      set_property(TARGET Libqrcodegencpp::Libqrcodegencpp PROPERTY IMPORTED_SONAME "${_output}")
    endif()
  elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
    execute_process(
      COMMAND sh -c "${CMAKE_OBJDUMP} -p '${Libqrcodegencpp_LIBRARY}' | grep SONAME"
      OUTPUT_VARIABLE _output
      RESULT_VARIABLE _result)

    if(_result EQUAL 0)
      string(REGEX REPLACE "[ \t]+SONAME[ \t]+([^ \t]+)" "\\1" _soname "${_output}")
      set_property(TARGET Libqrcodegencpp::Libqrcodegencpp PROPERTY IMPORTED_SONAME "${_soname}")
      unset(_soname)
    endif()
  endif()
  unset(_output)
  unset(_result)
endmacro()

find_path(
  Libqrcodegencpp_INCLUDE_DIR
  NAMES qrcodegencpp
  HINTS ${PC_Libqrcodegencpp_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include
  DOC "Libqrcodegencpp include directory")

find_library(
  Libqrcodegencpp_LIBRARY_RELEASE
  NAMES qrcodegencpp libqrcodegencpp
  HINTS ${PC_Libqrcodegencpp_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "Libqrcodegencpp location")

find_library(
  Libqrcodegencpp_LIBRARY_DEBUG
  NAMES qrcodegencppd libqrcodegencppd
  HINTS ${PC_Libqrcodegencpp_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "Libqrcodegencpp debug location.")

if(PC_Libqrcodegencpp_VERSION VERSION_GREATER 0)
  set(Libqrcodegencpp_VERSION ${PC_Libqrcodegencpp_VERSION})
else()
  if(NOT Libqrcodegencpp_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find Libqrcodegencpp version.")
  endif()
  set(Libqrcodegencpp_VERSION 0.0.0)
endif()

include(SelectLibraryConfigurations)
select_library_configurations(Libqrcodegencpp)

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin|Windows")
  set(Libqrcodegencpp_ERROR_REASON "Ensure that a qrcodegencpp distribution is provided as part of CMAKE_PREFIX_PATH.")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
  set(Libqrcodegencpp_ERROR_REASON "Ensure that qrcodegencpp is installed on the system.")
endif()

find_package_handle_standard_args(
  Libqrcodegencpp
  REQUIRED_VARS Libqrcodegencpp_LIBRARY Libqrcodegencpp_INCLUDE_DIR
  VERSION_VAR Libqrcodegencpp_VERSION REASON_FAILURE_MESSAGE "${Libqrcodegencpp_ERROR_REASON}")
mark_as_advanced(Libqrcodegencpp_INCLUDE_DIR Libqrcodegencpp_LIBRARY)
unset(Libqrcodegencpp_ERROR_REASON)

if(Libqrcodegencpp_FOUND)
  list(APPEND Libqrcodegencpp_INCLUDE_DIRS ${Libqrcodegencpp_INCLUDE_DIR}/qrcodegencpp)
  set(Libqrcodegencpp_LIBRARIES ${Libqrcodegencpp_LIBRARY})
  mark_as_advanced(Libqrcodegencpp_INCLUDE_DIR Libqrcodegencpp_LIBRARY)

  if(NOT TARGET Libqrcodegencpp::Libqrcodegencpp)
    if(IS_ABSOLUTE "${Libqrcodegencpp_LIBRARY_RELEASE}")
      add_library(Libqrcodegencpp::Libqrcodegencpp STATIC IMPORTED)
      set_property(TARGET Libqrcodegencpp::Libqrcodegencpp PROPERTY IMPORTED_LOCATION
                                                                    "${Libqrcodegencpp_LIBRARY_RELEASE}")
    else()
      add_library(Libqrcodegencpp::Libqrcodegencpp INTERFACE IMPORTED)
      set_property(TARGET Libqrcodegencpp::Libqrcodegencpp PROPERTY IMPORTED_LIBNAME
                                                                    "${Libqrcodegencpp_LIBRARY_RELEASE}")
    endif()

    libqrcodegencpp_set_soname()
    set_target_properties(
      Libqrcodegencpp::Libqrcodegencpp
      PROPERTIES INTERFACE_COMPILE_OPTIONS "${PC_Libqrcodegencpp_CFLAGS_OTHER}"
                 INTERFACE_INCLUDE_DIRECTORIES "${Libqrcodegencpp_INCLUDE_DIR}"
                 VERSION ${Libqrcodegencpp_VERSION})

    if(Libqrcodegencpp_LIBRARY_DEBUG)
      if(IS_ABSOLUTE "${Libqrcodegencpp_LIBRARY_DEBUG}")
        set_property(TARGET Libqrcodegencpp::Libqrcodegencpp PROPERTY IMPORTED_LOCATION_DEBUG
                                                                      "${Libqrcodegencpp_LIBRARY_DEBUG}")
      else()
        set_property(TARGET Libqrcodegencpp::Libqrcodegencpp PROPERTY IMPORTED_LIBNAME_DEBUG
                                                                      "${Libqrcodegencpp_LIBRARY_DEBUG}")
      endif()
      set_property(
        TARGET Libqrcodegencpp::Libqrcodegencpp
        APPEND
        PROPERTY IMPORTED_CONFIGURATIONS Debug)
    endif()

    set_target_properties(Libqrcodegencpp::Libqrcodegencpp PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                                                      "${Libqrcodegencpp_INCLUDE_DIRS}")

  endif()
endif()

include(FeatureSummary)
set_package_properties(
  Libqrcodegencpp PROPERTIES
  URL "https://www.nayuki.io/page/qr-code-generator-library"
  DESCRIPTION "This project aims to be the best, clearest library for generating QR Codes in C++.")
