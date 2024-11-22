#[=======================================================================[.rst
FindLibrnnoise
----------

FindModule for Librnnoise and associated libraries

.. versionchanged:: 3.0
  Updated FindModule to CMake standards

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` target ``Librnnoise::Librnnoise``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``Librnnoise_FOUND``
  True, if all required components and the core library were found.
``Librnnoise_VERSION``
  Detected version of found Librnnoise libraries.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Librnnoise_LIBRARY``
  Path to the library component of Librnnoise.
``Librnnoise_INCLUDE_DIR``
  Directory containing ``rnnoise.h``.

#]=======================================================================]

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_Librnnoise QUIET rnnoise)
endif()

# librrnoise_set_soname: Set SONAME on imported library target
macro(librnnoise_set_soname)
  if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
    execute_process(
      COMMAND sh -c "otool -D '${Librnnoise_LIBRARY}' | grep -v '${Librnnoise_LIBRARY}'"
      OUTPUT_VARIABLE _output
      RESULT_VARIABLE _result
    )

    if(_result EQUAL 0 AND _output MATCHES "^@rpath/")
      set_property(TARGET Librnnoise::Librnnoise PROPERTY IMPORTED_SONAME "${_output}")
    endif()
  elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
    execute_process(
      COMMAND sh -c "objdump -p '${Librnnoise_LIBRARY}' | grep SONAME"
      OUTPUT_VARIABLE _output
      RESULT_VARIABLE _result
    )

    if(_result EQUAL 0)
      string(REGEX REPLACE "[ \t]+SONAME[ \t]+([^ \t]+)" "\\1" _soname "${_output}")
      set_property(TARGET Librnnoise::Librnnoise PROPERTY IMPORTED_SONAME "${_soname}")
      unset(_soname)
    endif()
  endif()
  unset(_output)
  unset(_result)
endmacro()

find_path(
  Librnnoise_INCLUDE_DIR
  NAMES rnnoise.h
  HINTS ${PC_Librnnoise_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include
  DOC "Librnnoise include directory"
)

if(PC_Librnnoise_VERSION VERSION_GREATER 0)
  set(Librnnoise_VERSION ${PC_Librnnoise_VERSION})
else()
  if(NOT Librnnoise_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find Librnnoise version.")
  endif()
  set(Librnnoise_VERSION 0.0.0)
endif()

find_library(
  Librnnoise_LIBRARY
  NAMES rnnoise librnnoise
  HINTS ${PC_Librnnoise_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "Librnnoise location"
)

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin|Windows")
  set(Librnnoise_ERROR_REASON "Ensure that obs-deps is provided as part of CMAKE_PREFIX_PATH.")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
  set(Librnnoise_ERROR_REASON "Ensure librnnoise libraries are available in local libary paths.")
endif()

find_package_handle_standard_args(
  Librnnoise
  REQUIRED_VARS Librnnoise_LIBRARY Librnnoise_INCLUDE_DIR
  VERSION_VAR Librnnoise_VERSION
  REASON_FAILURE_MESSAGE "${Librnnoise_ERROR_REASON}"
)
mark_as_advanced(Librnnoise_INCLUDE_DIR Librnnoise_LIBRARY)
unset(Librnnoise_ERROR_REASON)

if(Librnnoise_FOUND)
  if(NOT TARGET Librnnoise::Librnnoise)
    if(IS_ABSOLUTE "${Librnnoise_LIBRARY}")
      add_library(Librnnoise::Librnnoise UNKNOWN IMPORTED)
      librnnoise_set_soname()
      set_property(TARGET Librnnoise::Librnnoise PROPERTY IMPORTED_LOCATION "${Librnnoise_LIBRARY}")
    else()
      add_library(Librnnoise::Librnnoise INTERFACE IMPORTED)
      set_property(TARGET Librnnoise::Librnnoise PROPERTY IMPORTED_LIBNAME "${Librnnoise_LIBRARY}")
    endif()

    set_target_properties(
      Librnnoise::Librnnoise
      PROPERTIES
        INTERFACE_COMPILE_OPTIONS "${PC_Librnnoise_CFLAGS_OTHER}"
        INTERFACE_INCLUDE_DIRECTORIES "${Librnnoise_INCLUDE_DIR}"
        VERSION ${Librnnoise_VERSION}
    )
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  Librnnoise
  PROPERTIES
    URL "https://gitlab.xiph.org/xiph/rnnoise"
    DESCRIPTION "Recurrent neural network for audio noise reduction."
)
