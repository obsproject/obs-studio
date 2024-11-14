#[=======================================================================[.rst
FindPython
----------

FindModule for Python import libraries and header on Windows

.. versionchanged:: 3.0
  Updated FindModule to CMake standards

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` target ``Python::Python``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``Python_FOUND``
  True, if all required components and the core library were found.
``Python_VERSION``
  Detected version of found Python libraries.
``Python_INCLUDE_DIRS``
  Include directories needed for Python.
``Python_LIBRARIES``
  Libraries needed to link to Python.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Python_LIBRARY``
  Path to the library component of Python.
``Python_INCLUDE_DIR``
  Directory containing ``python.h``.

#]=======================================================================]

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-lint: disable=C0301
# cmake-format: on

include(FindPackageHandleStandardArgs)

find_path(
  Python_INCLUDE_DIR
  NAMES Python.h
  PATH_SUFFIXES include include/python
  DOC "Python include directory")

find_library(
  Python_LIBRARY
  NAMES python3
  PATHS lib
  DOC "Python location")

if(EXISTS "${Python_INCLUDE_DIR}/patchlevel.h")
  file(STRINGS "${Python_INCLUDE_DIR}/patchlevel.h" _VERSION_STRING REGEX "^.*PY_VERSION[ \t]+\"[0-9\\.]+\"[ \t]*$")
  string(REGEX REPLACE ".*PY_VERSION[ \t]+\"([0-9\\.]+)\".*" "\\1" Python_VERSION "${_VERSION_STRING}")
else()
  if(NOT Python_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find Python version.")
  endif()
  set(Python_VERSION 0.0.0)
endif()

find_package_handle_standard_args(
  Python
  REQUIRED_VARS Python_LIBRARY Python_INCLUDE_DIR
  VERSION_VAR Python_VERSION HANDLE_VERSION_RANGE REASON_FAILURE_MESSAGE
                             "Ensure that obs-deps is provided as part of CMAKE_PREFIX_PATH.")
mark_as_advanced(Python_INCLUDE_DIR Python_LIBRARY)

if(Python_FOUND)
  set(Python_INCLUDE_DIRS ${Python_INCLUDE_DIR})
  set(Python_LIBRARIES ${Python_LIBRARY})
  set(Python_DEFINITIONS ${PC_Python_CFLAGS_OTHER})

  if(NOT TARGET Python::Python)
    if(IS_ABSOLUTE "${Python_LIBRARY}")
      add_library(Python::Python UNKNOWN IMPORTED)
      set_property(TARGET Python::Python PROPERTY IMPORTED_LOCATION "${Python_LIBRARY}")
    else()
      add_library(Python::Python INTERFACE IMPORTED)
      set_property(TARGET Python::Python PROPERTY IMPORTED_LIBNAME "${Python_LIBRARY}")
    endif()

    set_target_properties(Python::Python PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${Python_INCLUDE_DIR}"
                                                    VERSION ${Python_VERSION})
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  Python PROPERTIES
  URL "https://www.python.org"
  DESCRIPTION
    "Python is a programming language that lets you work more quickly and integrate your systems more effectively.")
