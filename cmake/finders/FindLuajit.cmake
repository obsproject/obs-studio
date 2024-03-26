#[=======================================================================[.rst
FindLuajit
----------

FindModule for Luajit and associated libraries

.. versionchanged:: 3.0
  Updated FindModule to CMake standards

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` target ``Luajit::Luajit``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``Luajit_FOUND``
  True, if all required components and the core library were found.
``Luajit_VERSION``
  Detected version of found Luajit libraries.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Luajit_LIBRARY``
  Path to the library component of Luajit.
``Luajit_INCLUDE_DIR``
  Directory containing ``luajit.h`` or ``lua.h``.

#]=======================================================================]

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_Luajit QUIET luajit)
endif()

# Luajit_set_soname: Set SONAME on imported library target
macro(Luajit_set_soname)
  if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
    execute_process(
      COMMAND sh -c "otool -D '${Luajit_LIBRARY}' | grep -v '${Luajit_LIBRARY}'"
      OUTPUT_VARIABLE _output
      RESULT_VARIABLE _result
    )

    if(_result EQUAL 0 AND _output MATCHES "^@rpath/")
      set_property(TARGET Luajit::Luajit PROPERTY IMPORTED_SONAME "${_output}")
    endif()
  elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
    execute_process(
      COMMAND sh -c "objdump -p '${Luajit_LIBRARY}' | grep SONAME"
      OUTPUT_VARIABLE _output
      RESULT_VARIABLE _result
    )

    if(_result EQUAL 0)
      string(REGEX REPLACE "[ \t]+SONAME[ \t]+([^ \t]+)" "\\1" _soname "${_output}")
      set_property(TARGET Luajit::Luajit PROPERTY IMPORTED_SONAME "${_soname}")
      unset(_soname)
    endif()
  endif()
  unset(_output)
  unset(_result)
endmacro()

find_path(
  Luajit_INCLUDE_DIR
  NAMES lua.h luajit.h
  HINTS ${PC_Luajit_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include
  PATH_SUFFIXES luajit-2.1 luajit-2.0 luajit
  DOC "Luajit include directory"
)

if(PC_Luajit_VERSION VERSION_GREATER 0)
  set(Luajit_VERSION ${PC_Luajit_VERSION})
elseif(EXISTS "${Luajit_INCLUDE_DIR}/luajit.h")
  file(STRINGS "${Luajit_INCLUDE_DIR}/luajit.h" _VERSION_STRING REGEX "#define[ \t]+LUAJIT_VERSION[ \t]+\".+\".*")
  string(REGEX REPLACE ".*#define[ \t]+LUAJIT_VERSION[ \t]+\"LuaJIT (.+)\".*" "\\1" Luajit_VERSION "${_VERSION_STRING}")
else()
  if(NOT Luajit_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find Luajit version.")
  endif()
endif()

find_library(
  Luajit_LIBRARY
  NAMES luajit luajit-51 luajit-5.1 lua51
  HINTS ${PC_Luajit_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "Luajit location"
)

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin|Windows")
  set(Luajit_ERROR_REASON "Ensure that obs-deps is provided as part of CMAKE_PREFIX_PATH.")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
  set(Luajit_ERROR_REASON "Ensure that LuaJIT is installed on the system.")
endif()

find_package_handle_standard_args(
  Luajit
  REQUIRED_VARS Luajit_LIBRARY Luajit_INCLUDE_DIR
  VERSION_VAR Luajit_VERSION
  REASON_FAILURE_MESSAGE "${Luajit_ERROR_REASON}"
)
mark_as_advanced(Luajit_INCLUDE_DIR Luajit_LIBRARY)
unset(Luajit_ERROR_REASON)

if(Luajit_FOUND)
  if(NOT TARGET Luajit::Luajit)
    if(IS_ABSOLUTE "${Luajit_LIBRARY}")
      add_library(Luajit::Luajit UNKNOWN IMPORTED)
      set_property(TARGET Luajit::Luajit PROPERTY IMPORTED_LOCATION "${Luajit_LIBRARY}")
    else()
      add_library(Luajit::Luajit INTERFACE IMPORTED)
      set_property(TARGET Luajit::Luajit PROPERTY IMPORTED_LIBNAME "${Luajit_LIBRARY}")
    endif()

    luajit_set_soname()
    set_target_properties(
      Luajit::Luajit
      PROPERTIES
        INTERFACE_COMPILE_OPTIONS "${PC_Luajit_CFLAGS_OTHER}"
        INTERFACE_INCLUDE_DIRECTORIES "${Luajit_INCLUDE_DIR}"
        VERSION ${Luajit_VERSION}
    )
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  Luajit
  PROPERTIES
    URL "https://luajit.org/luajit.html"
    DESCRIPTION "LuaJIT is a Just-In-Time Compiler (JIT) for the Lua programming language."
)
