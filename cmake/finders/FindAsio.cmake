#[=======================================================================[.rst
FindAsio
--------

FindModule for Asio and the associated library

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` target ``Asio::Asio``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``Asio_FOUND``
  True, if the library was found.
``Asio_VERSION``
  Detected version of found Asio library.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Asio_INCLUDE_DIR``
  Directory containing ``asio.hpp``.

#]=======================================================================]

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-lint: disable=C0301
# cmake-format: on

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_Asio QUIET asio)
endif()

find_path(
  Asio_INCLUDE_DIR
  NAMES asio.hpp
  HINTS ${PC_Asio_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include
  DOC "Asio include directory")

if(PC_Asio_VERSION VERSION_GREATER 0)
  set(Asio_VERSION ${PC_Asio_VERSION})
elseif(EXISTS "${Asio_INCLUDE_DIR}/asio/version.hpp")
  file(STRINGS "${Asio_INCLUDE_DIR}/asio/version.hpp" _version_string
       REGEX "#define[ \t]+ASIO_VERSION[ \t]+[0-9]+[ \t]+\\/\\/[ \t][0-9]+\\.[0-9]+\\.[0-9]+")

  string(REGEX REPLACE "#define[ \t]+ASIO_VERSION[ \t]+[0-9]+[ \t]+\\/\\/[ \t]([0-9]+\\.[0-9]+\\.[0-9]+)" "\\1"
                       Asio_VERSION "${_version_string}")
else()
  if(NOT Asio_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find Asio version.")
  endif()
  set(Asio_VERSION 0.0.0)
endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin|Windows")
  set(Asio_ERROR_REASON "Ensure that obs-deps is provided as part of CMAKE_PREFIX_PATH.")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
  set(Asio_ERROR_REASON "Ensure Asio library is available in local include paths.")
endif()

find_package_handle_standard_args(
  Asio
  REQUIRED_VARS Asio_INCLUDE_DIR
  VERSION_VAR Asio_VERSION REASON_FAILURE_MESSAGE "${Asio_ERROR_REASON}")
mark_as_advanced(Asio_INCLUDE_DIR)
unset(Asio_ERROR_REASON)

if(Asio_FOUND)
  if(NOT TARGET Asio::Asio)
    add_library(Asio::Asio INTERFACE IMPORTED)
    set_target_properties(Asio::Asio PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${Asio_INCLUDE_DIR}")
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  Asio PROPERTIES
  URL "http://think-async.com/Asio"
  DESCRIPTION
    "Asio is a cross-platform C++ library for network and low-level I/O programming that provides developers with a consistent asynchronous model using a modern C++ approach."
)
