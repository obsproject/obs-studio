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

  set(webrtc_filename "webrtc-VERSION-win-ARCH.7z")
  set(webrtc_destination "webrtc_dist")

  set(libmediasoupclient_filename "libmediasoupclient-VERSION-win-ARCH.7z")
  set(libmediasoupclient_destination "libmediasoupclient_dist")

  set(grpc_filename "grpc-release-VERSION.7z")
  set(grpc_destination "grpc-release-VERSION")

  set(openssl_filename "openssl-VERSION-ARCH.7z")
  set(openssl_destination "openssl-VERSION-ARCH")

  if(CMAKE_GENERATOR_PLATFORM STREQUAL Win32)
    set(arch x86)
    set(dependencies_list prebuilt)
  else()
    set(arch ${CMAKE_GENERATOR_PLATFORM})
    set(dependencies_list prebuilt qt6 cef webrtc libmediasoupclient grpc openssl)
  endif()
  set(platform windows-${arch})

  _check_dependencies()
endfunction()

_check_dependencies_windows()
