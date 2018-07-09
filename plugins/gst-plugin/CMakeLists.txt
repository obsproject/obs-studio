
project(gst-plugin)

# - Try to find GStreamer and its plugins
# Once done, this will define
#
#  GSTREAMER_FOUND - system has GStreamer
#  GSTREAMER_INCLUDE_DIRS - the GStreamer include directories
#  GSTREAMER_LIBRARIES - link these to use GStreamer
#
# Additionally, gstreamer-base is always looked for and required, and
# the following related variables are defined:
#
#  GSTREAMER_BASE_INCLUDE_DIRS - gstreamer-base's include directory
#  GSTREAMER_BASE_LIBRARIES - link to these to use gstreamer-base
#
# Optionally, the COMPONENTS keyword can be passed to find_package()
# and GStreamer plugins can be looked for.  Currently, the following
# plugins can be searched, and they define the following variables if
# found:
#
#  gstreamer-app:        GSTREAMER_APP_INCLUDE_DIRS and GSTREAMER_APP_LIBRARIES
#  gstreamer-audio:      GSTREAMER_AUDIO_INCLUDE_DIRS and GSTREAMER_AUDIO_LIBRARIES
#  gstreamer-fft:        GSTREAMER_FFT_INCLUDE_DIRS and GSTREAMER_FFT_LIBRARIES
#  gstreamer-gl:         GSTREAMER_GL_INCLUDE_DIRS and GSTREAMER_GL_LIBRARIES
#  gstreamer-mpegts:     GSTREAMER_MPEGTS_INCLUDE_DIRS and GSTREAMER_MPEGTS_LIBRARIES
#  gstreamer-pbutils:    GSTREAMER_PBUTILS_INCLUDE_DIRS and GSTREAMER_PBUTILS_LIBRARIES
#  gstreamer-tag:        GSTREAMER_TAG_INCLUDE_DIRS and GSTREAMER_TAG_LIBRARIES
#  gstreamer-video:      GSTREAMER_VIDEO_INCLUDE_DIRS and GSTREAMER_VIDEO_LIBRARIES
#
# Copyright (C) 2012 Raphael Kubo da Costa <rakuco@webkit.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND ITS CONTRIBUTORS ``AS
# IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR ITS
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

find_package(PkgConfig)

# Helper macro to find a GStreamer plugin (or GStreamer itself)
#   _component_prefix is prepended to the _INCLUDE_DIRS and _LIBRARIES variables (eg. "GSTREAMER_AUDIO")
#   _pkgconfig_name is the component's pkg-config name (eg. "gstreamer-1.0", or "gstreamer-video-1.0").
#   _library is the component's library name (eg. "gstreamer-1.0" or "gstvideo-1.0")
macro(FIND_GSTREAMER_COMPONENT _component_prefix _pkgconfig_name _library)

    string(REGEX MATCH "(.*)>=(.*)" _dummy "${_pkgconfig_name}")
    if ("${CMAKE_MATCH_2}" STREQUAL "")
        pkg_check_modules(PC_${_component_prefix} "${_pkgconfig_name} >= ${GStreamer_FIND_VERSION}")
    else ()
        pkg_check_modules(PC_${_component_prefix} ${_pkgconfig_name})
    endif ()
    set(${_component_prefix}_INCLUDE_DIRS ${PC_${_component_prefix}_INCLUDE_DIRS})

    find_library(${_component_prefix}_LIBRARIES
        NAMES ${_library}
        HINTS ${PC_${_component_prefix}_LIBRARY_DIRS} ${PC_${_component_prefix}_LIBDIR}
    )
endmacro()

# ------------------------
# 1. Find GStreamer itself
# ------------------------

# 1.1. Find headers and libraries
FIND_GSTREAMER_COMPONENT(GSTREAMER gstreamer-1.0 gstreamer-1.0)
FIND_GSTREAMER_COMPONENT(GSTREAMER_BASE gstreamer-base-1.0 gstbase-1.0)

# -------------------------
# 2. Find GStreamer plugins
# -------------------------

FIND_GSTREAMER_COMPONENT(GSTREAMER_APP gstreamer-app-1.0 gstapp-1.0)
FIND_GSTREAMER_COMPONENT(GSTREAMER_AUDIO gstreamer-audio-1.0 gstaudio-1.0)
FIND_GSTREAMER_COMPONENT(GSTREAMER_FFT gstreamer-fft-1.0 gstfft-1.0)
FIND_GSTREAMER_COMPONENT(GSTREAMER_GL gstreamer-gl-1.0 gstgl-1.0)
FIND_GSTREAMER_COMPONENT(GSTREAMER_MPEGTS gstreamer-mpegts-1.0>=1.4.0 gstmpegts-1.0)
FIND_GSTREAMER_COMPONENT(GSTREAMER_PBUTILS gstreamer-pbutils-1.0 gstpbutils-1.0)
FIND_GSTREAMER_COMPONENT(GSTREAMER_TAG gstreamer-tag-1.0 gsttag-1.0)
FIND_GSTREAMER_COMPONENT(GSTREAMER_VIDEO gstreamer-video-1.0 gstvideo-1.0)

# ------------------------------------------------
# 3. Process the COMPONENTS passed to FIND_PACKAGE
# ------------------------------------------------
set(_GSTREAMER_REQUIRED_VARS GSTREAMER_INCLUDE_DIRS GSTREAMER_LIBRARIES GSTREAMER_VERSION GSTREAMER_BASE_INCLUDE_DIRS GSTREAMER_BASE_LIBRARIES)

foreach (_component ${GStreamer_FIND_COMPONENTS})
    set(_gst_component "GSTREAMER_${_component}")
    string(TOUPPER ${_gst_component} _UPPER_NAME)

    list(APPEND _GSTREAMER_REQUIRED_VARS ${_UPPER_NAME}_INCLUDE_DIRS ${_UPPER_NAME}_LIBRARIES)
endforeach ()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GStreamer REQUIRED_VARS _GSTREAMER_REQUIRED_VARS
                                            VERSION_VAR   GSTREAMER_VERSION)

mark_as_advanced(
    GSTREAMER_APP_INCLUDE_DIRS
    GSTREAMER_APP_LIBRARIES
    GSTREAMER_AUDIO_INCLUDE_DIRS
    GSTREAMER_AUDIO_LIBRARIES
    GSTREAMER_BASE_INCLUDE_DIRS
    GSTREAMER_BASE_LIBRARIES
    GSTREAMER_FFT_INCLUDE_DIRS
    GSTREAMER_FFT_LIBRARIES
    GSTREAMER_GL_INCLUDE_DIRS
    GSTREAMER_GL_LIBRARIES
    GSTREAMER_INCLUDE_DIRS
    GSTREAMER_LIBRARIES
    GSTREAMER_MPEGTS_INCLUDE_DIRS
    GSTREAMER_MPEGTS_LIBRARIES
    GSTREAMER_PBUTILS_INCLUDE_DIRS
    GSTREAMER_PBUTILS_LIBRARIES
    GSTREAMER_TAG_INCLUDE_DIRS
    GSTREAMER_TAG_LIBRARIES
    GSTREAMER_VIDEO_INCLUDE_DIRS
    GSTREAMER_VIDEO_LIBRARIES
)

 message( "found = ${GSTREAMER_FOUND}" )
    message( "dirs = ${GSTREAMER_INCLUDE_DIRS}" )
    message( "libs = ${GSTREAMER_LIBRARIES}" )
    message( "base dir include = ${GSTREAMER_BASE_INCLUDE_DIRS}" )
    message( "base dir libraries = ${GSTREAMER_BASE_LIBRARIES}" )


set(gst-plugin_SOURCES
	gst-plugin.c)

add_library(gst-plugin MODULE
	${gst-plugin_SOURCES})

include_directories(gst-plugin PRIVATE ${GSTREAMER_INCLUDE_DIRS}
	"${GSTREAMER_INCLUDE_DIRS}"
	${GSTREAMER_LIBRARIES}
	${GSTREAMER_BASE_INCLUDE_DIRS}
	"${GSTREAMER_BASE_INCLUDE_DIRS}"
	${GSTREAMER_BASE_LIBRARIES}
	${GLIB_INCLUDE_DIRS})

target_link_libraries(gst-plugin
	libobs
	${gst-plugin_PLATFORM_DEPS}
	${GSTREAMER_LIBRARIES}
	${GSTREAMER_BASE_LIBRARIES}
    ${GSTREAMER_APP_LIBRARIES})

install_obs_plugin(gst-plugin)
