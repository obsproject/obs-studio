# - Try to find libX11-xcb
# Once done this will define
#
# X11_XCB_FOUND - system has libX11-xcb
# X11_XCB_LIBRARIES - Link these to use libX11-xcb
# X11_XCB_INCLUDE_DIR - the libX11-xcb include dir
# X11_XCB_DEFINITIONS - compiler switches required for using libX11-xcb

# Copyright (c) 2011 Fredrik Höglund <fredrik@kde.org>
# Copyright (c) 2008 Helio Chissini de Castro, <helio@kde.org>
# Copyright (c) 2007 Matthias Kretz, <kretz@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

IF (NOT WIN32)
  # use pkg-config to get the directories and then use these values
  # in the FIND_PATH() and FIND_LIBRARY() calls
  FIND_PACKAGE(PkgConfig)
  PKG_CHECK_MODULES(PKG_X11_XCB QUIET x11-xcb)

  SET(X11_XCB_DEFINITIONS ${PKG_X11_XCB_CFLAGS})

  FIND_PATH(X11_XCB_INCLUDE_DIR NAMES X11/Xlib-xcb.h HINTS ${PKG_X11_XCB_INCLUDE_DIRS})
  FIND_LIBRARY(X11_XCB_LIBRARIES NAMES X11-xcb HINTS ${PKG_X11_XCB_LIBRARY_DIRS})

  include(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(X11_XCB DEFAULT_MSG X11_XCB_LIBRARIES X11_XCB_INCLUDE_DIR)

  MARK_AS_ADVANCED(X11_XCB_INCLUDE_DIR X11_XCB_LIBRARIES)
ENDIF (NOT WIN32)
