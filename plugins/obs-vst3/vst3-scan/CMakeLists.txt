cmake_minimum_required(VERSION 3.28...3.30)

add_executable(obs-vst3-scanner)
add_executable(OBS::vst3-scanner ALIAS obs-vst3-scanner)

find_package(VST3SDK QUIET)
if(NOT VST3SDK_FOUND)
  message(STATUS "VST3 SDK not found or incomplete.")
  target_disable(obs-vst3-scanner)
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

set(VST3_SCANNER_SOURCES vst3-scanner.cpp VST3Scanner.cpp VST3Scanner.h)
target_sources(obs-vst3-scanner PRIVATE ${VST3_SCANNER_SOURCES} ${VST3_SDK_SOURCES})

# Disable warnings for SDK files
if(OS_LINUX OR OS_MACOS)
  set_source_files_properties(${VST3_SDK_SOURCES} PROPERTIES COMPILE_FLAGS "${SDK_WARN_FLAGS_STR}")
endif()

# macOS ARC for all .mm files from SDK
if(OS_MACOS)
  set_target_properties(obs-vst3-scanner PROPERTIES XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)
  set_target_properties(
    obs-vst3-scanner
    PROPERTIES XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER com.obsproject.obs-vst3-scanner
  )
  target_link_libraries(obs-vst3-scanner PRIVATE "-framework Cocoa" "-framework Foundation")
endif()

# Includes
target_include_directories(
  obs-vst3-scanner
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${VST3SDK_PATH}
    ${VST3SDK_PATH}/base
    ${VST3SDK_PATH}/pluginterfaces
    ${VST3SDK_PATH}/public.sdk/source/vst
    ${VST3SDK_PATH}/public.sdk/source/vst/hosting
    ${VST3SDK_PATH}/public.sdk/source/vst/utility
)

target_link_libraries(obs-vst3-scanner PRIVATE OBS::libobs)

set_target_properties_obs(obs-vst3-scanner PROPERTIES FOLDER plugins/obs-vst3)
