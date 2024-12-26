# OBS CMake Linux CPack configuration module

include_guard(GLOBAL)

include(cpackconfig_common)

# Add GPLv2 license file to CPack
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/UI/data/license/gplv2.txt")
set(CPACK_PACKAGE_EXECUTABLES "obs")

if(ENABLE_RELEASE_BUILD)
  set(CPACK_PACKAGE_VERSION "${OBS_VERSION_CANONICAL}")
else()
  set(CPACK_PACKAGE_VERSION "${OBS_VERSION}")
endif()

set(CPACK_SOURCE_PACKAGE_FILE_NAME "obs-studio-${CPACK_PACKAGE_VERSION}-sources")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES "/.git" "${CMAKE_BINARY_DIR}" "/.ccache" "/.deps")
set(CPACK_ARCHIVE_THREADS 0)

if(OS_LINUX)
  set(CPACK_GENERATOR "DEB")
  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS TRUE)
  set(CPACK_SET_DESTDIR TRUE)
  set(CPACK_DEBIAN_DEBUGINFO_PACKAGE TRUE)
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${OBS_COMPANY_NAME}")
elseif(OS_FREEBSD)
  set(CPACK_GENERATOR "FREEBSD")

  set(CPACK_FREEBSD_PACKAGE_MAINTAINER "${OBS_COMPANY_NAME}")
  set(CPACK_FREEBSD_PACKAGE_LICENSE "GPLv2")

  set(
    CPACK_FREEBSD_PACKAGE_DEPS
    "audio/alsa-lib"
    "audio/fdk-aac"
    "audio/jack"
    "audio/pulseaudio"
    "audio/sndio"
    "devel/jansson"
    "devel/libpci"
    "devel/libsysinfo"
    "devel/nlohmann-json"
    "devel/qt6-base"
    "devel/qt6-svg"
    "devel/swig"
    "devel/websocketpp"
    "ftp/curl"
    "graphics/mesa-libs"
    "graphics/qr-code-generator"
    "lang/luajit"
    "lang/python39"
    "misc/e2fsprogs-libuuid"
    "multimedia/ffmpeg"
    "multimedia/librist"
    "multimedia/pipewire"
    "multimedia/v4l_compat"
    "multimedia/vlc"
    "net/asio"
    "www/libdatachannel"
    "www/srt"
  )
endif()

include(CPack)
