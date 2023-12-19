#[=======================================================================[.rst
FindWebsocketpp
----------

FindModule for WebSocket++ and the associated library

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` target ``Websocketpp::Websocketpp``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``Websocketpp_FOUND``
  True, if the library was found.
``Websocketpp_VERSION``
  Detected version of found Websocketpp library.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Websocketpp_INCLUDE_DIR``
  Directory containing ``websocketpp/client.hpp``.

#]=======================================================================]

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-lint: disable=C0301
# cmake-format: on

include(FindPackageHandleStandardArgs)

find_path(
  Websocketpp_INCLUDE_DIR
  NAMES websocketpp/client.hpp
  PATHS /usr/include /usr/local/include
  DOC "WebSocket++ include directory")

if(EXISTS "${Websocketpp_INCLUDE_DIR}/websocketpp/version.hpp")
  file(STRINGS "${Websocketpp_INCLUDE_DIR}/websocketpp/version.hpp" _version_string
       REGEX "^.*(major|minor|patch)_version[ \t]+=[ \t]+[0-9]+")

  string(REGEX REPLACE ".*major_version[ \t]+=[ \t]+([0-9]+).*" "\\1" _version_major "${_version_string}")
  string(REGEX REPLACE ".*minor_version[ \t]+=[ \t]+([0-9]+).*" "\\1" _version_minor "${_version_string}")
  string(REGEX REPLACE ".*patch_version[ \t]+=[ \t]+([0-9]+).*" "\\1" _version_patch "${_version_string}")

  set(Websocketpp_VERSION "${_version_major}.${_version_minor}.${_version_patch}")
  unset(_version_major)
  unset(_version_minor)
  unset(_version_patch)
else()
  if(NOT Websocketpp_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find WebSocket++ version.")
  endif()
  set(Websocketpp_VERSION 0.0.0)
endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin|Windows")
  set(Websocketpp_ERROR_REASON "Ensure that obs-deps is provided as part of CMAKE_PREFIX_PATH.")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
  set(Websocketpp_ERROR_REASON "Ensure WebSocket++ library is available in local include paths.")
endif()

find_package_handle_standard_args(
  Websocketpp
  REQUIRED_VARS Websocketpp_INCLUDE_DIR
  VERSION_VAR Websocketpp_VERSION REASON_FAILURE_MESSAGE "${Websocketpp_ERROR_REASON}")
mark_as_advanced(Websocketpp_INCLUDE_DIR)
unset(Websocketpp_ERROR_REASON)

if(Websocketpp_FOUND)
  if(NOT TARGET Websocketpp::Websocketpp)
    add_library(Websocketpp::Websocketpp INTERFACE IMPORTED)
    set_target_properties(Websocketpp::Websocketpp PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                                              "${Websocketpp_INCLUDE_DIR}")
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  Websocketpp PROPERTIES
  URL "https://www.zaphoyd.com/websocketpp/"
  DESCRIPTION "WebSocket++ is a header only C++ library that implements RFC6455 The WebSocket Protocol.")
