#[=======================================================================[.rst
FindSteinbergVST3
-----------

FindModule for Steinberg VST3 SDK

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 3.0

This module defines the :prop_tgt:`IMPORTED` target ``Steinberg::VST3``.


Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``SteinbergVST3_FOUND``
  True, if all required components and the core library were found.

``SteinbergVST3_VERSION``
  Detected version of Steinberg VST3 SDK.

#]=======================================================================]

include(FindPackageHandleStandardArgs)

# FIXME: Build static library of VST3 SDK as part of obs-deps for Windows and macOS, only require source files
#        on Linux and convert Windows and macOS library type from OBJECT to INTERFACE.
set(
  SteinbergVST3_REQUIRED_SOURCES
  vst3sdk/pluginterfaces/base/coreiids.cpp
  vst3sdk/pluginterfaces/base/funknown.cpp
  vst3sdk/public.sdk/source/common/commonstringconvert.cpp
  vst3sdk/public.sdk/source/common/memorystream.cpp
  vst3sdk/public.sdk/source/vst/hosting/connectionproxy.cpp
  vst3sdk/public.sdk/source/vst/hosting/eventlist.cpp
  vst3sdk/public.sdk/source/vst/hosting/hostclasses.cpp
  vst3sdk/public.sdk/source/vst/hosting/module.cpp
  vst3sdk/public.sdk/source/vst/hosting/parameterchanges.cpp
  vst3sdk/public.sdk/source/vst/hosting/pluginterfacesupport.cpp
  vst3sdk/public.sdk/source/vst/hosting/plugprovider.cpp
  vst3sdk/public.sdk/source/vst/hosting/processdata.cpp
  vst3sdk/public.sdk/source/vst/moduleinfo/moduleinfoparser.cpp
  vst3sdk/public.sdk/source/vst/utility/stringconvert.cpp
  vst3sdk/public.sdk/source/vst/vstinitiids.cpp
)

if(OS_WINDOWS)
  list(
    APPEND SteinbergVST3_REQUIRED_SOURCES
    vst3sdk/public.sdk/source/common/threadchecker_win32.cpp
    vst3sdk/public.sdk/source/vst/hosting/module_win32.cpp
  )
elseif(OS_MACOS)
  list(
    APPEND SteinbergVST3_REQUIRED_SOURCES
    vst3sdk/public.sdk/source/common/threadchecker_mac.mm
    vst3sdk/public.sdk/source/vst/hosting/module_mac.mm
  )
elseif(OS_LINUX)
  list(
    APPEND SteinbergVST3_REQUIRED_SOURCES
    vst3sdk/public.sdk/source/common/threadchecker_linux.cpp
    vst3sdk/public.sdk/source/vst/hosting/module_linux.cpp
  )
endif()

find_path(
  SteinbergVST3_INCLUDE_DIR
  NAMES vst3sdk/pluginterfaces/base/funknown.h
  PATHS "${STEINBERG_VST3_PATH}" /usr/include /usr/local/include
  DOC "Steinberg VST3 SDK directory"
)

set(SteinbergVST3_SOURCES)
foreach(_vst3sdk_source_file IN LISTS SteinbergVST3_REQUIRED_SOURCES)
  set(_source_file_path "${SteinbergVST3_INCLUDE_DIR}/${_vst3sdk_source_file}")
  if(NOT EXISTS "${_source_file_path}")
    unset(SteinbergVST3_SOURCES)
    break()
  else()
    list(APPEND SteinbergVST3_SOURCES "${_source_file_path}")
  endif()
endforeach()

set(SteinbergVST3_VERSION 3.0.0)

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin|Windows")
  set(SteinbergVST3_ERROR_REASON "Ensure that obs-deps is provided as part of CMAKE_PREFIX_PATH.")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
  set(
    SteinbergVST3_ERROR_REASON
    "Ensure Steinberg VST3 SDK is available in local library paths or provided via STEINBERG_VST3_PATH."
  )
endif()

find_package_handle_standard_args(
  SteinbergVST3
  REQUIRED_VARS SteinbergVST3_INCLUDE_DIR SteinbergVST3_SOURCES
  VERSION_VAR SteinbergVST3_VERSION
  REASON_FAILURE_MESSAGE "${SteinbergVST3_ERROR_REASON}"
)
mark_as_advanced(SteinbergVST3_INCLUDE_DIR)
unset(SteinbergVST3_ERROR_REASON)

if(SteinbergVST3_FOUND)
  if(NOT TARGET Steinberg::VST3)
    add_library(Steinberg::VST3 INTERFACE IMPORTED)
    target_sources(Steinberg::VST3 INTERFACE "${SteinbergVST3_SOURCES}")
    set_property(TARGET Steinberg::VST3 APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${SteinbergVST3_INCLUDE_DIR}")
    set_property(
      TARGET Steinberg::VST3
      APPEND
      PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${SteinbergVST3_INCLUDE_DIR}/vst3sdk"
    )
  endif()
endif()

set_property(
  SOURCE "${SteinbergVST3_INCLUDE_DIR}/vst3sdk/public.sdk/source/common/commonstringconvert.cpp"
  PROPERTY COMPILE_OPTIONS "-Wno-deprecated-declarations"
)

set_property(
  SOURCE "${SteinbergVST3_INCLUDE_DIR}/vst3sdk/public.sdk/source/vst/utility/stringconvert.cpp"
  PROPERTY COMPILE_OPTIONS "-Wno-deprecated-declarations"
)

if(OS_MACOS)
  set_property(
    SOURCE "${SteinbergVST3_INCLUDE_DIR}/vst3sdk/public.sdk/source/common/threadchecker_mac.mm"
    PROPERTY COMPILE_OPTIONS "-fobjc-arc"
  )
  set_property(
    SOURCE "${SteinbergVST3_INCLUDE_DIR}/vst3sdk/public.sdk/source/vst/hosting/module_mac.mm"
    PROPERTY COMPILE_OPTIONS "-fobjc-arc;-Wno-unused-parameter"
  )
endif()

include(FeatureSummary)
set_package_properties(
  VST3SDK
  PROPERTIES
    URL "https://www.steinberg.net/developers/"
    DESCRIPTION "The Steinberg VST3 SDK provides the headers and sources for hosting and developing VST3 plug-ins."
)
