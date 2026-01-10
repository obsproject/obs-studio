cmake_minimum_required(VERSION 3.28...3.30)

option(ENABLE_VST3 "Enable building OBS with VST3 plugin" ON)

if(NOT ENABLE_VST3)
  target_disable(obs-vst3)
  return()
endif()

project(obs-vst3)

find_package(Qt6 REQUIRED Widgets)
set(CMAKE_AUTOMOC ON)

add_library(obs-vst3 MODULE VST3ComponentHolder.h VST3ComponentHolder.cpp)
add_library(OBS::vst3 ALIAS obs-vst3)

find_package(VST3SDK QUIET)

if(NOT VST3SDK_FOUND)
  message(STATUS "VST3 SDK not found — disabling obs-vst3 plugin.")
  target_disable(obs-vst3)
  return()
endif()

# SDK compile warnings
set(
  SDK_WARN_FLAGS
  -Wno-cast-align
  -Wno-conversion
  -Wno-cpp
  -Wno-delete-non-virtual-dtor
  -Wno-deprecated
  -Wno-deprecated-copy-dtor
  -Wno-deprecated-declarations
  -Wno-dangling-else
  -Wno-extra
  -Wno-extra-semi
  -Wno-float-equal
  -Wno-format
  -Wno-format-security
  -Wno-format-truncation
  -Wno-ignored-qualifiers
  -Wno-int-to-pointer-cast
  -Wno-missing-braces
  -Wno-missing-field-initializers
  -Wno-non-virtual-dtor
  -Wno-overloaded-virtual
  -Wno-parentheses
  -Wno-pedantic
  -Wno-redundant-decls
  -Wno-reorder
  -Wno-shadow
  -Wno-sign-compare
  -Wno-sign-conversion
  -Wno-switch-default
  -Wno-type-limits
  -Wno-unused-but-set-variable
  -Wno-unused-function
  -Wno-unused-parameter
  -Wno-zero-as-null-pointer-constant
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  list(APPEND SDK_WARN_FLAGS -Wno-class-memaccess -Wno-maybe-uninitialized)
endif()

list(JOIN SDK_WARN_FLAGS " " SDK_WARN_FLAGS_STR)

# SDK sources
set(VST3_SDK_SOURCES)
foreach(_src IN LISTS VST3SDK_REQUIRED_FILES)
  list(APPEND VST3_SDK_SOURCES "${VST3SDK_PATH}/${_src}")
endforeach()

# Main sources
set(
  VST3_MAIN_SOURCES
  plugin-main.cpp
  obs-vst3.cpp
  obs-vst3.h
  VST3ComponentHolder.cpp
  VST3ComponentHolder.h
  VST3HostApp.cpp
  VST3HostApp.h
  VST3Scanner.cpp
  VST3Scanner.h
  VST3Plugin.cpp
  VST3Plugin.h
  VST3EditorWindow.h
)

# Editor window sources
set(VST3EDITORWINDOW_SRC)
if(OS_WINDOWS)
  list(APPEND VST3EDITORWINDOW_SRC editor/win/VST3EditorWindow.cpp)
elseif(OS_MACOS)
  list(APPEND VST3EDITORWINDOW_SRC editor/mac/VST3EditorWindow.mm)
elseif(OS_LINUX)
  list(APPEND VST3EDITORWINDOW_SRC editor/linux/VST3EditorWindow.cpp editor/linux/RunLoopImpl.cpp)
endif()

# Add all sources
target_sources(obs-vst3 PRIVATE ${VST3_MAIN_SOURCES} ${VST3EDITORWINDOW_SRC} ${VST3_SDK_SOURCES})

# Disable warnings for SDK files
if(OS_LINUX OR OS_MACOS)
  set_source_files_properties(${VST3_SDK_SOURCES} PROPERTIES COMPILE_FLAGS "${SDK_WARN_FLAGS_STR}")
endif()

# macOS ARC for all .mm files (editor + sdk)
if(OS_MACOS)
  set_source_files_properties(
    editor/mac/VST3EditorWindow.mm
    "${VST3SDK_PATH}/public.sdk/source/vst/hosting/module_mac.mm"
    "${VST3SDK_PATH}/public.sdk/source/common/threadchecker_mac.mm"
    PROPERTIES COMPILE_OPTIONS "-fobjc-arc" COMPILE_FLAGS "${SDK_WARN_FLAGS_STR}"
  )
  target_link_libraries(obs-vst3 PRIVATE "-framework Cocoa" "-framework Foundation")
endif()

# Includes
target_include_directories(
  obs-vst3
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${VST3SDK_PATH}
    ${VST3SDK_PATH}/base
    ${VST3SDK_PATH}/pluginterfaces
    ${VST3SDK_PATH}/public.sdk/source/vst
    ${VST3SDK_PATH}/public.sdk/source/vst/hosting
    ${VST3SDK_PATH}/public.sdk/source/vst/utility
)

target_link_libraries(obs-vst3 PRIVATE OBS::libobs Qt6::Widgets)

# Windows resources
if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in obs-vst3.rc)
  target_sources(obs-vst3 PRIVATE obs-vst3.rc)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/windows/obs-studio.ico
    ${CMAKE_CURRENT_BINARY_DIR}/obs-studio.ico
    COPYONLY
  )
endif()

set_target_properties_obs(obs-vst3 PROPERTIES FOLDER plugins PREFIX "")
