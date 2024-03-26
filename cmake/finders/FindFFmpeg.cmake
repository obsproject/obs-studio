#[=======================================================================[.rst
FindFFmpeg
----------

FindModule for FFmpeg and associated libraries

.. versionchanged:: 3.0
  Updated FindModule to CMake standards

Components
^^^^^^^^^^

.. versionadded:: 1.0

This module contains provides several components:

``avcodec``
``avdevice``
``avfilter``
``avformat``
``avutil``
``postproc``
``swscale``
``swresample``

Import targets exist for each component.

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` targets:

``FFmpeg::avcodec``
  AVcodec component

``FFmpeg::avdevice``
  AVdevice component

``FFmpeg::avfilter``
  AVfilter component

``FFmpeg::avformat``
  AVformat component

``FFmpeg::avutil``
  AVutil component

``FFmpeg::postproc``
  postproc component

``FFmpeg::swscale``
  SWscale component

``FFmpeg::swresample``
  SWresample component

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``FFmpeg_FOUND``
  True, if all required components and the core library were found.
``FFmpeg_VERSION``
  Detected version of found FFmpeg libraries.
``FFmpeg_INCLUDE_DIRS``
  Include directories needed for FFmpeg.
``FFmpeg_LIBRARIES``
  Libraries needed to link to FFmpeg.
``FFmpeg_DEFINITIONS``
  Compiler flags required for FFmpeg.

``FFmpeg_<COMPONENT>_VERSION``
  Detected version of found FFmpeg component library.
``FFmpeg_<COMPONENT>_INCLUDE_DIRS``
  Include directories needed for FFmpeg component.
``FFmpeg_<COMPONENT>_LIBRARIES``
  Libraries needed to link to FFmpeg component.
``FFmpeg_<COMPONENT>_DEFINITIONS``
  Compiler flags required for FFmpeg component.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``FFmpeg_<COMPONENT>_LIBRARY``
  Path to the library component of FFmpeg.
``FFmpeg_<COMPONENT>_INCLUDE_DIR``
  Directory containing ``<COMPONENT>.h``.

#]=======================================================================]

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-lint: disable=C0307
# cmake-format: on

include(FindPackageHandleStandardArgs)

set(
  _DEFAULT_COMPONENTS
  avcodec
  avdevice
  avformat
  avfilter
  avresample
  avutil
  postproc
  swscale
  swresample
)

set(component_avcodec libavcodec avcodec avcodec.h)
set(component_avdevice libavdevice avdevice avdevice.h)
set(component_avformat libavformat avformat avformat.h)
set(component_avfilter libavfilter avfilter avfilter.h)
set(component_avresample libavresample avresample avresample.h)
set(component_avutil libavutil avutil avutil.h)
set(component_postproc libpostproc postproc postprocess.h)
set(component_swscale libswscale swscale swscale.h)
set(component_swresample libswresample swresample swresample.h)

if(NOT FFmpeg_FIND_COMPONENTS)
  set(FFmpeg_FIND_COMPONENTS ${_DEFAULT_COMPONENTS})
endif()

# FFmpeg_find_component: Find and set up requested FFmpeg component
macro(FFmpeg_find_component component)
  list(GET component_${component} 0 component_libname)
  list(GET component_${component} 1 component_name)
  list(GET component_${component} 2 component_header)

  if(NOT CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
      pkg_search_module(PC_FFmpeg_${component} QUIET ${component_libname})
    endif()
  endif()

  find_path(
    FFmpeg_${component}_INCLUDE_DIR
    NAMES ${component_libname}/${component_header} ${component_libname}/version.h
    HINTS ${PC_FFmpeg_${component}_INCLUDE_DIRS}
    PATHS /usr/include /usr/local/include
    DOC "FFmpeg component ${component_name} include directory"
  )

  ffmpeg_check_version()

  if(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
    find_library(
      FFmpeg_${component}_IMPLIB
      NAMES ${component_libname} ${component_name}
      DOC "FFmpeg component ${component_name} import library location"
    )

    ffmpeg_find_dll()
  else()
    find_library(
      FFmpeg_${component}_LIBRARY
      NAMES ${component_libname} ${component_name}
      HINTS ${PC_FFmpeg_${component}_LIBRARY_DIRS}
      PATHS /usr/lib /usr/local/lib
      DOC "FFmpeg component ${component_name} location"
    )
  endif()

  if(FFmpeg_${component}_LIBRARY AND FFmpeg_${component}_INCLUDE_DIR)
    set(FFmpeg_${component}_FOUND TRUE)
    set(FFmpeg_${component}_LIBRARIES ${${_library_var}})
    set(FFmpeg_${component}_INCLUDE_DIRS ${FFmpeg_${component}_INCLUDE_DIR})
    set(FFmpeg_${component}_DEFINITIONS ${PC_FFmpeg_${component}_CFLAGS_OTHER})
    mark_as_advanced(FFmpeg_${component}_LIBRARY FFmpeg_${component}_INCLUDE_DIR FFmpeg_${component}_IMPLIB)
  endif()
endmacro()

# FFmpeg_find_dll: Macro to find DLL for corresponding import library
macro(FFmpeg_find_dll)
  cmake_path(GET FFmpeg_${component}_IMPLIB PARENT_PATH _implib_path)
  cmake_path(SET _bin_path NORMALIZE "${_implib_path}/../bin")

  string(REGEX REPLACE "([0-9]+)\\.[0-9]+\\.[0-9]+" "\\1" _dll_version "${FFmpeg_${component}_VERSION}")

  find_program(
    FFmpeg_${component}_LIBRARY
    NAMES ${component_name}-${_dll_version}.dll
    HINTS ${_implib_path} ${_bin_path}
    DOC "FFmpeg component ${component_name} DLL location"
  )

  if(NOT FFmpeg_${component}_LIBRARY)
    set(FFmpeg_${component}_LIBRARY "${FFmpeg_${component}_IMPLIB}")
  endif()

  unset(_implib_path)
  unset(_bin_path)
  unset(_dll_version)
endmacro()

# FFmpeg_check_version: Macro to help extract version number from FFmpeg headers
macro(FFmpeg_check_version)
  if(PC_FFmpeg_${component}_VERSION)
    set(FFmpeg_${component}_VERSION ${PC_FFmpeg_${component}_VERSION})
  elseif(EXISTS "${FFmpeg_${component}_INCLUDE_DIR}/${component_libname}/version.h")
    if(EXISTS "${FFmpeg_${component}_INCLUDE_DIR}/${component_libname}/version_major.h")
      file(
        STRINGS
        "${FFmpeg_${component}_INCLUDE_DIR}/${component_libname}/version_major.h"
        _version_string
        REGEX "^.*VERSION_MAJOR[ \t]+[0-9]+[ \t]*$"
      )
      string(REGEX REPLACE ".*VERSION_MAJOR[ \t]+([0-9]+).*" "\\1" _version_major "${_version_string}")

      file(
        STRINGS
        "${FFmpeg_${component}_INCLUDE_DIR}/${component_libname}/version.h"
        _version_string
        REGEX "^.*VERSION_(MINOR|MICRO)[ \t]+[0-9]+[ \t]*$"
      )
      string(REGEX REPLACE ".*VERSION_MINOR[ \t]+([0-9]+).*" "\\1" _version_minor "${_version_string}")
      string(REGEX REPLACE ".*VERSION_MICRO[ \t]+([0-9]+).*" "\\1" _version_patch "${_version_string}")
    else()
      file(
        STRINGS
        "${FFmpeg_${component}_INCLUDE_DIR}/${component_libname}/version.h"
        _version_string
        REGEX "^.*VERSION_(MAJOR|MINOR|MICRO)[ \t]+[0-9]+[ \t]*$"
      )
      string(REGEX REPLACE ".*VERSION_MAJOR[ \t]+([0-9]+).*" "\\1" _version_major "${_version_string}")
      string(REGEX REPLACE ".*VERSION_MINOR[ \t]+([0-9]+).*" "\\1" _version_minor "${_version_string}")
      string(REGEX REPLACE ".*VERSION_MICRO[ \t]+([0-9]+).*" "\\1" _version_patch "${_version_string}")
    endif()

    set(FFmpeg_${component}_VERSION "${_version_major}.${_version_minor}.${_version_patch}")
    unset(_version_major)
    unset(_version_minor)
    unset(_version_patch)
  else()
    if(NOT FFmpeg_FIND_QUIETLY)
      message(AUTHOR_WARNING "Failed to find ${component_name} version.")
    endif()
    set(FFmpeg_${component}_VERSION 0.0.0)
  endif()
endmacro()

# FFmpeg_set_soname: Set SONAME property on imported library targets
macro(FFmpeg_set_soname)
  if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
    execute_process(
      COMMAND sh -c "otool -D '${FFmpeg_${component}_LIBRARY}' | grep -v '${FFmpeg_${component}_LIBRARY}'"
      OUTPUT_VARIABLE _output
      RESULT_VARIABLE _result
    )

    if(_result EQUAL 0 AND _output MATCHES "^@rpath/")
      set_property(TARGET FFmpeg::${component} PROPERTY IMPORTED_SONAME "${_output}")
    endif()
  elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
    execute_process(
      COMMAND sh -c "objdump -p '${FFmpeg_${component}_LIBRARY}' | grep SONAME"
      OUTPUT_VARIABLE _output
      RESULT_VARIABLE _result
    )

    if(_result EQUAL 0)
      string(REGEX REPLACE "[ \t]+SONAME[ \t]+([^ \t]+)" "\\1" _soname "${_output}")
      set_property(TARGET FFmpeg::${component} PROPERTY IMPORTED_SONAME "${_soname}")
      unset(_soname)
    endif()
  endif()
  unset(_output)
  unset(_result)
endmacro()

foreach(component IN LISTS FFmpeg_FIND_COMPONENTS)
  if(NOT component IN_LIST _DEFAULT_COMPONENTS)
    message(FATAL_ERROR "Unknown FFmpeg component specified: ${component}.")
  endif()

  if(NOT FFmpeg_${component}_FOUND)
    ffmpeg_find_component(${component})
  endif()

  if(FFmpeg_${component}_FOUND)
    list(APPEND FFmpeg_LIBRARIES ${FFmpeg_${component}_LIBRARY})
    list(APPEND FFmpeg_DEFINITIONS ${FFmpeg_${component}_DEFINITIONS})
    list(APPEND FFmpeg_INCLUDE_DIRS ${FFmpeg_${component}_INCLUDE_DIR})
  endif()
endforeach()

if(NOT FFmpeg_avutil_FOUND)
  ffmpeg_find_component(avutil)
endif()

if(EXISTS "${FFmpeg_avutil_INCLUDE_DIR}/libavutil/ffversion.h")
  file(
    STRINGS
    "${FFmpeg_avutil_INCLUDE_DIR}/libavutil/ffversion.h"
    _version_string
    REGEX "^.*FFMPEG_VERSION[ \t]+\"n?[0-9a-z\\~.-]+\"[ \t]*$"
  )
  string(REGEX REPLACE ".*FFMPEG_VERSION[ \t]+\"n?([0-9]+\\.[0-9]).*\".*" "\\1" FFmpeg_VERSION "${_version_string}")
endif()

list(REMOVE_DUPLICATES FFmpeg_INCLUDE_DIRS)
list(REMOVE_DUPLICATES FFmpeg_LIBRARIES)
list(REMOVE_DUPLICATES FFmpeg_DEFINITIONS)

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin|Windows")
  set(FFmpeg_ERROR_REASON "Ensure that obs-deps is provided as part of CMAKE_PREFIX_PATH.")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
  set(FFmpeg_ERROR_REASON "Ensure that required FFmpeg libraries are installed on the system.")
endif()

find_package_handle_standard_args(
  FFmpeg
  REQUIRED_VARS FFmpeg_LIBRARIES FFmpeg_INCLUDE_DIRS
  VERSION_VAR FFmpeg_VERSION
  HANDLE_COMPONENTS
  REASON_FAILURE_MESSAGE "${FFmpeg_ERROR_REASON}"
)

if(FFmpeg_FOUND AND NOT TARGET FFmpeg::FFmpeg)
  add_library(FFmpeg::FFmpeg INTERFACE IMPORTED)
endif()

foreach(component IN LISTS FFmpeg_FIND_COMPONENTS)
  if(FFmpeg_${component}_FOUND AND NOT TARGET FFmpeg::${component})
    if(IS_ABSOLUTE "${FFmpeg_${component}_LIBRARY}")
      if(DEFINED FFmpeg_${component}_IMPLIB)
        if(FFmpeg_${component}_IMPLIB STREQUAL FFmpeg_${component}_LIBRARY)
          add_library(FFmpeg::${component} STATIC IMPORTED)
        else()
          add_library(FFmpeg::${component} SHARED IMPORTED)
          set_property(TARGET FFmpeg::${component} PROPERTY IMPORTED_IMPLIB "${FFmpeg_${component}_IMPLIB}")
        endif()
      else()
        add_library(FFmpeg::${component} UNKNOWN IMPORTED)
        ffmpeg_set_soname()
      endif()

      set_property(TARGET FFmpeg::${component} PROPERTY IMPORTED_LOCATION "${FFmpeg_${component}_LIBRARY}")
    else()
      add_library(FFmpeg::${component} INTERFACE IMPORTED)
      set_property(TARGET FFmpeg::${component} PROPERTY IMPORTED_LIBNAME "${FFmpeg_${component}_LIBRARY}")
    endif()
    set_target_properties(
      FFmpeg::${component}
      PROPERTIES
        INTERFACE_COMPILE_OPTIONS "${PC_FFmpeg_${component}_CFLAGS_OTHER}"
        INTERFACE_INCLUDE_DIRECTORIES "${FFmpeg_${component}_INCLUDE_DIR}"
        VERSION ${FFmpeg_${component}_VERSION}
    )

    get_target_property(_ffmpeg_interface_libraries FFmpeg::FFmpeg INTERFACE_LINK_LIBRARIES)
    if(NOT FFmpeg::${component} IN_LIST _ffmpeg_interface_libraries)
      set_property(TARGET FFmpeg::FFmpeg APPEND PROPERTY INTERFACE_LINK_LIBRARIES FFmpeg::${component})
    endif()
  endif()
endforeach()

include(FeatureSummary)
set_package_properties(
  FFmpeg
  PROPERTIES
    URL "https://www.ffmpeg.org"
    DESCRIPTION "A complete, cross-platform solution to record, convert and stream audio and video."
)
