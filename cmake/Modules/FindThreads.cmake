#.rst:
# FindThreads
# -----------
#
# This module determines the thread library of the system.
#
# The following import target is created
#
# ::
#
#   CMake::Threads
#
# The following variables are set
#
# ::
#
#   THREADS_FOUND
#   THREADS_LIBRARIES
#   THREADS_DEFINITIONS
#
# Legacy variables
#
# ::
#
#   CMAKE_THREAD_LIBS_INIT     - the thread library
#   CMAKE_THREAD_DEFS_INIT     - the thread compile definitions
#   CMAKE_USE_SPROC_INIT       - are we using sproc?
#   CMAKE_USE_WIN32_THREADS_INIT - using WIN32 threads?
#   CMAKE_USE_PTHREADS_INIT    - are we using pthreads
#   CMAKE_HP_PTHREADS_INIT     - are we using hp pthreads
#
# For systems with multiple thread libraries, caller can set
#
# ::
#
#   CMAKE_THREAD_PREFER_PTHREAD

#=============================================================================
# Copyright 2002-2014 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CheckSymbolExists)
set(Threads_FOUND FALSE)
set(CMAKE_REQUIRED_QUIET_SAVE ${CMAKE_REQUIRED_QUIET})
set(CMAKE_REQUIRED_QUIET ${Threads_FIND_QUIETLY})

# Do we have sproc?
if(CMAKE_SYSTEM_NAME MATCHES IRIX AND NOT CMAKE_THREAD_PREFER_PTHREAD)
  check_include_files("sys/types.h;sys/prctl.h"  CMAKE_HAVE_SPROC_H)
endif()

if(CMAKE_HAVE_SPROC_H AND NOT CMAKE_THREAD_PREFER_PTHREAD)
  # We have sproc
  set(CMAKE_USE_SPROC_INIT 1)
else()
  # Do we have pthreads?
  check_include_files("pthread.h" CMAKE_HAVE_PTHREAD_H)

  if(CMAKE_HAVE_PTHREAD_H)

    #
    # We have pthread.h
    # Let's check for the library now.
    #

    set(CMAKE_HAVE_THREADS_LIBRARY)

    # Check if pthread functions are in normal C library
    check_symbol_exists(pthread_create pthread.h CMAKE_HAVE_LIBC_CREATE)
    if(CMAKE_HAVE_LIBC_CREATE)
      set(CMAKE_THREAD_LIBS_INIT "")
      set(CMAKE_HAVE_THREADS_LIBRARY 1)
      set(Threads_FOUND TRUE)
    endif()

    if(NOT Threads_FOUND AND NOT THREADS_HAVE_PTHREAD_ARG)
      message(STATUS "Check if compiler accepts -pthread")
      try_run(THREADS_PTHREAD_ARG THREADS_HAVE_PTHREAD_ARG
        ${CMAKE_BINARY_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/CheckForPthreads.c
        COMPILE_DEFINITIONS -pthread
        CMAKE_FLAGS -DLINK_LIBRARIES:STRING=-pthread
        COMPILE_OUTPUT_VARIABLE OUTPUT)

      if(THREADS_HAVE_PTHREAD_ARG)
        if(THREADS_PTHREAD_ARG STREQUAL "2")
          message(STATUS "Check if compiler accepts -pthread - yes")
        else()
          message(STATUS "Check if compiler accepts -pthread - no")
          file(APPEND
            ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
            "Determining if compiler accepts -pthread returned ${THREADS_PTHREAD_ARG} instead of 2. The compiler had the following output:\n${OUTPUT}\n\n")
          set(THREADS_HAVE_PTHREAD_ARG)
        endif()
      else()
        message(STATUS "Check if compiler accepts -pthread - no")
        file(APPEND
          ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
          "Determining if compiler accepts -pthread failed with the following output:\n${OUTPUT}\n\n")
      endif()
    endif()

    if(NOT Threads_FOUND AND THREADS_HAVE_PTHREAD_ARG)
      set(Threads_FOUND TRUE)
      set(CMAKE_THREAD_LIBS_INIT "-pthread")
      set(CMAKE_THREAD_DEFS_INIT "-pthread")
    endif()

    if(NOT Threads_FOUND)
      if(NOT CMAKE_HAVE_THREADS_LIBRARY)
        # Do we have -lpthreads
        check_library_exists(pthreads pthread_create "" CMAKE_HAVE_PTHREADS_CREATE)
        if(CMAKE_HAVE_PTHREADS_CREATE)
          set(CMAKE_THREAD_LIBS_INIT "-lpthreads")
          set(CMAKE_HAVE_THREADS_LIBRARY 1)
          set(Threads_FOUND TRUE)
        endif()

        # Ok, how about -lpthread
        check_library_exists(pthread pthread_create "" CMAKE_HAVE_PTHREAD_CREATE)
        if(CMAKE_HAVE_PTHREAD_CREATE)
          set(CMAKE_THREAD_LIBS_INIT "-lpthread")
          set(CMAKE_HAVE_THREADS_LIBRARY 1)
          set(Threads_FOUND TRUE)
        endif()

        if(CMAKE_SYSTEM MATCHES "SunOS.*")
          # On sun also check for -lthread
          check_library_exists(thread thr_create "" CMAKE_HAVE_THR_CREATE)
          if(CMAKE_HAVE_THR_CREATE)
            set(CMAKE_THREAD_LIBS_INIT "-lthread")
            set(CMAKE_HAVE_THREADS_LIBRARY 1)
            set(Threads_FOUND TRUE)
          endif()
        endif()
      endif()
    endif()

  endif()
endif()

if(CMAKE_THREAD_LIBS_INIT OR CMAKE_HAVE_LIBC_CREATE)
  set(CMAKE_USE_PTHREADS_INIT 1)
  set(Threads_FOUND TRUE)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
  set(CMAKE_USE_WIN32_THREADS_INIT 1)
  set(Threads_FOUND TRUE)
endif()

if(CMAKE_USE_PTHREADS_INIT)
  if(CMAKE_SYSTEM_NAME MATCHES "HP-UX")
    # Use libcma if it exists and can be used.  It provides more
    # symbols than the plain pthread library.  CMA threads
    # have actually been deprecated:
    #   http://docs.hp.com/en/B3920-90091/ch12s03.html#d0e11395
    #   http://docs.hp.com/en/947/d8.html
    # but we need to maintain compatibility here.
    # The CMAKE_HP_PTHREADS setting actually indicates whether CMA threads
    # are available.
    check_library_exists(cma pthread_attr_create "" CMAKE_HAVE_HP_CMA)
    if(CMAKE_HAVE_HP_CMA)
      set(CMAKE_THREAD_LIBS_INIT "-lcma")
      set(CMAKE_HP_PTHREADS_INIT 1)
      set(Threads_FOUND TRUE)
    endif()
    set(CMAKE_USE_PTHREADS_INIT 1)
  endif()

  if(CMAKE_SYSTEM MATCHES "OSF1-V")
    set(CMAKE_USE_PTHREADS_INIT 0)
    set(CMAKE_THREAD_LIBS_INIT )
  endif()

  if(CMAKE_SYSTEM MATCHES "CYGWIN_NT")
    set(CMAKE_USE_PTHREADS_INIT 1)
    set(Threads_FOUND TRUE)
    set(CMAKE_THREAD_LIBS_INIT )
    set(CMAKE_USE_WIN32_THREADS_INIT 0)
  endif()
endif()

set(CMAKE_REQUIRED_QUIET ${CMAKE_REQUIRED_QUIET_SAVE})
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Threads DEFAULT_MSG Threads_FOUND)

if(THREADS_FOUND)
  set(THREADS_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}")
  set(THREADS_DEFINITIONS "${CMAKE_THREAD_DEFS_INIT}")
endif()
