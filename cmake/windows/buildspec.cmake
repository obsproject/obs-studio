# OBS CMake Windows build dependencies module

include_guard(GLOBAL)

include(buildspec_common)

# _check_dependencies_windows: Set up Windows slice for _check_dependencies
function(_check_dependencies_windows)
  set(dependencies_dir "${CMAKE_CURRENT_SOURCE_DIR}/.deps")
  set(prebuilt_filename "windows-deps-VERSION-ARCH-REVISION.zip")
  set(prebuilt_destination "obs-deps-VERSION-ARCH")
  set(qt6_filename "windows-deps-qt6-VERSION-ARCH-REVISION.zip")
  set(qt6_destination "obs-deps-qt6-VERSION-ARCH")
  set(cef_filename "cef_binary_VERSION_windows_ARCH_REVISION.zip")
  set(cef_destination "cef_binary_VERSION_windows_ARCH")

  if(CMAKE_GENERATOR_PLATFORM STREQUAL Win32)
    set(arch x86)
    set(dependencies_list prebuilt)
  else()
    string(TOLOWER "${CMAKE_GENERATOR_PLATFORM}" arch)
    set(dependencies_list prebuilt qt6 cef)
  endif()
  set(platform windows-${arch})

  _check_dependencies()
endfunction()

_check_dependencies_windows()
