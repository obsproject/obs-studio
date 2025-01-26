#[=======================================================================[.rst
FindLibx264
----------

FindModule for Libx264 and associated libraries

.. versionchanged:: 3.0
  Updated FindModule to CMake standards

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` target ``Libx264::Libx264``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``Libx264_FOUND``
  True, if all required components and the core library were found.
``Libx264_VERSION``
  Detected version of found Libx264 libraries.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Libx264_LIBRARY``
  Path to the library component of Libx264.
``Libx264_INCLUDE_DIR``
  Directory containing ``x264.h``.

#]=======================================================================]

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_Libx264 QUIET x264)
endif()

# Libx264_set_soname: Set SONAME on imported library target
macro(Libx264_set_soname)
  if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
    execute_process(
      COMMAND sh -c "otool -D '${Libx264_LIBRARY}' | grep -v '${Libx264_LIBRARY}'"
      OUTPUT_VARIABLE _output
      RESULT_VARIABLE _result
    )

    if(_result EQUAL 0 AND _output MATCHES "^@rpath/")
      set_property(TARGET Libx264::Libx264 PROPERTY IMPORTED_SONAME "${_output}")
    endif()
  elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
    execute_process(
      COMMAND sh -c "objdump -p '${Libx264_LIBRARY}' | grep SONAME"
      OUTPUT_VARIABLE _output
      RESULT_VARIABLE _result
    )

    if(_result EQUAL 0)
      string(REGEX REPLACE "[ \t]+SONAME[ \t]+([^ \t]+)" "\\1" _soname "${_output}")
      set_property(TARGET Libx264::Libx264 PROPERTY IMPORTED_SONAME "${_soname}")
      unset(_soname)
    endif()
  endif()
  unset(_output)
  unset(_result)
endmacro()

# Libx264_find_dll: Find DLL for corresponding import library
macro(Libx264_find_dll)
  cmake_path(GET Libx264_IMPLIB PARENT_PATH _implib_path)
  cmake_path(SET _bin_path NORMALIZE "${_implib_path}/../bin")

  string(REGEX REPLACE "[0-9]+\\.([0-9]+)\\.[0-9]+" "\\1" _dll_version "${Libx264_VERSION}")

  find_program(
    Libx264_LIBRARY
    NAMES libx264-${_dll_version}.dll x264-${_dll_version}.dll libx264.dll x264.dll
    HINTS ${_implib_path} ${_bin_path}
    DOC "Libx264 DLL location"
  )

  if(NOT Libx264_LIBRARY)
    set(Libx264_LIBRARY "${Libx264_IMPLIB}")
  endif()
  unset(_implib_path)
  unset(_bin_path)
  unset(_dll_version)
endmacro()

find_path(
  Libx264_INCLUDE_DIR
  NAMES x264.h
  HINTS ${PC_Libx264_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include
  DOC "Libx264 include directory"
)

if(PC_Libx264_VERSION VERSION_GREATER 0)
  set(Libx264_VERSION ${PC_Libx264_VERSION})
elseif(EXISTS "${Libx264_INCLUDE_DIR}/x264_config.h")
  file(STRINGS "${Libx264_INCLUDE_DIR}/x264_config.h" _VERSION_STRING REGEX "#define[ \t]+X264_POINTVER[ \t]+.+")
  string(REGEX REPLACE ".*#define[ \t]+X264_POINTVER[ \t]+\"([^ \t]+).*\".*" "\\1" Libx264_VERSION "${_VERSION_STRING}")
else()
  if(NOT Libx264_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find Libx264 version.")
  endif()
  set(Libx264_VERSION 0.0.0)
endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
  find_library(Libx264_IMPLIB NAMES x264 libx264 DOC "Libx264 import library location")

  libx264_find_dll()
else()
  find_library(
    Libx264_LIBRARY
    NAMES x264 libx264
    HINTS ${PC_Libx264_LIBRARY_DIRS}
    PATHS /usr/lib /usr/local/lib
    DOC "Libx264 location"
  )
endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin|Windows")
  set(Libx264_ERROR_REASON "Ensure that obs-deps is provided as part of CMAKE_PREFIX_PATH.")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
  set(Libx264_ERROR_REASON "Ensure that x264 is installed on the system.")
endif()

find_package_handle_standard_args(
  Libx264
  REQUIRED_VARS Libx264_LIBRARY Libx264_INCLUDE_DIR
  VERSION_VAR Libx264_VERSION
  REASON_FAILURE_MESSAGE "${Libx264_ERROR_REASON}"
)
mark_as_advanced(Libx264_INCLUDE_DIR Libx264_LIBRARY Libx264_IMPLIB)
unset(Libx264_ERROR_REASON)

if(Libx264_FOUND)
  if(NOT TARGET Libx264::Libx264)
    if(IS_ABSOLUTE "${Libx264_LIBRARY}")
      if(DEFINED Libx264_IMPLIB)
        if(Libx264_IMPLIB STREQUAL Libx264_LIBRARY)
          add_library(Libx264::Libx264 STATIC IMPORTED)
        else()
          add_library(Libx264::Libx264 SHARED IMPORTED)
          set_property(TARGET Libx264::Libx264 PROPERTY IMPORTED_IMPLIB "${Libx264_IMPLIB}")
        endif()
      else()
        add_library(Libx264::Libx264 UNKNOWN IMPORTED)
      endif()
      set_property(TARGET Libx264::Libx264 PROPERTY IMPORTED_LOCATION "${Libx264_LIBRARY}")
    else()
      add_library(Libx264::Libx264 INTERFACE IMPORTED)
      set_property(TARGET Libx264::Libx264 PROPERTY IMPORTED_LIBNAME "${Libx264_LIBRARY}")
    endif()

    libx264_set_soname()
    set_target_properties(
      Libx264::Libx264
      PROPERTIES
        INTERFACE_COMPILE_OPTIONS "${PC_Libx264_CFLAGS_OTHER}"
        INTERFACE_INCLUDE_DIRECTORIES "${Libx264_INCLUDE_DIR}"
        VERSION "${Libx264_VERSION}"
    )
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  Libx264
  PROPERTIES
    URL "https://www.videolan.org/developers/x264.html"
    DESCRIPTION
      "x264 is a free software library and application for encoding video streams into the H.264/MPEG-4 AVC compression format."
)
