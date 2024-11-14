cmake_minimum_required(VERSION 3.22...3.25)

if(CMAKE_HOST_SYSTEM_NAME MATCHES "(Darwin)" OR OBS_CMAKE_VERSION VERSION_GREATER_EQUAL 3.0.0)
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

  project(obs-studio VERSION ${OBS_VERSION_CANONICAL})

  if(CMAKE_HOST_SYSTEM_NAME MATCHES "(Windows)" AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/32bit/projects.cmake")
    return()
  endif()

  include(compilerconfig)
  include(defaults)
  include(helpers)

  option(ENABLE_UI "Enable building with UI (requires Qt)" ON)
  option(ENABLE_SCRIPTING "Enable scripting support" ON)
  option(ENABLE_HEVC "Enable HEVC encoders" ON)

  add_subdirectory(libobs)
  if(OS_WINDOWS)
    add_subdirectory(libobs-d3d11)
    add_subdirectory(libobs-winrt)
  endif()
  add_subdirectory(libobs-opengl)
  add_subdirectory(plugins)

  add_subdirectory(test/test-input)

  add_subdirectory(UI)

  message_configuration()
  return()
endif()

message(
  DEPRECATION
    "\n"
    "============ LEGACY BUILD SYSTEM IS DEPRECATED ============"
    "\n"
    "You are using the legacy build system to build OBS Studio. "
    "The legacy build system is unsupported and will be removed in the near future."
    "\n"
    "To migrate to the new build system, familiarize yourself with CMake presets "
    "(https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html) and create "
    "a user preset with your customized build settings, inheriting from one of the default presets."
    "\n"
    "============ LEGACY BUILD SYSTEM IS DEPRECATED ============")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/Modules")
include(VersionConfig)

# Prohibit in-source builds
if("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
  message(FATAL_ERROR "OBS: You cannot build in a source directory (or any directory with CMakeLists.txt file)."
                      "Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
endif()

project(obs-studio VERSION ${OBS_VERSION_CANONICAL})
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Use target folders for MSVC/Xcode/etc.
include(DeprecationHelpers)
include(ObsHelpers)
# Set default compiler flags
include(CompilerConfig)

# Allow selection of common build types via UI
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      "RelWithDebInfo"
      CACHE STRING "OBS build type [Release, RelWithDebInfo, Debug, MinSizeRel]" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Release RelWithDebInfo Debug MinSizeRel)
endif()

# Global project options
option(ENABLE_HEVC "Enable HEVC encoders" ON)
if(ENABLE_HEVC)
  add_compile_definitions(ENABLE_HEVC)
endif()
option(BUILD_FOR_DISTRIBUTION "Build for distribution (enables optimizations)" OFF)
option(ENABLE_UI "Enable building with UI (requires Qt)" ON)
if (NOT ENABLE_UI)
  if(MSVC)
    include(CopyMSVCBins)
  endif()
endif()
option(ENABLE_SCRIPTING "Enable scripting support" ON)
option(USE_LIBCXX "Use libc++ instead of libstdc++" ${APPLE})
option(BUILD_TESTS "Build test directory (includes test sources and possibly a platform test executable)" OFF)

if(OS_WINDOWS)
  option(INSTALLER_RUN
         "Build a multiarch installer (needs to run independently after both archs have compiled) (Windows)" OFF)

elseif(OS_POSIX)
  option(LINUX_PORTABLE "Build portable version (Linux)" OFF)
  option(USE_XDG "Utilize XDG Base Directory Specification (Linux)" ON)
  option(ENABLE_PULSEAUDIO "Enable PulseAudio support" ON)
  if(OS_LINUX)
    option(ENABLE_WAYLAND "Enable building with support for Wayland (Linux)" ON)
    option(BUILD_FOR_PPA "Build for PPA distribution" OFF)
  endif()
endif()

setup_obs_project()
mark_as_advanced(BUILD_TESTS USE_LIBCXX)

if(INSTALLER_RUN)
  generate_multiarch_installer()
  return()
endif()

# OBS sources and plugins
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  add_subdirectory(deps)
  add_subdirectory(libobs-opengl)
  if(OS_WINDOWS)
    add_subdirectory(libobs-d3d11)
    add_subdirectory(libobs-winrt)
  endif()
  add_subdirectory(libobs)
  add_subdirectory(plugins)
else()
  add_subdirectory(deps)
  add_subdirectory(libobs)
  add_subdirectory(plugins/win-dshow)
endif()

# OBS main app
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  add_subdirectory(UI)
endif()

# Tests
if(ENABLE_UNIT_TESTS)
  enable_testing()
endif()

if(BUILD_TESTS OR ENABLE_UNIT_TESTS)
  add_subdirectory(test)
endif()

if(WIN32)
	include(FetchContent)

	# Compare current linked libs with prev
	FetchContent_Declare(deps_checker URL "https://raw.githubusercontent.com/stream-labs/obs-studio-node/staging/dependency_checker/check_dependencies.cmd" DOWNLOAD_NO_EXTRACT true)
	FetchContent_MakeAvailable(deps_checker)

	add_custom_target(check_dependencies COMMAND ${CMAKE_COMMAND}
		"-Ddeps_checker_SOURCE_DIR=${deps_checker_SOURCE_DIR}"
		"-Ddeps_CONFIGURATION=$<CONFIGURATION>"
		"-Ddeps_CONFIG=$<CONFIG>"
		"-Ddeps_CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR}"

		-P "${CMAKE_SOURCE_DIR}/slobs_CI/check_libraries.cmake" )
endif()

#Configure 32-bit projects
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
  #message(FATAL_ERROR "CURL_INCLUDE_DIR: ${CURL_INCLUDE_DIR}, CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}, PREFIX_PATH_X86: ${PREFIX_PATH_X86}, OBS_VERSION: ${OBS_VERSION}, OBS_VERSION_OVERRIDE: ${OBS_VERSION_OVERRIDE}")
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    execute_process(
      COMMAND
        "${CMAKE_COMMAND}"
        -S ${CMAKE_CURRENT_SOURCE_DIR}
        -B ${CMAKE_SOURCE_DIR}/build_x86
        -G "${CMAKE_GENERATOR}" -A Win32
        -DCMAKE_SYSTEM_VERSION:STRING='${CMAKE_SYSTEM_VERSION}'
        -DCMAKE_INSTALL_PREFIX='${CMAKE_INSTALL_PREFIX}'
        -DVLCPath="${VLCPath}"
        -DCEF_ROOT_DIR="${CEF_ROOT_DIR}"
        -DUSE_UI_LOOP=${USE_UI_LOOP}
        -DENABLE_UI=${ENABLE_UI}
        -DCOPIED_DEPENDENCIES=${COPIED_DEPENDENCIES}
        -DCOPY_DEPENDENCIES=${COPY_DEPENDENCIES}
        -DENABLE_SCRIPTING=${ENABLE_SCRIPTING}
        -DGPU_PRIORITY_VAL=${GPU_PRIORITY_VAL}
        -DBUILD_CAPTIONS=${BUILD_CAPTIONS}
        -DCOMPILE_D3D12_HOOK=${COMPILE_D3D12_HOOK}
        -DENABLE_BROWSER=OFF
        -DCMAKE_PREFIX_PATH:PATH='${PREFIX_PATH_X86}'
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DBUILD_FOR_DISTRIBUTION=${BUILD_FOR_DISTRIBUTION}
        -DCURL_INCLUDE_DIR=${CURL_INCLUDE_DIR}
        -DENABLE_VLC=false
        -DVIRTUALCAM_GUID:STRING=${VIRTUALCAM_GUID}
        -DOBS_VERSION='${OBS_VERSION}'
        -DOBS_VERSION_OVERRIDE='${OBS_VERSION_OVERRIDE}'
        -DCMAKE_MESSAGE_LOG_LEVEL='Trace'
      RESULT_VARIABLE _process_result COMMAND_ERROR_IS_FATAL ANY)
  endif()
endif()
