#[=======================================================================[.rst
FindWayland
-----------

FindModule for Wayland and and associated components

.. versionchanged:: 3.0
  Updated FindModule to CMake standards

Components
^^^^^^^^^^

.. versionadded:: 1.0

This module contains provides several components:

``Client``
``Server``
``EGL``
``Cursor``

Import targets exist for each component.

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` targets:

``Wayland::Client``
  Wayland client component

``Wayland::Server``
  Wayland server component

``Wayland::EGL``
  Wayland EGL component

``Wayland::Cursor``
  Wayland cursor component

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``Wayland_<COMPONENT>_FOUND``
  True, if required component was found.
``Wayland_<COMPONENT>_VERSION``
  Detected version of Wayland component.
``Wayland_<COMPONENT>_INCLUDE_DIRS``
  Include directories needed for Wayland component.
``Wayland_<COMPONENT>_LIBRARIES``
  Libraries needed to link to Wayland component.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Wayland_<COMPONENT>_LIBRARY``
  Path to the component of Wayland.
``Wayland_<COMPONENT>_INCLUDE_DIR``
  Directory containing ``<COMPONENT>.h``.

#]=======================================================================]

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-lint: disable=C0301
# cmake-format: on

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)

list(APPEND _Wayland_DEFAULT_COMPONENTS Client Server EGL Cursor)
list(APPEND _Wayland_LIBRARIES wayland-client wayland-server wayland-egl wayland-cursor)

if(NOT Wayland_FIND_COMPONENTS)
  set(Wayland_FIND_COMPONENTS ${_Wayland_DEFAULT_COMPONENTS})
endif()

# Wayland_find_component: Macro to setup targets for specified Wayland component
macro(Wayland_find_component component)
  list(GET _Wayland_DEFAULT_COMPONENTS ${component} COMPONENT_NAME)
  list(GET _Wayland_LIBRARIES ${component} COMPONENT_LIBRARY)

  if(NOT TARGET Wayland::${COMPONENT_NAME})
    if(PKG_CONFIG_FOUND)
      pkg_search_module(PC_Wayland_${COMPONENT_NAME} QUIET ${COMPONENT_LIBRARY})
    endif()

    find_path(
      Wayland_${COMPONENT_NAME}_INCLUDE_DIR
      NAMES ${COMPONENT_LIBRARY}.h
      HINTS ${PC_Wayland_${COMPONENT_NAME}_INCLUDE_DIRS}
      PATHS /usr/include /usr/local/include
      DOC "Wayland ${COMPONENT_NAME} include directory")

    find_library(
      Wayland_${COMPONENT_NAME}_LIBRARY
      NAMES ${COMPONENT_LIBRARY}
      HINTS ${PC_Wayland_${COMPONENT_NAME}_LIBRARY_DIRS}
      PATHS /usr/lib /usr/local/lib
      DOC "Wayland ${COMPONENT_NAME} location")

    if(PC_Wayland_${COMPONENT_NAME}_VERSION VERSION_GREATER 0)
      set(Wayland_${COMPONENT_NAME}_VERSION ${PC_Wayland_${COMPONENT_NAME}_VERSION})
    else()
      if(NOT Wayland_FIND_QUIETLY)
        message(AUTHOR_WARNING "Failed to find Wayland ${COMPONENT_NAME} version.")
      endif()
      set(Wayland_${COMPONENT_NAME}_VERSION 0.0.0)
    endif()

    if(Wayland_${COMPONENT_NAME}_LIBRARY AND Wayland_${COMPONENT_NAME}_INCLUDE_DIR)
      set(Wayland_${COMPONENT_NAME}_FOUND TRUE)
      set(Wayland_${COMPONENT_NAME}_LIBRARIES ${Wayland_${COMPONENT_NAME}_LIBRARY})
      set(Wayland_${COMPONENT_NAME}_INCLUDE_DIRS ${Wayland_${COMPONENT_NAME}_INCLUDE_DIR})
      set(Wayland_${COMPONENT_NAME}_DEFINITIONS ${PC_Wayland_${COMPONENT_NAME}_CFLAGS_OTHER})
      mark_as_advanced(Wayland_${COMPONENT_NAME}_LIBRARY Wayland_${COMPONENT_NAME}_INCLUDE_DIR)

      if(IS_ABSOLUTE "${Wayland_${COMPONENT_NAME}_LIBRARY}")
        add_library(Wayland::${COMPONENT_NAME} UNKNOWN IMPORTED)
        set_property(TARGET Wayland::${COMPONENT_NAME} PROPERTY IMPORTED_LOCATION
                                                                "${Wayland_${COMPONENT_NAME}_LIBRARY}")
      else()
        add_library(Wayland::${COMPONENT_NAME} INTERFACE IMPORTED)
        set_property(TARGET Wayland::${COMPONENT_NAME} PROPERTY IMPORTED_LIBNAME "${Wayland_${COMPONENT_NAME}_LIBRARY}")
      endif()

      set_target_properties(
        Wayland::${COMPONENT_NAME}
        PROPERTIES INTERFACE_COMPILE_OPTIONS "${PC_Wayland_${COMPONENT_NAME}_CFLAGS_OTHER}"
                   INTERFACE_INCLUDE_DIRECTORIES "${Wayland_${COMPONENT_NAME}_INCLUDE_DIR}"
                   VERSION ${Wayland_${COMPONENT_NAME}_VERSION})
      list(APPEND Wayland_COMPONENTS Wayland::${COMPONENT_NAME})
      list(APPEND Wayland_INCLUDE_DIRS ${Wayland_${COMPONENT_NAME}_INCLUDE_DIR})
      list(APPEND Wayland_LIBRARIES ${Wayland_${COMPONENT_NAME}_LIBRARY})

      if(NOT Wayland_VERSION)
        set(Wayland_VERSION ${Wayland_${COMPONENT_NAME}_VERSION})
      endif()
    endif()
  else()
    list(APPEND Wayland_COMPONENTS Wayland::${COMPONENT_NAME})
  endif()
endmacro()

foreach(component IN LISTS Wayland_FIND_COMPONENTS)
  list(FIND _Wayland_DEFAULT_COMPONENTS "${component}" valid_component)

  if(valid_component GREATER_EQUAL 0)
    wayland_find_component(${valid_component})
  else()
    message(FATAL_ERROR "Unknown Wayland component required: ${component}.")
  endif()
endforeach()

find_package_handle_standard_args(
  Wayland
  REQUIRED_VARS Wayland_LIBRARIES Wayland_INCLUDE_DIRS
  VERSION_VAR Wayland_VERSION
  HANDLE_COMPONENTS REASON_FAILURE_MESSAGE "Ensure that Wayland is installed on the system.")

unset(_Wayland_DEFAULT_COMPONENTS)
unset(_Wayland_LIBRARIES)

include(FeatureSummary)
set_package_properties(
  Wayland PROPERTIES
  URL "https://wayland.freedesktop.org"
  DESCRIPTION
    "A replacement for the X11 window system protocol and architecture with the aim to be easier to develop, extend, and maintain."
)
