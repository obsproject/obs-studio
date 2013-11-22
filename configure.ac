#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.67])
AC_INIT([obs-studio], [0.0.1], [obs.jim@gmail.com])
AC_CONFIG_SRCDIR([libobs/obs.c])
AC_CONFIG_MACRO_DIR([m4])
# AC_CONFIG_HEADERS([config.h])
AC_CANONICAL_HOST

LT_INIT
AM_INIT_AUTOMAKE([foreign])
AM_SILENT_RULES([yes])

# Checks for programs.
AC_PROG_CC_STDC
AC_PROG_OBJC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_SED

AX_CXX_COMPILE_STDCXX_11
# AX_EXT
AX_APPEND_FLAG(["-msse2"], [CPPFLAGS])

# currently only x86 and x86_64 are supported architectures
case $host in
	i?86*)
		AM_CONDITIONAL([ARCH_X86],    true)
		AM_CONDITIONAL([ARCH_X86_64], false)
		;;
	amd64* | x86_64*)
		AM_CONDITIONAL([ARCH_X86],    false)
		AM_CONDITIONAL([ARCH_X86_64], true)
		;;
	*)
		AC_MSG_ERROR([host architecture not currently supported])
		;;
esac

case $host_os in
	mingw*)
		AM_CONDITIONAL([OS_WIN], true)
		AM_CONDITIONAL([OS_OSX], false)
		AM_CONDITIONAL([OS_NIX], false)
		;;
	darwin*)
		AM_CONDITIONAL([OS_WIN], false)
		AM_CONDITIONAL([OS_OSX], true)
		AM_CONDITIONAL([OS_NIX], false)
		;;
	*) #TODO - fix nix, but not with sticks, and definitely not with bricks
		AM_CONDITIONAL([OS_WIN], false)
		AM_CONDITIONAL([OS_OSX], false)
		AM_CONDITIONAL([OS_NIX], true)
		PKG_CHECK_MODULES([X11], [x11])
		;;
esac

# checks for wx
AM_OPTIONS_WXCONFIG
reqwx=2.9.0
AM_PATH_WXCONFIG($reqwx, wxWin=1)
if test "$wxWin" != 1; then
	AC_MSG_ERROR([
		wxWidgets must be installed on your system.
 
		Please check that wx-config is in path, the directory
		where wxWidgets libraries are installed (returned by
		'wx-config --libs' or 'wx-config --static --libs' command)
		is in LD_LIBRARY_PATH or equivalent variable and
		wxWidgets version is $reqwx or above.
		])
fi
 
CPPFLAGS="$CPPFLAGS $WX_CPPFLAGS"
CXXFLAGS="$CXXFLAGS $WX_CXXFLAGS_ONLY"
CFLAGS="$CFLAGS $WX_CFLAGS_ONLY"
LIBS="$LIBS $WX_LIBS"

# checks for ffmpeg
AC_CHECK_HEADER([libavcodec/avcodec.h], , AC_MSG_ERROR([libavcodec header not found]))
AC_CHECK_HEADER([libavformat/avformat.h], , AC_MSG_ERROR([libavformat header not found]))
AC_CHECK_HEADER([libavutil/avutil.h], , AC_MSG_ERROR([libavutil header not found]))
AC_CHECK_HEADER([libavutil/channel_layout.h], , AC_MSG_ERROR([libavutil header not found]))
AC_CHECK_HEADER([libswscale/swscale.h], , AC_MSG_ERROR([libswscale header not found]))
AC_CHECK_HEADER([libswresample/swresample.h], , AC_MSG_ERROR([libswresample header not found]))
AC_CHECK_LIB([avcodec], [avcodec_find_encoder_by_name], , AC_MSG_ERROR([libavcodec not found]))
AC_CHECK_LIB([avformat], [av_guess_format], , AC_MSG_ERROR([libavformat not found]))
AC_CHECK_LIB([avutil], [av_samples_alloc], , AC_MSG_ERROR([libavutil not found]))
AC_CHECK_LIB([swscale], [sws_scale], , AC_MSG_ERROR([libswscale not found]))
AC_CHECK_LIB([swresample], [swr_convert], , AC_MSG_ERROR([libswresample not found]))

# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([inttypes.h limits.h malloc.h stddef.h stdint.h stdlib.h string.h sys/timeb.h wchar.h wctype.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_FSEEKO
AC_FUNC_STRTOD
AC_CHECK_FUNCS([memmove memset socket strstr strtol strtoul])

AC_CONFIG_FILES([makefile
		 build/makefile
		 obs/makefile
		 test/makefile
		 test/test-input/makefile
		 libobs/makefile
		 libobs-opengl/makefile])

AC_OUTPUT
