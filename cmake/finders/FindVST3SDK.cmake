#[=======================================================================[.rst
FindVST3SDK
-----------

FindModule for VST3 SDK

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 3.0

This module defines the :prop_tgt:`IMPORTED` target ``VST3::SDK``.


Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``VST3SDK_FOUND``
  True, if all required components and the core library were found.

``VST3SDK_PATH``
  Path to the SDK.
  
``VST3SDK_REQUIRED_FILES``
  List of required files.
#]=======================================================================]

include(FindPackageHandleStandardArgs)

find_path(
  VST3SDK_PATH
  NAMES pluginterfaces/base/funknown.h
  PATHS ${VST3SDK_PATH} ${CMAKE_SOURCE_DIR}/plugins/obs-vst3/sdk ${CMAKE_SOURCE_DIR}/deps/vst3sdk
  NO_DEFAULT_PATH
)

if(VST3SDK_PATH)
  set(
    VST3SDK_REQUIRED_FILES
    pluginterfaces/base/funknown.cpp
    pluginterfaces/base/coreiids.cpp
    public.sdk/source/vst/vstinitiids.cpp
    public.sdk/source/vst/hosting/connectionproxy.cpp
    public.sdk/source/vst/hosting/eventlist.cpp
    public.sdk/source/vst/hosting/hostclasses.cpp
    public.sdk/source/vst/hosting/module.cpp
    public.sdk/source/vst/hosting/parameterchanges.cpp
    public.sdk/source/vst/hosting/pluginterfacesupport.cpp
    public.sdk/source/vst/hosting/processdata.cpp
    public.sdk/source/vst/hosting/plugprovider.cpp
    public.sdk/source/vst/moduleinfo/moduleinfoparser.cpp
    public.sdk/source/common/commonstringconvert.cpp
    public.sdk/source/common/memorystream.cpp
    public.sdk/source/vst/utility/stringconvert.cpp
  )

  if(OS_WINDOWS)
    list(
      APPEND VST3SDK_REQUIRED_FILES
      public.sdk/source/vst/hosting/module_win32.cpp
      public.sdk/source/common/threadchecker_win32.cpp
    )
  elseif(OS_MACOS)
    list(
      APPEND VST3SDK_REQUIRED_FILES
      public.sdk/source/vst/hosting/module_mac.mm
      public.sdk/source/common/threadchecker_mac.mm
    )
  elseif(OS_LINUX)
    list(
      APPEND VST3SDK_REQUIRED_FILES
      public.sdk/source/vst/hosting/module_linux.cpp
      public.sdk/source/common/threadchecker_linux.cpp
    )
  endif()

  set(_vst3sdk_missing "")
  foreach(_f IN LISTS VST3SDK_REQUIRED_FILES)
    if(NOT EXISTS "${VST3SDK_PATH}/${_f}")
      list(APPEND _vst3sdk_missing "${_f}")
    endif()
  endforeach()

  if(_vst3sdk_missing)
    message(STATUS "VST3 SDK candidate at ${VST3SDK_PATH} is incomplete:")
    foreach(_m IN LISTS _vst3sdk_missing)
      message(STATUS "  Missing: ${_m}")
    endforeach()
    set(VST3SDK_FOUND FALSE)
  else()
    set(VST3SDK_FOUND TRUE)
  endif()
endif()

find_package_handle_standard_args(
  VST3SDK
  REQUIRED_VARS VST3SDK_PATH VST3SDK_FOUND
  REASON_FAILURE_MESSAGE
    "Could not find a complete Steinberg VST3 SDK. Set VST3SDK_PATH to the SDK root containing base, pluginterfaces, public.sdk/source/vst/hosting."
)

if(VST3SDK_FOUND)
  if(NOT TARGET VST3::SDK)
    add_library(VST3::SDK INTERFACE IMPORTED)
    set_target_properties(VST3::SDK PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${VST3SDK_PATH}")
    message(STATUS "Found VST3 SDK: ${VST3SDK_PATH}")
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  VST3SDK
  PROPERTIES
    URL "https://www.steinberg.net/developers/"
    DESCRIPTION "The Steinberg VST3 SDK provides the headers and sources for hosting and developing VST3 plug-ins."
)
