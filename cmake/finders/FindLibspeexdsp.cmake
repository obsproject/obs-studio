#[=======================================================================[.rst
FindLibspeexdsp
----------

FindModule for Libspeexdsp and associated libraries

.. versionchanged:: 3.0
  Updated FindModule to CMake standards

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` target ``SpeexDSP::Libspeexdsp``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``Libspeexdsp_FOUND``
  True, if all required components and the core library were found.
``Libspeexdsp_VERSION``
  Detected version of found Libspeexdsp libraries.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Libspeexdsp_LIBRARY``
  Path to the library component of Libspeexdsp.
``Libspeexdsp_INCLUDE_DIR``
  Directory containing ``speex/speex_preprocess.h``.

#]=======================================================================]

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-lint: disable=C0307
# cmake-format: on

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_Libspeexdsp QUIET speexdsp)
endif()

# libspeexdsp_set_soname: Set SONAME on imported library target
macro(libspeexdsp_set_soname)
  if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
    execute_process(
      COMMAND sh -c "otool -D '${Libspeexdsp_LIBRARY}' | grep -v '${Libspeexdsp_LIBRARY}'"
      OUTPUT_VARIABLE _output
      RESULT_VARIABLE _result)

    if(_result EQUAL 0 AND _output MATCHES "^@rpath/")
      set_property(TARGET SpeexDSP::Libspeexdsp PROPERTY IMPORTED_SONAME "${_output}")
    endif()
  elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
    execute_process(
      COMMAND sh -c "objdump -p '${Libspeexdsp_LIBRARY}' | grep SONAME"
      OUTPUT_VARIABLE _output
      RESULT_VARIABLE _result)

    if(_result EQUAL 0)
      string(REGEX REPLACE "[ \t]+SONAME[ \t]+([^ \t]+)" "\\1" _soname "${_output}")
      set_property(TARGET SpeexDSP::Libspeexdsp PROPERTY IMPORTED_SONAME "${_soname}")
      unset(_soname)
    endif()
  endif()
  unset(_output)
  unset(_result)
endmacro()

find_path(
  Libspeexdsp_INCLUDE_DIR
  NAMES speex/speex_preprocess.h
  HINTS ${PC_Libspeexdsp_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include
  DOC "Libspeexdsp include directory")

if(PC_Libspeexdsp_VERSION VERSION_GREATER 0)
  set(Libspeexdsp_VERSION ${PC_Libspeexdsp_VERSION})
else()
  if(NOT Libspeexdsp_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find Libspeexdsp version.")
  endif()
  set(Libspeexdsp_VERSION 0.0.0)
endif()

find_library(
  Libspeexdsp_LIBRARY
  NAMES speexdsp libspeexdsp
  HINTS ${PC_Libspeexdsp_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "Libspeexdsp location")

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin|Windows")
  set(Libspeexdsp_ERROR_REASON "Ensure that obs-deps is provided as part of CMAKE_PREFIX_PATH.")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
  set(Libspeexdsp_ERROR_REASON "Ensure that libspeexdsp is installed on the system.")
endif()

find_package_handle_standard_args(
  Libspeexdsp
  REQUIRED_VARS Libspeexdsp_LIBRARY Libspeexdsp_INCLUDE_DIR
  VERSION_VAR Libspeexdsp_VERSION REASON_FAILURE_MESSAGE "${Libspeexdsp_ERROR_REASON}")
mark_as_advanced(Libspeexdsp_INCLUDE_DIR Libspeexdsp_LIBRARY)
unset(Libspeexdsp_ERROR_REASON)

if(Libspeexdsp_FOUND)
  if(NOT TARGET SpeexDSP::Libspeexdsp)
    if(IS_ABSOLUTE "${Libspeexdsp_LIBRARY}")
      add_library(SpeexDSP::Libspeexdsp UNKNOWN IMPORTED)
      set_property(TARGET SpeexDSP::Libspeexdsp PROPERTY IMPORTED_LOCATION "${Libspeexdsp_LIBRARY}")
    else()
      add_library(SpeexDSP::Libspeexdsp INTERFACE IMPORTED)
      set_property(TARGET SpeexDSP::Libspeexdsp PROPERTY IMPORTED_LIBNAME "${Libspeexdsp_LIBRARY}")
    endif()

    libspeexdsp_set_soname()
    set_target_properties(
      SpeexDSP::Libspeexdsp
      PROPERTIES INTERFACE_COMPILE_OPTIONS "${PC_Libspeexdsp_CFLAGS_OTHER}"
                 INTERFACE_INCLUDE_DIRECTORIES "${Libspeexdsp_INCLUDE_DIR}"
                 VERSION ${Libspeexdsp_VERSION})
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  Libspeexdsp PROPERTIES
  URL "https://gitlab.xiph.org/xiph/speexdsp"
  DESCRIPTION "DSP library derived from speex.")
