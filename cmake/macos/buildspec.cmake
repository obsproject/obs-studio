# OBS CMake macOS build dependencies module

include_guard(GLOBAL)

include(buildspec_common)

# _check_dependencies_macos: Set up macOS slice for _check_dependencies
function(_check_dependencies_macos)
  set(arch universal)
  set(platform macos-${arch})

  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/buildspec.json" buildspec)

  set(dependencies_dir "${CMAKE_CURRENT_SOURCE_DIR}/.deps")
  set(prebuilt_filename "macos-deps-VERSION-ARCH-REVISION.tar.xz")
  set(prebuilt_destination "obs-deps-VERSION-ARCH")
  set(qt6_filename "macos-deps-qt6-VERSION-ARCH-REVISION.tar.xz")
  set(qt6_destination "obs-deps-qt6-VERSION-ARCH")
  set(cef_filename "cef_binary_VERSION_macos_ARCH_REVISION.tar.xz")
  set(cef_destination "cef_binary_VERSION_macos_ARCH")
  set(dependencies_list prebuilt qt6 cef)

  _check_dependencies()

  execute_process(
    COMMAND "xattr" -r -d com.apple.quarantine "${dependencies_dir}/${destination}"
    RESULT_VARIABLE result
    COMMAND_ERROR_IS_FATAL ANY
  )
endfunction()

_check_dependencies_macos()
