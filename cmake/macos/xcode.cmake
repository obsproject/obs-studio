# OBS CMake macOS Xcode module

# Use a compiler wrapper to enable ccache in Xcode projects
if(ENABLE_CCACHE AND CCACHE_PROGRAM)
  configure_file("${CMAKE_SOURCE_DIR}/cmake/macos/resources/ccache-launcher-c.in" ccache-launcher-c)
  configure_file("${CMAKE_SOURCE_DIR}/cmake/macos/resources/ccache-launcher-cxx.in" ccache-launcher-cxx)

  execute_process(COMMAND chmod a+rx "${CMAKE_BINARY_DIR}/ccache-launcher-c" "${CMAKE_BINARY_DIR}/ccache-launcher-cxx")
  set(CMAKE_XCODE_ATTRIBUTE_CC "${CMAKE_BINARY_DIR}/ccache-launcher-c")
  set(CMAKE_XCODE_ATTRIBUTE_CXX "${CMAKE_BINARY_DIR}/ccache-launcher-cxx")
  set(CMAKE_XCODE_ATTRIBUTE_LD "${CMAKE_C_COMPILER}")
  set(CMAKE_XCODE_ATTRIBUTE_LDPLUSPLUS "${CMAKE_CXX_COMPILER}")
endif()

# Set project variables
set(CMAKE_XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION ${OBS_BUILD_NUMBER})
set(CMAKE_XCODE_ATTRIBUTE_DYLIB_COMPATIBILITY_VERSION 1.0.0)
set(CMAKE_XCODE_ATTRIBUTE_MARKETING_VERSION ${OBS_VERSION_CANONICAL})

# Set deployment target
set(CMAKE_XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET ${CMAKE_OSX_DEPLOYMENT_TARGET})

if(NOT OBS_PROVISIONING_PROFILE)
  set(OBS_PROVISIONING_PROFILE
      ""
      CACHE STRING "OBS provisioning profile name for macOS" FORCE)
else()
  set(CMAKE_XCODE_ATTRIBUTE_PROVISIONING_PROFILE_NAME "${OBS_PROVISIONING_PROFILE}")
endif()

if(NOT OBS_CODESIGN_TEAM)
  # Switch to manual codesigning if no codesigning team is provided
  set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_STYLE Manual)
  set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${OBS_CODESIGN_IDENTITY}")
else()
  if(OBS_CODESIGN_IDENTITY AND NOT OBS_CODESIGN_IDENTITY STREQUAL "-")
    # Switch to manual codesigning if a non-adhoc codesigning identity is provided
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_STYLE Manual)
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${OBS_CODESIGN_IDENTITY}")
  else()
    # Switch to automatic codesigning via valid team ID
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_STYLE Automatic)
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Development")
  endif()
  set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${OBS_CODESIGN_TEAM}")
endif()

# Only create a single Xcode project file
set(CMAKE_XCODE_GENERATE_TOP_LEVEL_PROJECT_ONLY TRUE)
# Add all libraries to project link phase (lets Xcode handle linking)
set(CMAKE_XCODE_LINK_BUILD_PHASE_MODE KNOWN_LOCATION)

# Enable codesigning with secure timestamp when not in Debug configuration (required for Notarization)
set(CMAKE_XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS[variant=Release] "--timestamp")
set(CMAKE_XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS[variant=RelWithDebInfo] "--timestamp")
set(CMAKE_XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS[variant=MinSizeRel] "--timestamp")

# Enable codesigning with hardened runtime option when not in Debug configuration (required for Notarization)
set(CMAKE_XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME[variant=Release] YES)
set(CMAKE_XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME[variant=RelWithDebInfo] YES)
set(CMAKE_XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME[variant=MinSizeRel] YES)

# Disable injection of Xcode's base entitlements used for debugging when not in Debug configuration (required for
# Notarization)
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_INJECT_BASE_ENTITLEMENTS[variant=Release] NO)
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_INJECT_BASE_ENTITLEMENTS[variant=RelWithDebInfo] NO)
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_INJECT_BASE_ENTITLEMENTS[variant=MinSizeRel] NO)

# Use Swift version 5.0 by default
set(CMAKE_XCODE_ATTRIBUTE_SWIFT_VERSION 5.0)

# Use DWARF with separate dSYM files when in Release or MinSizeRel configuration
set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT[variant=Debug] dwarf)
set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT[variant=RelWithDebInfo] dwarf)
set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT[variant=Release] dwarf-with-dsym)
set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT[variant=MinSizeRel] dwarf-with-dsym)

# Strip generated and installed products only in Release or MinSizeRel configuration
set(CMAKE_XCODE_ATTRIBUTE_STRIP_INSTALLED_PRODUCT[variant=Release] YES)
set(CMAKE_XCODE_ATTRIBUTE_STRIP_INSTALLED_PRODUCT[variant=MinSizeRel] YES)

# Make all symbols hidden by default
set(CMAKE_XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN YES)
set(CMAKE_XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN YES)

# Strip unused code in Release or MinSizeRel configuration only
set(CMAKE_XCODE_ATTRIBUTE_DEAD_CODE_STRIPPING[variant=Release] YES)
set(CMAKE_XCODE_ATTRIBUTE_DEAD_CODE_STRIPPING[variant=MinSizeRel] YES)

# Display mangled names in Debug configuration
set(CMAKE_XCODE_ATTRIBUTE_LINKER_DISPLAYS_MANGLED_NAMES[variant=Debug] YES)

# Disable using ARC in ObjC by default (OBS does not support this - yet)
set(CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC NO)
# Disable strict aliasing
set(CMAKE_XCODE_ATTRIBUTE_GCC_STRICT_ALIASING NO)
# Set C language default to C17
set(CMAKE_XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD c17)
# Set C++ language default to c++17
set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD c++17)
# Enable support for module imports in ObjC
set(CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES NO)
# Enable automatic linking of imported modules in ObjC
set(CMAKE_XCODE_ATTRIBUTE_CLANG_MODULES_AUTOLINK NO)
# Enable strict msg_send rules for ObjC
set(CMAKE_XCODE_ATTRIBUTE_ENABLE_STRICT_OBJC_MSGSEND YES)

# Set default warnings for ObjC and C++
set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_BLOCK_CAPTURE_AUTORELEASING YES_ERROR)
set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_SUSPICIOUS_IMPLICIT_CONVERSION NO)
set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_DEPRECATED_OBJC_IMPLEMENTATIONS YES)
set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_OBJC_REPEATED_USE_OF_WEAK YES)
set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_RANGE_LOOP_ANALYSIS YES)
set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_EMPTY_BODY YES)

# Set default warnings for C and C++
set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_ABOUT_MISSING_FIELD_INITIALIZERS NO)
set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_ABOUT_MISSING_NEWLINE YES)
set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_ABOUT_RETURN_TYPE YES_ERROR)
set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_CHECK_SWITCH_STATEMENTS YES)
set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_FOUR_CHARACTER_CONSTANTS YES)
set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_SIGN_COMPARE YES)
set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_SHADOW NO)
set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_UNUSED_FUNCTION NO)
set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_UNUSED_PARAMETER YES)
set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_UNUSED_VALUE YES)
set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_UNUSED_VARIABLE YES)
set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_TYPECHECK_CALLS_TO_PRINTF YES)

# Add additional warning compiler flags
set(CMAKE_XCODE_ATTRIBUTE_WARNING_CFLAGS "-Wvla -Wformat-security -Wno-error=shorten-64-to-32")

set(CMAKE_XCODE_ATTRIBUTE_GCC_TREAT_WARNINGS_AS_ERRORS YES)

# Disable color diagnostics
set(CMAKE_COLOR_DIAGNOSTICS FALSE)

# Disable usage of RPATH in build or install configurations
set(CMAKE_SKIP_RPATH TRUE)
# Have Xcode set default RPATH entries
set(CMAKE_XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../Frameworks")
