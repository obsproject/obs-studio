# Doesn't really make sense anywhere else
if(NOT MSVC)
  return()
endif()

# Internal variable to avoid copying more than once
if(COPIED_DEPENDENCIES)
  return()
endif()

option(COPY_DEPENDENCIES "Automatically try copying all dependencies" ON)
if(NOT COPY_DEPENDENCIES)
  return()
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(_bin_suffix 64)
else()
  set(_bin_suffix 32)
endif()

file(
  GLOB
  FFMPEG_BIN_FILES
  "${FFMPEG_avcodec_INCLUDE_DIR}/../bin/ffmpeg.exe"
  "${FFMPEG_avcodec_INCLUDE_DIR}/../bin/ffprobe.exe"
  "${FFMPEG_avcodec_INCLUDE_DIR}/../bin/avcodec-*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/../bin${_bin_suffix}/avcodec-*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/bin${_bin_suffix}/avcodec-*.dll"
  "${FFMPEG_avformat_INCLUDE_DIR}/../bin/avformat-*.dll"
  "${FFMPEG_avformat_INCLUDE_DIR}/../bin${_bin_suffix}/avformat-*.dll"
  "${FFMPEG_avformat_INCLUDE_DIR}/bin${_bin_suffix}/avformat-*.dll"
  "${FFMPEG_avutil_INCLUDE_DIR}/../bin/avutil-*.dll"
  "${FFMPEG_avutil_INCLUDE_DIR}/../bin${_bin_suffix}/avutil-*.dll"
  "${FFMPEG_avutil_INCLUDE_DIR}/bin${_bin_suffix}/avutil-*.dll"
  "${FFMPEG_avdevice_INCLUDE_DIR}/../bin/avdevice-*.dll"
  "${FFMPEG_avdevice_INCLUDE_DIR}/../bin${_bin_suffix}/avdevice-*.dll"
  "${FFMPEG_avdevice_INCLUDE_DIR}/bin${_bin_suffix}/avdevice-*.dll"
  "${FFMPEG_avfilter_INCLUDE_DIR}/../bin/avfilter-*.dll"
  "${FFMPEG_avfilter_INCLUDE_DIR}/../bin${_bin_suffix}/avfilter-*.dll"
  "${FFMPEG_avfilter_INCLUDE_DIR}/bin${_bin_suffix}/avfilter-*.dll"
  "${FFMPEG_postproc_INCLUDE_DIR}/../bin/postproc-*.dll"
  "${FFMPEG_postproc_INCLUDE_DIR}/../bin${_bin_suffix}/postproc-*.dll"
  "${FFMPEG_postproc_INCLUDE_DIR}/bin${_bin_suffix}/postproc-*.dll"
  "${FFMPEG_swscale_INCLUDE_DIR}/../bin/swscale-*.dll"
  "${FFMPEG_swscale_INCLUDE_DIR}/bin${_bin_suffix}/swscale-*.dll"
  "${FFMPEG_swscale_INCLUDE_DIR}/../bin${_bin_suffix}/swscale-*.dll"
  "${FFMPEG_swresample_INCLUDE_DIR}/../bin/swresample-*.dll"
  "${FFMPEG_swresample_INCLUDE_DIR}/../bin${_bin_suffix}/swresample-*.dll"
  "${FFMPEG_swresample_INCLUDE_DIR}/bin${_bin_suffix}/swresample-*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/../bin/libopus*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/../bin/opus*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/bin/libopus*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/bin/opus*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/../bin/libogg*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/../bin/libvorbis*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/bin/libogg*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/bin/libvorbis*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/../bin/libvpx*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/bin/libvpx*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/../bin/srt*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/bin/srt*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/../bin/libmbedcrypto*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/bin/libmbedcrypto*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/../bin/libmbedtls*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/bin/libmbedtls*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/../bin/libmbedx509*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/bin/libmbedx509*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/../bin${_bin_suffix}/libopus*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/../bin${_bin_suffix}/opus*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/bin${_bin_suffix}/libopus*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/bin${_bin_suffix}/opus*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/../bin/libbz2*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/../bin/zlib*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/bin/libbz2*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/bin/zlib*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/../bin${_bin_suffix}/libSvtAv1Enc.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/bin${_bin_suffix}/libSvtAv1Enc.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/../bin/libSvtAv1Enc.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/bin/libSvtAv1Enc.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/../bin${_bin_suffix}/libaom.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/bin${_bin_suffix}/libaom.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/../bin/libaom.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/bin/libaom.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/../bin${_bin_suffix}/librist.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/bin${_bin_suffix}/librist.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/../bin/librist.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/bin/librist.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/../bin${_bin_suffix}/libbz2*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/../bin${_bin_suffix}/zlib*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/bin${_bin_suffix}/libbz2*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/bin${_bin_suffix}/zlib*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/../bin/*datachannel*.dll"
  "${FFMPEG_avcodec_INCLUDE_DIR}/bin/*datachannel*.dll")

file(GLOB X264_BIN_FILES "${X264_INCLUDE_DIR}/../bin${_bin_suffix}/libx264-*.dll"
     "${X264_INCLUDE_DIR}/../bin/libx264-*.dll" "${X264_INCLUDE_DIR}/bin/libx264-*.dll"
     "${X264_INCLUDE_DIR}/bin${_bin_suffix}/libx264-*.dll")

file(
  GLOB
  FREETYPE_BIN_FILES
  "${FREETYPE_INCLUDE_DIR_ft2build}/../../bin${_bin_suffix}/libfreetype*-*.dll"
  "${FREETYPE_INCLUDE_DIR_ft2build}/../../bin/libfreetype*-*.dll"
  "${FREETYPE_INCLUDE_DIR_ft2build}/../bin${_bin_suffix}/libfreetype*-*.dll"
  "${FREETYPE_INCLUDE_DIR_ft2build}/../bin/libfreetype*-*.dll"
  "${FREETYPE_INCLUDE_DIR_ft2build}/bin/libfreetype*-*.dll"
  "${FREETYPE_INCLUDE_DIR_ft2build}/bin${_bin_suffix}/libfreetype*-*.dll"
  "${FREETYPE_INCLUDE_DIR_ft2build}/../../bin${_bin_suffix}/freetype.dll"
  "${FREETYPE_INCLUDE_DIR_ft2build}/../../bin/freetype.dll"
  "${FREETYPE_INCLUDE_DIR_ft2build}/../bin${_bin_suffix}/freetype.dll"
  "${FREETYPE_INCLUDE_DIR_ft2build}/../bin/freetype.dll")

file(GLOB LIBFDK_BIN_FILES "${Libfdk_INCLUDE_DIR}/../bin${_bin_suffix}/libfdk*-*.dll"
     "${Libfdk_INCLUDE_DIR}/../bin/libfdk*-*.dll" "${Libfdk_INCLUDE_DIR}/bin/libfdk*-*.dll"
     "${Libfdk_INCLUDE_DIR}/bin${_bin_suffix}/libfdk*-*.dll")

file(
  GLOB
  SSL_BIN_FILES
  "${SSL_INCLUDE_DIR}/../bin${_bin_suffix}/ssleay32*.dll"
  "${SSL_INCLUDE_DIR}/../bin${_bin_suffix}/libeay32*.dll"
  "${SSL_INCLUDE_DIR}/../bin/ssleay32*.dll"
  "${SSL_INCLUDE_DIR}/../bin/libeay32*.dll"
  "${SSL_INCLUDE_DIR}/bin${_bin_suffix}/ssleay32*.dll"
  "${SSL_INCLUDE_DIR}/bin${_bin_suffix}/libeay32*.dll"
  "${SSL_INCLUDE_DIR}/bin/ssleay32*.dll"
  "${SSL_INCLUDE_DIR}/bin/libeay32*.dll")

if(NOT DEFINED CURL_INCLUDE_DIR AND TARGET CURL::libcurl)
  get_target_property(CURL_INCLUDE_DIR CURL::libcurl INTERFACE_INCLUDE_DIRECTORIES)
endif()

file(
  GLOB
  CURL_BIN_FILES
  "${CURL_INCLUDE_DIR}/../build/Win${_bin_suffix}/VC12/DLL Release - DLL Windows SSPI/libcurl.dll"
  "${CURL_INCLUDE_DIR}/../bin${_bin_suffix}/libcurl*.dll"
  "${CURL_INCLUDE_DIR}/../bin${_bin_suffix}/curl*.dll"
  "${CURL_INCLUDE_DIR}/../bin/libcurl*.dll"
  "${CURL_INCLUDE_DIR}/../bin/curl*.dll"
  "${CURL_INCLUDE_DIR}/bin${_bin_suffix}/libcurl*.dll"
  "${CURL_INCLUDE_DIR}/bin${_bin_suffix}/curl*.dll"
  "${CURL_INCLUDE_DIR}/bin/libcurl*.dll"
  "${CURL_INCLUDE_DIR}/bin/curl*.dll")

file(
  GLOB
  LUA_BIN_FILES
  "${LUAJIT_INCLUDE_DIR}/../../bin${_bin_suffix}/lua*.dll"
  "${LUAJIT_INCLUDE_DIR}/../../bin/lua*.dll"
  "${LUAJIT_INCLUDE_DIR}/../bin${_bin_suffix}/lua*.dll"
  "${LUAJIT_INCLUDE_DIR}/../bin/lua*.dll"
  "${LUAJIT_INCLUDE_DIR}/bin${_bin_suffix}/lua*.dll"
  "${LUAJIT_INCLUDE_DIR}/bin/lua*.dll"
  "${LUAJIT_INCLUDE_DIR}/lua*.dll")

if(ZLIB_LIB)
  get_filename_component(ZLIB_BIN_PATH ${ZLIB_LIB} PATH)
endif()
file(GLOB ZLIB_BIN_FILES "${ZLIB_BIN_PATH}/zlib*.dll")

if(NOT ZLIB_BIN_FILES)
  file(GLOB ZLIB_BIN_FILES "${ZLIB_INCLUDE_DIR}/../bin${_bin_suffix}/zlib*.dll" "${ZLIB_INCLUDE_DIR}/../bin/zlib*.dll"
       "${ZLIB_INCLUDE_DIR}/bin${_bin_suffix}/zlib*.dll" "${ZLIB_INCLUDE_DIR}/bin/zlib*.dll")
endif()

file(GLOB RNNOISE_BIN_FILES "${RNNOISE_INCLUDE_DIR}/../bin${_bin_suffix}/rnnoise*.dll"
     "${RNNOISE_INCLUDE_DIR}/../bin/rnnoise*.dll")

set(QtCore_DIR "${Qt6Core_DIR}")
cmake_path(SET QtCore_DIR_NORM NORMALIZE "${QtCore_DIR}/../../..")
set(QtCore_BIN_DIR "${QtCore_DIR_NORM}bin")
set(QtCore_PLUGIN_DIR "${QtCore_DIR_NORM}plugins")
message(STATUS "QtCore_BIN_DIR: ${QtCore_BIN_DIR}")
message(STATUS "QtCore_PLUGIN_DIR: ${QtCore_PLUGIN_DIR}")

file(
  GLOB
  QT_DEBUG_BIN_FILES
  "${QtCore_BIN_DIR}/Qt6Cored.dll"
  "${QtCore_BIN_DIR}/Qt6Guid.dll"
  "${QtCore_BIN_DIR}/Qt6Widgetsd.dll"
  "${QtCore_BIN_DIR}/Qt6Svgd.dll"
  "${QtCore_BIN_DIR}/Qt6Xmld.dll"
  "${QtCore_BIN_DIR}/Qt6Networkd.dll"
  "${QtCore_BIN_DIR}/libGLESv2d.dll"
  "${QtCore_BIN_DIR}/libEGLd.dll")
file(GLOB QT_DEBUG_PLAT_BIN_FILES "${QtCore_PLUGIN_DIR}/platforms/qwindowsd.dll")
file(GLOB QT_DEBUG_STYLES_BIN_FILES "${QtCore_PLUGIN_DIR}/styles/qwindowsvistastyled.dll")
file(GLOB QT_DEBUG_ICONENGINE_BIN_FILES "${QtCore_PLUGIN_DIR}/iconengines/qsvgicond.dll")
file(GLOB QT_DEBUG_IMAGEFORMATS_BIN_FILES "${QtCore_PLUGIN_DIR}/imageformats/qsvgd.dll"
     "${QtCore_PLUGIN_DIR}/imageformats/qgifd.dll" "${QtCore_PLUGIN_DIR}/imageformats/qjpegd.dll")

file(
  GLOB
  QT_BIN_FILES
  "${QtCore_BIN_DIR}/Qt6Core.dll"
  "${QtCore_BIN_DIR}/Qt6Gui.dll"
  "${QtCore_BIN_DIR}/Qt6Widgets.dll"
  "${QtCore_BIN_DIR}/Qt6Svg.dll"
  "${QtCore_BIN_DIR}/Qt6Xml.dll"
  "${QtCore_BIN_DIR}/Qt6Network.dll"
  "${QtCore_BIN_DIR}/libGLESv2.dll"
  "${QtCore_BIN_DIR}/libEGL.dll")
file(GLOB QT_PLAT_BIN_FILES "${QtCore_PLUGIN_DIR}/platforms/qwindows.dll")
file(GLOB QT_STYLES_BIN_FILES "${QtCore_PLUGIN_DIR}/styles/qwindowsvistastyle.dll")
file(GLOB QT_ICONENGINE_BIN_FILES "${QtCore_PLUGIN_DIR}/iconengines/qsvgicon.dll")
file(GLOB QT_IMAGEFORMATS_BIN_FILES "${QtCore_PLUGIN_DIR}/imageformats/qsvg.dll"
     "${QtCore_PLUGIN_DIR}/imageformats/qgif.dll" "${QtCore_PLUGIN_DIR}/imageformats/qjpeg.dll")

file(GLOB QT_ICU_BIN_FILES "${QtCore_BIN_DIR}/icu*.dll")

set(ALL_BASE_BIN_FILES
    ${FFMPEG_BIN_FILES}
    ${X264_BIN_FILES}
    ${CURL_BIN_FILES}
    ${LUA_BIN_FILES}
    ${SSL_BIN_FILES}
    ${ZLIB_BIN_FILES}
    ${LIBFDK_BIN_FILES}
    ${FREETYPE_BIN_FILES}
    ${RNNOISE_BIN_FILES}
    ${QT_ICU_BIN_FILES})

set(ALL_REL_BIN_FILES ${QT_BIN_FILES})

set(ALL_DBG_BIN_FILES ${QT_DEBUG_BIN_FILES})

set(ALL_PLATFORM_BIN_FILES)
set(ALL_PLATFORM_REL_BIN_FILES ${QT_PLAT_BIN_FILES})
set(ALL_PLATFORM_DBG_BIN_FILES ${QT_DEBUG_PLAT_BIN_FILES})

set(ALL_STYLES_BIN_FILES)
set(ALL_STYLES_REL_BIN_FILES ${QT_STYLES_BIN_FILES})
set(ALL_STYLES_DBG_BIN_FILES ${QT_DEBUG_STYLES_BIN_FILES})

set(ALL_ICONENGINE_BIN_FILES)
set(ALL_ICONENGINE_REL_BIN_FILES ${QT_ICONENGINE_BIN_FILES})
set(ALL_ICONENGINE_DBG_BIN_FILES ${QT_DEBUG_ICONENGINE_BIN_FILES})

set(ALL_IMAGEFORMATS_BIN_FILES)
set(ALL_IMAGEFORMATS_REL_BIN_FILES ${QT_IMAGEFORMATS_BIN_FILES})
set(ALL_IMAGEFORMATS_DBG_BIN_FILES ${QT_DEBUG_IMAGEFORMATS_BIN_FILES})

foreach(
  list
  ALL_BASE_BIN_FILES
  ALL_REL_BIN_FILES
  ALL_DBG_BIN_FILES
  ALL_PLATFORM_BIN_FILES
  ALL_PLATFORM_REL_BIN_FILES
  ALL_PLATFORM_DBG_BIN_FILES
  ALL_STYLES_BIN_FILES
  ALL_STYLES_REL_BIN_FILES
  ALL_STYLES_DBG_BIN_FILES
  ALL_ICONENGINE_BIN_FILES
  ALL_ICONENGINE_REL_BIN_FILES
  ALL_ICONENGINE_DGB_BIN_FILES
  ALL_IMAGEFORMATS_BIN_FILES
  ALL_IMAGEFORMATS_REL_BIN_FILES
  ALL_IMAGEFORMATS_DGB_BIN_FILES)
  if(${list})
    list(REMOVE_DUPLICATES ${list})
  endif()
endforeach()

message(STATUS "FFmpeg files: ${FFMPEG_BIN_FILES}")
message(STATUS "x264 files: ${X264_BIN_FILES}")
message(STATUS "Libfdk files: ${LIBFDK_BIN_FILES}")
message(STATUS "Freetype files: ${FREETYPE_BIN_FILES}")
message(STATUS "rnnoise files: ${RNNOISE_BIN_FILES}")
message(STATUS "curl files: ${CURL_BIN_FILES}")
message(STATUS "lua files: ${LUA_BIN_FILES}")
message(STATUS "ssl files: ${SSL_BIN_FILES}")
message(STATUS "zlib files: ${ZLIB_BIN_FILES}")
message(STATUS "Qt Debug files: ${QT_DEBUG_BIN_FILES}")
message(STATUS "Qt Debug Platform files: ${QT_DEBUG_PLAT_BIN_FILES}")
message(STATUS "Qt Debug Styles files: ${QT_DEBUG_STYLES_BIN_FILES}")
message(STATUS "Qt Debug Iconengine files: ${QT_DEBUG_ICONENGINE_BIN_FILES}")
message(STATUS "Qt Debug Imageformat files: ${QT_DEBUG_IMAGEFORMATS_BIN_FILES}")
message(STATUS "Qt Release files: ${QT_BIN_FILES}")
message(STATUS "Qt Release Platform files: ${QT_PLAT_BIN_FILES}")
message(STATUS "Qt Release Styles files: ${QT_STYLES_BIN_FILES}")
message(STATUS "Qt Release Iconengine files: ${QT_ICONENGINE_BIN_FILES}")
message(STATUS "Qt Release Imageformat files: ${QT_IMAGEFORMATS_BIN_FILES}")
message(STATUS "Qt ICU files: ${QT_ICU_BIN_FILES}")

foreach(BinFile ${ALL_BASE_BIN_FILES})
  message(STATUS "copying ${BinFile} to ${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}")
  file(COPY "${BinFile}" DESTINATION "${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}/")
endforeach()

foreach(BinFile ${ALL_REL_BIN_FILES})
  message(STATUS "copying ${BinFile} to ${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}r")
  file(COPY "${BinFile}" DESTINATION "${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}r/")
endforeach()

foreach(BinFile ${ALL_DBG_BIN_FILES})
  message(STATUS "copying ${BinFile} to ${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}d")
  file(COPY "${BinFile}" DESTINATION "${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}d/")
endforeach()

foreach(BinFile ${ALL_PLATFORM_BIN_FILES})
  make_directory("${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}/platforms")
  file(COPY "${BinFile}" DESTINATION "${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}/platforms/")
endforeach()

foreach(BinFile ${ALL_PLATFORM_REL_BIN_FILES})
  make_directory("${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}r/platforms")
  file(COPY "${BinFile}" DESTINATION "${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}r/platforms/")
endforeach()

foreach(BinFile ${ALL_PLATFORM_DBG_BIN_FILES})
  make_directory("${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}d/platforms")
  file(COPY "${BinFile}" DESTINATION "${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}d/platforms/")
endforeach()

foreach(BinFile ${ALL_STYLES_BIN_FILES})
  make_directory("${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}/styles")
  file(COPY "${BinFile}" DESTINATION "${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}/styles/")
endforeach()

foreach(BinFile ${ALL_STYLES_REL_BIN_FILES})
  make_directory("${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}r/styles")
  file(COPY "${BinFile}" DESTINATION "${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}r/styles/")
endforeach()

foreach(BinFile ${ALL_STYLES_DBG_BIN_FILES})
  make_directory("${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}d/styles")
  file(COPY "${BinFile}" DESTINATION "${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}d/styles/")
endforeach()

foreach(BinFile ${ALL_ICONENGINE_BIN_FILES})
  make_directory("${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}/iconengines")
  file(COPY "${BinFile}" DESTINATION "${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}/iconengines/")
endforeach()

foreach(BinFile ${ALL_ICONENGINE_REL_BIN_FILES})
  make_directory("${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}r/iconengines")
  file(COPY "${BinFile}" DESTINATION "${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}r/iconengines/")
endforeach()

foreach(BinFile ${ALL_ICONENGINE_DBG_BIN_FILES})
  make_directory("${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}d/iconengines")
  file(COPY "${BinFile}" DESTINATION "${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}d/iconengines/")
endforeach()

foreach(BinFile ${ALL_IMAGEFORMATS_BIN_FILES})
  make_directory("${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}/imageformats")
  file(COPY "${BinFile}" DESTINATION "${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}/imageformats/")
endforeach()

foreach(BinFile ${ALL_IMAGEFORMATS_REL_BIN_FILES})
  make_directory("${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}r/imageformats")
  file(COPY "${BinFile}" DESTINATION "${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}r/imageformats/")
endforeach()

foreach(BinFile ${ALL_IMAGEFORMATS_DBG_BIN_FILES})
  make_directory("${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}d/imageformats")
  file(COPY "${BinFile}" DESTINATION "${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}d/imageformats/")
endforeach()

set(COPIED_DEPENDENCIES
    TRUE
    CACHE BOOL "Dependencies have been copied, set to false to copy again" FORCE)
