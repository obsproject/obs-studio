if(POLICY CMP0087)
  cmake_policy(SET CMP0087 NEW)
endif()

set(OBS_STANDALONE_PLUGIN_DIR ${CMAKE_SOURCE_DIR}/release)

include(GNUInstallDirs)
if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
  set(OS_MACOS ON)
  set(OS_POSIX ON)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux|FreeBSD|OpenBSD")
  set(OS_POSIX ON)
  string(TOUPPER "${CMAKE_SYSTEM_NAME}" _SYSTEM_NAME_U)
  set(OS_${_SYSTEM_NAME_U} ON)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  set(OS_WINDOWS ON)
  set(OS_POSIX OFF)
endif()

# Old-Style plugin detected, find "modern" libobs variant instead and set global include directories to fix "bad" plugin
# behavior
if(DEFINED LIBOBS_INCLUDE_DIR AND NOT TARGET OBS::libobs)
  message(
    DEPRECATION
      "You are using an outdated method of adding 'libobs' to your project. Refer to the updated wiki on how to build and export 'libobs' and use it in your plugin projects."
  )
  find_package(libobs REQUIRED)
  if(TARGET OBS::libobs)
    set_target_properties(OBS::libobs PROPERTIES IMPORTED_GLOBAL TRUE)
    message(STATUS "OBS: Using modern libobs target")

    add_library(libobs ALIAS OBS::libobs)
    if(OS_WINDOWS)
      add_library(w32-pthreads ALIAS OBS::w32-pthreads)
    endif()
  endif()
endif()

# Set macOS and Windows specific if default value is used
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT AND (OS_WINDOWS OR OS_MACOS))
  set(CMAKE_INSTALL_PREFIX
      ${OBS_STANDALONE_PLUGIN_DIR}
      CACHE STRING "Directory to install OBS plugin after building" FORCE)
endif()

# Set default build type to RelWithDebInfo and specify allowed alternative values
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      "RelWithDebInfo"
      CACHE STRING "OBS build type [Release, RelWithDebInfo, Debug, MinSizeRel]" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Release RelWithDebInfo Debug MinSizeRel)
endif()

# Set default Qt version to AUTO, preferring an available Qt6 with a fallback to Qt5
if(NOT QT_VERSION)
  set(QT_VERSION
      AUTO
      CACHE STRING "OBS Qt version [AUTO, 6, 5]" FORCE)
  set_property(CACHE QT_VERSION PROPERTY STRINGS AUTO 6 5)
endif()

# Macro to find best possible Qt version for use with the project:
#
# * Use QT_VERSION value as a hint for desired Qt version
# * If "AUTO" was specified, prefer Qt6 over Qt5
# * Creates versionless targets of desired component if none had been created by Qt itself (Qt versions < 5.15)
#
macro(find_qt)
  set(multiValueArgs COMPONENTS COMPONENTS_WIN COMPONENTS_MAC COMPONENTS_LINUX)
  cmake_parse_arguments(FIND_QT "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  # Do not use versionless targets in the first step to avoid Qt::Core being clobbered by later opportunistic
  # find_package runs
  set(QT_NO_CREATE_VERSIONLESS_TARGETS ON)

  # Loop until _QT_VERSION is set (or FATAL_ERROR aborts script execution early)
  while(NOT _QT_VERSION)
    if(QT_VERSION STREQUAL AUTO AND NOT _QT_TEST_VERSION)
      set(_QT_TEST_VERSION 6)
    elseif(NOT QT_VERSION STREQUAL AUTO)
      set(_QT_TEST_VERSION ${QT_VERSION})
    endif()

    find_package(
      Qt${_QT_TEST_VERSION}
      COMPONENTS Core
      QUIET)

    if(TARGET Qt${_QT_TEST_VERSION}::Core)
      set(_QT_VERSION
          ${_QT_TEST_VERSION}
          CACHE INTERNAL "")
      message(STATUS "Qt version found: ${_QT_VERSION}")
      unset(_QT_TEST_VERSION)
      break()
    elseif(QT_VERSION STREQUAL AUTO)
      if(_QT_TEST_VERSION EQUAL 6)
        message(WARNING "Qt6 was not found, falling back to Qt5")
        set(_QT_TEST_VERSION 5)
        continue()
      endif()
    endif()
    message(FATAL_ERROR "Neither Qt6 nor Qt5 found.")
  endwhile()

  # Enable versionless targets for the remaining Qt components
  set(QT_NO_CREATE_VERSIONLESS_TARGETS OFF)

  set(_QT_COMPONENTS ${FIND_QT_COMPONENTS})
  if(OS_WINDOWS)
    list(APPEND _QT_COMPONENTS ${FIND_QT_COMPONENTS_WIN})
  elseif(OS_MACOS)
    list(APPEND _QT_COMPONENTS ${FIND_QT_COMPONENTS_MAC})
  else()
    list(APPEND _QT_COMPONENTS ${FIND_QT_COMPONENTS_LINUX})
  endif()

  find_package(
    Qt${_QT_VERSION}
    COMPONENTS ${_QT_COMPONENTS}
    REQUIRED)

  list(APPEND _QT_COMPONENTS Core)

  if("Gui" IN_LIST FIND_QT_COMPONENTS_LINUX)
    list(APPEND _QT_COMPONENTS "GuiPrivate")
  endif()

  # Check for versionless targets of each requested component and create if necessary
  foreach(_COMPONENT IN LISTS _QT_COMPONENTS)
    if(NOT TARGET Qt::${_COMPONENT} AND TARGET Qt${_QT_VERSION}::${_COMPONENT})
      add_library(Qt::${_COMPONENT} INTERFACE IMPORTED)
      set_target_properties(Qt::${_COMPONENT} PROPERTIES INTERFACE_LINK_LIBRARIES Qt${_QT_VERSION}::${_COMPONENT})
    endif()
  endforeach()
endmacro()

# Set relative path variables for file configurations
file(RELATIVE_PATH RELATIVE_INSTALL_PATH ${CMAKE_SOURCE_DIR} ${CMAKE_INSTALL_PREFIX})
file(RELATIVE_PATH RELATIVE_BUILD_PATH ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})

if(OS_POSIX)
  # Set default GCC/clang compile options:
  #
  # * Treat warnings as errors
  # * Enable extra warnings, https://clang.llvm.org/docs/DiagnosticsReference.html#wextra
  # * Warning about usage of variable length array, https://clang.llvm.org/docs/DiagnosticsReference.html#wvla
  # * Warning about bad format specifiers, https://clang.llvm.org/docs/DiagnosticsReference.html#wformat
  # * Warning about non-strings used as format strings,
  #   https://clang.llvm.org/docs/DiagnosticsReference.html#wformat-security
  # * Warning about non-exhaustive switch blocks, https://clang.llvm.org/docs/DiagnosticsReference.html#wswitch
  # * Warning about unused parameters, https://clang.llvm.org/docs/DiagnosticsReference.html#wunused-parameter
  # * DISABLE warning about unused functions, https://clang.llvm.org/docs/DiagnosticsReference.html#wunused-function
  # * DISABLE warning about missing field initializers,
  #   https://clang.llvm.org/docs/DiagnosticsReference.html#wmissing-field-initializers
  # * DISABLE strict aliasing optimisations
  # * C ONLY - treat implicit function declarations (use before declare) as errors,
  #   https://clang.llvm.org/docs/DiagnosticsReference.html#wimplicit-function-declaration
  # * C ONLY - DISABLE warning about missing braces around subobject initalizers,
  #   https://clang.llvm.org/docs/DiagnosticsReference.html#wmissing-braces
  # * C ONLY, Clang ONLY - Warning about implicit conversion of NULL to another type,
  #   https://clang.llvm.org/docs/DiagnosticsReference.html#wnull-conversion
  # * C & C++, Clang ONLY - Disable warning about integer conversion losing precision,
  #   https://clang.llvm.org/docs/DiagnosticsReference.html#wshorten-64-to-32
  # * C++, GCC ONLY - Warning about implicit conversion of NULL to another type
  # * Enable color diagnostics on Clang (CMAKE_COLOR_DIAGNOSTICS available in CMake 3.24)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
      -Werror
      -Wextra
      -Wvla
      -Wformat
      -Wformat-security
      -Wswitch
      -Wunused-parameter
      -Wno-unused-function
      -Wno-missing-field-initializers
      -fno-strict-aliasing
      "$<$<COMPILE_LANGUAGE:C>:-Werror-implicit-function-declaration;-Wno-missing-braces>"
      "$<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang>:-Wnull-conversion;-Wno-error=shorten-64-to-32;-fcolor-diagnostics>"
      "$<$<COMPILE_LANG_AND_ID:CXX,AppleClang,Clang>:-Wnull-conversion;-Wno-error=shorten-64-to-32;-fcolor-diagnostics>"
      "$<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wconversion-null>"
      "$<$<CONFIG:DEBUG>:-DDEBUG=1;-D_DEBUG=1>")

  # GCC 12.1.0 has a regression bug which trigger maybe-uninitialized warnings where there is not.
  # (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105562)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL "12.1.0")
    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wno-error=maybe-uninitialized)
  endif()

  if(NOT CCACHE_SET)
    # Try to find and enable ccache
    find_program(CCACHE_PROGRAM "ccache")
    set(CCACHE_SUPPORT
        ON
        CACHE BOOL "Enable ccache support")
    mark_as_advanced(CCACHE_PROGRAM)
    if(CCACHE_PROGRAM AND CCACHE_SUPPORT)
      set(CMAKE_CXX_COMPILER_LAUNCHER
          ${CCACHE_PROGRAM}
          CACHE INTERNAL "")
      set(CMAKE_C_COMPILER_LAUNCHER
          ${CCACHE_PROGRAM}
          CACHE INTERNAL "")
      set(CMAKE_OBJC_COMPILER_LAUNCHER
          ${CCACHE_PROGRAM}
          CACHE INTERNAL "")
      set(CMAKE_OBJCXX_COMPILER_LAUNCHER
          ${CCACHE_PROGRAM}
          CACHE INTERNAL "")
      set(CMAKE_CUDA_COMPILER_LAUNCHER
          ${CCACHE_PROGRAM}
          CACHE INTERNAL "") # CMake 3.9+
      set(CCACHE_SET
          ON
          CACHE INTERNAL "")
    endif()
  endif()
endif()

# Set required C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Get lowercase host architecture for easier comparison
if(MSVC_CXX_ARCHITECTURE_ID)
  string(TOLOWER ${MSVC_CXX_ARCHITECTURE_ID} _HOST_ARCH)
else()
  string(TOLOWER ${CMAKE_SYSTEM_PROCESSOR} _HOST_ARCH)
endif()

if(_HOST_ARCH MATCHES "i[3-6]86|x86|x64|x86_64|amd64" AND NOT CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
  # Enable MMX, SSE and SSE2 on compatible host systems (assuming no cross-compile)
  set(ARCH_SIMD_FLAGS -mmmx -msse -msse2)
elseif(_HOST_ARCH MATCHES "arm64|arm64e|aarch64")
  # Enable available built-in SIMD support in Clang and GCC
  if(CMAKE_C_COMPILER_ID MATCHES "^(Apple)?Clang|GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "^(Apple)?Clang|GNU")
    include(CheckCCompilerFlag)
    include(CheckCXXCompilerFlag)

    check_c_compiler_flag("-fopenmp-simd" C_COMPILER_SUPPORTS_OPENMP_SIMD)
    check_cxx_compiler_flag("-fopenmp-simd" CXX_COMPILER_SUPPORTS_OPENMP_SIMD)
    target_compile_options(
      ${CMAKE_PROJECT_NAME}
      PRIVATE -DSIMDE_ENABLE_OPENMP
              "$<$<AND:$<COMPILE_LANGUAGE:C>,$<BOOL:C_COMPILER_SUPPORTS_OPENMP_SIMD>>:-fopenmp-simd>"
              "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<BOOL:CXX_COMPILER_SUPPORTS_OPENMP_SIMD>>:-fopenmp-simd>")
  endif()
endif()

# macOS specific settings
if(OS_MACOS)
  # Set macOS-specific C++ standard library
  target_compile_options(${CMAKE_PROJECT_NAME}
                         PRIVATE "$<$<COMPILE_LANG_AND_ID:OBJC,AppleClang,Clang>:-fcolor-diagnostics>" -stdlib=libc++)

  # Set build architecture to host architecture by default
  if(NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES
        ${CMAKE_HOST_SYSTEM_PROCESSOR}
        CACHE STRING "Build architecture for macOS" FORCE)
  endif()
  set_property(CACHE CMAKE_OSX_ARCHITECTURES PROPERTY STRINGS arm64 x86_64 "arm64;x86_64")

  # Set deployment target to 11.0 for Apple Silicon or 10.15 for Intel and Universal builds
  if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET[arch=arm64] "11.0")
    set(CMAKE_XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET[arch=x86_64] "10.15")

    if("${CMAKE_OSX_ARCHITECTURES}" STREQUAL "arm64")
      set(_MACOS_DEPLOYMENT_TARGET "11.0")
    else()
      set(_MACOS_DEPLOYMENT_TARGET "10.15")
    endif()

    set(CMAKE_OSX_DEPLOYMENT_TARGET
        ${_MACOS_DEPLOYMENT_TARGET}
        CACHE STRING "Minimum macOS version to target for deployment (at runtime); newer APIs weak linked" FORCE)
    unset(_MACOS_DEPLOYMENT_TARGET)
  endif()

  set_property(CACHE CMAKE_OSX_DEPLOYMENT_TARGET PROPERTY STRINGS 13.0 12.0 11.0 10.15)

  # Override macOS install directory
  if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX
        ${CMAKE_BINARY_DIR}/install
        CACHE STRING "Directory to install OBS to after building" FORCE)
  endif()

  # Set up codesigning for Xcode builds with team IDs or standalone builds with developer identity
  if(NOT OBS_BUNDLE_CODESIGN_TEAM)
    if(NOT OBS_BUNDLE_CODESIGN_IDENTITY)
      set(OBS_BUNDLE_CODESIGN_IDENTITY
          "-"
          CACHE STRING "OBS code signing identity for macOS" FORCE)
    endif()
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ${OBS_BUNDLE_CODESIGN_IDENTITY})
  else()
    # Team ID specified, warn if Xcode generator is not used and fall back to ad-hoc signing
    if(NOT XCODE)
      message(
        WARNING
          "Code signing with a team identifier is only supported with the Xcode generator. Using ad-hoc code signature instead."
      )
      if(NOT OBS_BUNDLE_CODESIGN_IDENTITY)
        set(OBS_BUNDLE_CODESIGN_IDENTITY
            "-"
            CACHE STRING "OBS code signing identity for macOS" FORCE)
      endif()
    else()
      unset(OBS_BUNDLE_CODESIGN_IDENTITY)
      set_property(CACHE OBS_BUNDLE_CODESIGN_TEAM PROPERTY HELPSTRING "OBS code signing team for macOS")
      set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_STYLE Automatic)
      set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ${OBS_BUNDLE_CODESIGN_TEAM})
    endif()
  endif()

  # Set path to entitlements property list for codesigning. Entitlements should match the host binary, in this case
  # OBS.app.
  set(OBS_CODESIGN_ENTITLEMENTS
      ${CMAKE_SOURCE_DIR}/cmake/bundle/macos/entitlements.plist
      CACHE INTERNAL "Path to codesign entitlements plist")
  # Enable linker codesigning by default. Building OBS or plugins on host systems older than macOS 10.15 is not
  # supported
  set(OBS_CODESIGN_LINKER
      ON
      CACHE BOOL "Enable linker codesigning on macOS (macOS 11+ required)")

  # Tell Xcode to pretend the linker signed binaries so that editing with install_name_tool preserves ad-hoc signatures.
  # This option is supported by codesign on macOS 11 or higher. See CMake Issue 21854:
  # https://gitlab.kitware.com/cmake/cmake/-/issues/21854
  if(OBS_CODESIGN_LINKER)
    set(CMAKE_XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "-o linker-signed")
  endif()

  # Set default options for bundling on macOS
  set(CMAKE_MACOSX_RPATH ON)
  set(CMAKE_SKIP_BUILD_RPATH OFF)
  set(CMAKE_BUILD_WITH_INSTALL_RPATH OFF)
  set(CMAKE_INSTALL_RPATH "@executable_path/../Frameworks/")
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH OFF)

  # Helper function for plugin targets (macOS version)
  function(setup_plugin_target target)
    # Sanity check for required bundle information
    #
    # * Bundle identifier
    # * Bundle version
    # * Short version string
    if(NOT DEFINED MACOSX_PLUGIN_GUI_IDENTIFIER)
      message(
        FATAL_ERROR
          "No 'MACOSX_PLUGIN_GUI_IDENTIFIER' set, but is required to build plugin bundles on macOS - example: 'com.yourname.pluginname'"
      )
    endif()

    if(NOT DEFINED MACOSX_PLUGIN_BUNDLE_VERSION)
      message(
        FATAL_ERROR
          "No 'MACOSX_PLUGIN_BUNDLE_VERSION' set, but is required to build plugin bundles on macOS - example: '25'")
    endif()

    if(NOT DEFINED MACOSX_PLUGIN_SHORT_VERSION_STRING)
      message(
        FATAL_ERROR
          "No 'MACOSX_PLUGIN_SHORT_VERSION_STRING' set, but is required to build plugin bundles on macOS - example: '1.0.2'"
      )
    endif()

    # Set variables for automatic property list generation
    set(MACOSX_PLUGIN_BUNDLE_NAME
        "${target}"
        PARENT_SCOPE)
    set(MACOSX_PLUGIN_BUNDLE_VERSION
        "${MACOSX_PLUGIN_BUNDLE_VERSION}"
        PARENT_SCOPE)
    set(MACOSX_PLUGIN_SHORT_VERSION_STRING
        "${MACOSX_PLUGIN_SHORT_VERSION_STRING}"
        PARENT_SCOPE)
    set(MACOSX_PLUGIN_EXECUTABLE_NAME
        "${target}"
        PARENT_SCOPE)
    set(MACOSX_PLUGIN_BUNDLE_TYPE
        "BNDL"
        PARENT_SCOPE)

    # Set installation target to install prefix root (default for bundles)
    install(
      TARGETS ${target}
      LIBRARY DESTINATION "."
              COMPONENT obs_plugins
              NAMELINK_COMPONENT ${target}_Development)

    if(TARGET Qt::Core)
      # Framework version has changed between Qt5 (uses wrong numerical version) and Qt6 (uses correct alphabetical
      # version)
      if(${_QT_VERSION} EQUAL 5)
        set(_QT_FW_VERSION "${QT_VERSION}")
      else()
        set(_QT_FW_VERSION "A")
      endif()

      # Set up install-time command to fix Qt library references to point into OBS.app bundle
      set(_COMMAND
          "${CMAKE_INSTALL_NAME_TOOL} \\
        -change ${CMAKE_PREFIX_PATH}/lib/QtWidgets.framework/Versions/${QT_VERSION}/QtWidgets @rpath/QtWidgets.framework/Versions/${_QT_FW_VERSION}/QtWidgets \\
        -change ${CMAKE_PREFIX_PATH}/lib/QtCore.framework/Versions/${QT_VERSION}/QtCore @rpath/QtCore.framework/Versions/${_QT_FW_VERSION}/QtCore \\
        -change ${CMAKE_PREFIX_PATH}/lib/QtGui.framework/Versions/${QT_VERSION}/QtGui @rpath/QtGui.framework/Versions/${_QT_FW_VERSION}/QtGui \\
        \\\"\${CMAKE_INSTALL_PREFIX}/${target}.plugin/Contents/MacOS/${target}\\\"")
      install(CODE "execute_process(COMMAND /bin/sh -c \"${_COMMAND}\")" COMPONENT obs_plugins)
      unset(_QT_FW_VERSION)
    endif()

    # Set macOS bundle properties
    set_target_properties(
      ${target}
      PROPERTIES PREFIX ""
                 BUNDLE ON
                 BUNDLE_EXTENSION "plugin"
                 OUTPUT_NAME ${target}
                 MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/bundle/macOS/Plugin-Info.plist.in"
                 XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${MACOSX_PLUGIN_GUI_IDENTIFIER}"
                 XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS
                 "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/bundle/macOS/entitlements.plist")

    # If not building with Xcode, manually code-sign the plugin
    if(NOT XCODE)
      set(_COMMAND
          "/usr/bin/codesign --force \\
          --sign \\\"${OBS_BUNDLE_CODESIGN_IDENTITY}\\\" \\
          --options runtime \\
          --entitlements \\\"${CMAKE_CURRENT_FUNCTION_LIST_DIR}/bundle/macOS/entitlements.plist\\\" \\
          \\\"\${CMAKE_INSTALL_PREFIX}/${target}.plugin\\\"")
      install(CODE "execute_process(COMMAND /bin/sh -c \"${_COMMAND}\")" COMPONENT obs_plugins)
    endif()

    install_bundle_resources(${target})
  endfunction()

  # Helper function to add resources from "data" directory as bundle resources
  function(install_bundle_resources target)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/data)
      file(GLOB_RECURSE _DATA_FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/*")
      foreach(_DATA_FILE IN LISTS _DATA_FILES)
        file(RELATIVE_PATH _RELATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/data/ ${_DATA_FILE})
        get_filename_component(_RELATIVE_PATH ${_RELATIVE_PATH} PATH)
        target_sources(${target} PRIVATE ${_DATA_FILE})
        set_source_files_properties(${_DATA_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/${_RELATIVE_PATH})
        string(REPLACE "\\" "\\\\" _GROUP_NAME ${_RELATIVE_PATH})
        source_group("Resources\\${_GROUP_NAME}" FILES ${_DATA_FILE})
      endforeach()
    endif()
  endfunction()
else()
  # Check for target architecture (64bit vs 32bit)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_ARCH_SUFFIX 64)
  else()
    set(_ARCH_SUFFIX 32)
  endif()
  set(OBS_OUTPUT_DIR ${CMAKE_BINARY_DIR}/rundir)

  # Unix specific settings
  if(OS_POSIX)
    # Paths to binaries and plugins differ between portable and non-portable builds on Linux
    option(LINUX_PORTABLE "Build portable version (Linux)" ON)
    if(NOT LINUX_PORTABLE)
      set(OBS_LIBRARY_DESTINATION ${CMAKE_INSTALL_LIBDIR})
      set(OBS_PLUGIN_DESTINATION ${OBS_LIBRARY_DESTINATION}/obs-plugins)
      set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)
      set(OBS_DATA_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/obs)
    else()
      set(OBS_LIBRARY_DESTINATION bin/${_ARCH_SUFFIX}bit)
      set(OBS_PLUGIN_DESTINATION obs-plugins/${_ARCH_SUFFIX}bit)
      set(CMAKE_INSTALL_RPATH "$ORIGIN/" "${CMAKE_INSTALL_PREFIX}/${OBS_LIBRARY_DESTINATION}")
      set(OBS_DATA_DESTINATION "data")
    endif()

    # Setup Linux-specific CPack values for "deb" package generation
    if(OS_LINUX)
      set(CPACK_PACKAGE_NAME "${CMAKE_PROJECT_NAME}")
      set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${LINUX_MAINTAINER_EMAIL}")
      set(CPACK_PACKAGE_VERSION "${CMAKE_PROJECT_VERSION}")
      set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-linux-x86_64")

      set(CPACK_GENERATOR "DEB")
      set(CPACK_DEBIAN_PACKAGE_DEPENDS
          "obs-studio (>= 27.0.0), libqt5core5a (>= 5.9.0~beta), libqt5gui5 (>= 5.3.0), libqt5widgets5 (>= 5.7.0)")

      set(CPACK_OUTPUT_FILE_PREFIX ${CMAKE_SOURCE_DIR}/release)

      if(NOT LINUX_PORTABLE)
        set(CPACK_SET_DESTDIR ON)
      endif()
      include(CPack)
    endif()
    # Windows specific settings
  else()
    set(OBS_LIBRARY_DESTINATION "bin/${_ARCH_SUFFIX}bit")
    set(OBS_LIBRARY32_DESTINATION "bin/32bit")
    set(OBS_LIBRARY64_DESTINATION "bin/64bit")
    set(OBS_PLUGIN_DESTINATION "obs-plugins/${_ARCH_SUFFIX}bit")
    set(OBS_PLUGIN32_DESTINATION "obs-plugins/32bit")
    set(OBS_PLUGIN64_DESTINATION "obs-plugins/64bit")

    set(OBS_DATA_DESTINATION "data")

    if(MSVC)
      # Set default Visual Studio CL.exe compile options.
      #
      # * Enable building with multiple processes,
      #   https://docs.microsoft.com/en-us/cpp/build/reference/mp-build-with-multiple-processes?view=msvc-170
      # * Enable lint-like warnings,
      #   https://docs.microsoft.com/en-us/cpp/build/reference/compiler-option-warning-level?view=msvc-170
      # * Enable treating all warnings as errors,
      #   https://docs.microsoft.com/en-us/cpp/build/reference/compiler-option-warning-level?view=msvc-170
      # * RelWithDebInfo ONLY - Enable expanding of all functions not explicitly marked for no inlining,
      #   https://docs.microsoft.com/en-us/cpp/build/reference/ob-inline-function-expansion?view=msvc-170
      # * Enable UNICODE support,
      #   https://docs.microsoft.com/en-us/windows/win32/learnwin32/working-with-strings#unicode-and-ansi-functions
      # * DISABLE warnings about using POSIX function names,
      #   https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-3-c4996?view=msvc-170#posix-function-names
      # * DISABLE warnings about unsafe CRT library functions,
      #   https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-3-c4996?view=msvc-170#unsafe-crt-library-functions
      # * DISABLE warnings about nonstandard nameless structs/unions,
      #   https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4201?view=msvc-170
      target_compile_options(
        ${CMAKE_PROJECT_NAME}
        PRIVATE /MP
                /W3
                /WX
                /wd4201
                "$<$<CONFIG:RELWITHDEBINFO>:/Ob2>"
                "$<$<CONFIG:DEBUG>:/DDEBUG=1;/D_DEBUG=1>"
                /DUNICODE
                /D_UNICODE
                /D_CRT_SECURE_NO_WARNINGS
                /D_CRT_NONSTDC_NO_WARNINGS)

      # Set default Visual Studio linker options.
      #
      # * Enable removal of functions and data that are never used,
      #   https://docs.microsoft.com/en-us/cpp/build/reference/opt-optimizations?view=msvc-170
      # * Enable treating all warnings as errors,
      #   https://docs.microsoft.com/en-us/cpp/build/reference/wx-treat-linker-warnings-as-errors?view=msvc-170
      # * x64 ONLY - DISABLE creation of table of safe exception handlers,
      #   https://docs.microsoft.com/en-us/cpp/build/reference/safeseh-image-has-safe-exception-handlers?view=msvc-170
      # * Debug ONLY - DISABLE incremental linking,
      #   https://docs.microsoft.com/en-us/cpp/build/reference/incremental-link-incrementally?view=msvc-170
      # * RelWithDebInfo ONLY - Disable incremental linking, but enable COMDAT folding,
      #   https://docs.microsoft.com/en-us/cpp/build/reference/opt-optimizations?view=msvc-170
      target_link_options(
        ${CMAKE_PROJECT_NAME}
        PRIVATE
        "LINKER:/OPT:REF"
        "LINKER:/WX"
        "$<$<NOT:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>>:LINKER\:/SAFESEH\:NO>"
        "$<$<CONFIG:DEBUG>:LINKER\:/INCREMENTAL\:NO>"
        "$<$<CONFIG:RELWITHDEBINFO>:LINKER\:/INCREMENTAL\:NO;/OPT\:ICF>")
    endif()
  endif()

  # Helper function for plugin targets (Windows and Linux version)
  function(setup_plugin_target target)
    # Set prefix to empty string to avoid automatic naming of generated library, i.e. "lib<YOUR_PLUGIN_NAME>"
    set_target_properties(${target} PROPERTIES PREFIX "")

    # Set install directories
    install(
      TARGETS ${target}
      RUNTIME DESTINATION "${OBS_PLUGIN_DESTINATION}" COMPONENT ${target}_Runtime
      LIBRARY DESTINATION "${OBS_PLUGIN_DESTINATION}"
              COMPONENT ${target}_Runtime
              NAMELINK_COMPONENT ${target}_Development)

    # Set rundir install directory
    install(
      FILES $<TARGET_FILE:${target}>
      DESTINATION $<CONFIG>/${OBS_PLUGIN_DESTINATION}
      COMPONENT obs_rundir
      EXCLUDE_FROM_ALL)

    if(OS_WINDOWS)
      # Set install directory for optional PDB symbol files
      install(
        FILES $<TARGET_PDB_FILE:${target}>
        CONFIGURATIONS "RelWithDebInfo" "Debug"
        DESTINATION ${OBS_PLUGIN_DESTINATION}
        COMPONENT ${target}_Runtime
        OPTIONAL)

      # Set rundir install directory for optional PDB symbol files
      install(
        FILES $<TARGET_PDB_FILE:${target}>
        CONFIGURATIONS "RelWithDebInfo" "Debug"
        DESTINATION $<CONFIG>/${OBS_PLUGIN_DESTINATION}
        COMPONENT obs_rundir
        OPTIONAL EXCLUDE_FROM_ALL)
    endif()

    # Add resources from data directory
    setup_target_resources(${target} obs-plugins/${target})

    # Set up plugin for testing in available OBS build on Windows
    if(OS_WINDOWS AND DEFINED OBS_BUILD_DIR)
      setup_target_for_testing(${target} obs-plugins/${target})
    endif()

    # Custom command to install generated plugin into rundir
    add_custom_command(
      TARGET ${target}
      POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -DCMAKE_INSTALL_PREFIX=${OBS_OUTPUT_DIR} -DCMAKE_INSTALL_COMPONENT=obs_rundir
              -DCMAKE_INSTALL_CONFIG_NAME=$<CONFIG> -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_install.cmake
      COMMENT "Installing to plugin rundir"
      VERBATIM)
  endfunction()

  # Helper function to add resources from "data" directory
  function(setup_target_resources target destination)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/data)
      install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/
        DESTINATION ${OBS_DATA_DESTINATION}/${destination}
        USE_SOURCE_PERMISSIONS
        COMPONENT obs_plugins)

      install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data
        DESTINATION $<CONFIG>/${OBS_DATA_DESTINATION}/${destination}
        USE_SOURCE_PERMISSIONS
        COMPONENT obs_rundir
        EXCLUDE_FROM_ALL)
    endif()
  endfunction()

  if(OS_WINDOWS)
    # Additional Windows-only helper function to copy plugin to existing OBS development directory:
    #
    # Copies plugin with associated PDB symbol files as well as contents of data directory into the OBS rundir as
    # specified by "OBS_BUILD_DIR".
    function(setup_target_for_testing target destination)
      install(
        FILES $<TARGET_FILE:${target}>
        DESTINATION $<CONFIG>/${OBS_PLUGIN_DESTINATION}
        COMPONENT obs_testing
        EXCLUDE_FROM_ALL)

      install(
        FILES $<TARGET_PDB_FILE:${target}>
        CONFIGURATIONS "RelWithDebInfo" "Debug"
        DESTINATION $<CONFIG>/${OBS_PLUGIN_DESTINATION}
        COMPONENT obs_testing
        OPTIONAL EXCLUDE_FROM_ALL)

      install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/
        DESTINATION $<CONFIG>/${OBS_DATA_DESTINATION}/${destination}
        USE_SOURCE_PERMISSIONS
        COMPONENT obs_testing
        EXCLUDE_FROM_ALL)

      add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -DCMAKE_INSTALL_PREFIX=${OBS_BUILD_DIR}/rundir -DCMAKE_INSTALL_COMPONENT=obs_testing
                -DCMAKE_INSTALL_CONFIG_NAME=$<CONFIG> -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_install.cmake
        COMMENT "Installing to OBS test directory"
        VERBATIM)
    endfunction()
  endif()
endif()
