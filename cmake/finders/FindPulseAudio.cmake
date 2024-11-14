#[=======================================================================[.rst
FindPulseAudio
--------------

FindModule for PulseAudio and associated libraries

.. versionchanged:: 3.0
  Updated FindModule to CMake standards

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` target ``PulseAudio::PulseAudio``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``PulseAudio_FOUND``
  True, if all required components and the core library were found.
``PulseAudio_VERSION``
  Detected version of found PulseAudio libraries.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``PulseAudio_LIBRARY``
  Path to the library component of PulseAudio.
``PulseAudio_INCLUDE_DIR``
  Directory containing ``pulseaudio.h``.

#]=======================================================================]

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-lint: disable=C0301
# cmake-format: on

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_PulseAudio QUIET libpulse)
endif()

find_path(
  PulseAudio_INCLUDE_DIR
  NAMES pulse/pulseaudio.h
  HINTS ${PC_PulseAudio_INCLUDE_DIRS}
  PATHS /usr/include/ /usr/local/include
  DOC "PulseAudio include directory")

find_library(
  PulseAudio_LIBRARY
  NAMES pulse
  HINTS ${PC_PulseAudio_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "PulseAudio location")

if(PC_PulseAudio_VERSION VERSION_GREATER 0)
  set(PulseAudio_VERSION ${PC_PulseAudio_VERSION})
elseif(EXISTS "${PulseAudio_INCLUDE_DIR}/version.h")
  file(STRINGS "${PulseAudio_INCLUDE_DIR}/version.h" _VERSION_STRING
       REGEX "^.*pa_get_headers_version\\(\\)[\t ]+\\(\".*\"\\)[ \t]*$")
  string(REGEX REPLACE ".*pa_get_headers_version\\(\\)[\t ]+\\(\"([^\"]*)\"\\).*" "\\1" PulseAudio_VERSION
                       "${_VERSION_STRING}")
else()
  if(NOT PulseAudio_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find PulseAudio version.")
  endif()
  set(PulseAudio_VERSION 0.0.0)
endif()

find_package_handle_standard_args(
  PulseAudio
  REQUIRED_VARS PulseAudio_INCLUDE_DIR PulseAudio_LIBRARY
  VERSION_VAR PulseAudio_VERSION REASON_FAILURE_MESSAGE "Ensure that PulseAudio is installed on the system.")
mark_as_advanced(PulseAudio_INCLUDE_DIR PulseAudio_LIBRARY)

if(PulseAudio_FOUND)
  if(NOT TARGET PulseAudio::PulseAudio)
    if(IS_ABSOLUTE "${PulseAudio_LIBRARY}")
      add_library(PulseAudio::PulseAudio UNKNOWN IMPORTED)
      set_property(TARGET PulseAudio::PulseAudio PROPERTY IMPORTED_LOCATION "${PulseAudio_LIBRARY}")
    else()
      add_library(PulseAudio::PulseAudio INTERFACE IMPORTED)
      set_property(TARGET PulseAudio::PulseAudio PROPERTY IMPORTED_LIBNAME "${PulseAudio_LIBRARY}")
    endif()

    set_target_properties(
      PulseAudio::PulseAudio
      PROPERTIES INTERFACE_COMPILE_OPTIONS "${PC_PulseAudio_CFLAFGS_OTHER}"
                 INTERFACE_INCLUDE_DIRECTORIES "${PulseAudio_INCLUDE_DIR}"
                 VERSION ${PulseAudio_VERSION})
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  PulseAudio PROPERTIES
  URL "https://www.freedesktop.org/wiki/Software/PulseAudio/"
  DESCRIPTION
    "PulseAudio is a sound server system for POSIX OSes, meaning that it is a proxy for your sound applications.")
