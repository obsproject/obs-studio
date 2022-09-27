# Distributed under the OSI-approved MIT License.  See accompanying
# file LICENSE or https://github.com/Crascit/DownloadProject for details.
#
# MODULE:   DownloadProject
#
# PROVIDES:
#   download_project( PROJ projectName
#                    [PREFIX prefixDir]
#                    [DOWNLOAD_DIR downloadDir]
#                    [SOURCE_DIR srcDir]
#                    [BINARY_DIR binDir]
#                    [QUIET]
#                    ...
#   )
#
#       Provides the ability to download and unpack a tarball, zip file, git repository,
#       etc. at configure time (i.e. when the cmake command is run). How the downloaded
#       and unpacked contents are used is up to the caller, but the motivating case is
#       to download source code which can then be included directly in the build with
#       add_subdirectory() after the call to download_project(). Source and build
#       directories are set up with this in mind.
#
#       The PROJ argument is required. The projectName value will be used to construct
#       the following variables upon exit (obviously replace projectName with its actual
#       value):
#
#           projectName_SOURCE_DIR
#           projectName_BINARY_DIR
#
#       The SOURCE_DIR and BINARY_DIR arguments are optional and would not typically
#       need to be provided. They can be specified if you want the downloaded source
#       and build directories to be located in a specific place. The contents of
#       projectName_SOURCE_DIR and projectName_BINARY_DIR will be populated with the
#       locations used whether you provide SOURCE_DIR/BINARY_DIR or not.
#
#       The DOWNLOAD_DIR argument does not normally need to be set. It controls the
#       location of the temporary CMake build used to perform the download.
#
#       The PREFIX argument can be provided to change the base location of the default
#       values of DOWNLOAD_DIR, SOURCE_DIR and BINARY_DIR. If all of those three arguments
#       are provided, then PREFIX will have no effect. The default value for PREFIX is
#       CMAKE_BINARY_DIR.
#
#       The QUIET option can be given if you do not want to show the output associated
#       with downloading the specified project.
#
#       In addition to the above, any other options are passed through unmodified to
#       ExternalProject_Add() to perform the actual download, patch and update steps.
#       The following ExternalProject_Add() options are explicitly prohibited (they
#       are reserved for use by the download_project() command):
#
#           CONFIGURE_COMMAND
#           BUILD_COMMAND
#           INSTALL_COMMAND
#           TEST_COMMAND
#
#       Only those ExternalProject_Add() arguments which relate to downloading, patching
#       and updating of the project sources are intended to be used. Also note that at
#       least one set of download-related arguments are required.
#
#       If using CMake 3.2 or later, the UPDATE_DISCONNECTED option can be used to
#       prevent a check at the remote end for changes every time CMake is run
#       after the first successful download. See the documentation of the ExternalProject
#       module for more information. It is likely you will want to use this option if it
#       is available to you. Note, however, that the ExternalProject implementation contains
#       bugs which result in incorrect handling of the UPDATE_DISCONNECTED option when
#       using the URL download method or when specifying a SOURCE_DIR with no download
#       method. Fixes for these have been created, the last of which is scheduled for
#       inclusion in CMake 3.8.0. Details can be found here:
#
#           https://gitlab.kitware.com/cmake/cmake/commit/bdca68388bd57f8302d3c1d83d691034b7ffa70c
#           https://gitlab.kitware.com/cmake/cmake/issues/16428
#
#       If you experience build errors related to the update step, consider avoiding
#       the use of UPDATE_DISCONNECTED.
#
# EXAMPLE USAGE:
#
#   include(DownloadProject)
#   download_project(PROJ                googletest
#                    GIT_REPOSITORY      https://github.com/google/googletest.git
#                    GIT_TAG             master
#                    UPDATE_DISCONNECTED 1
#                    QUIET
#   )
#
#   add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
#
#========================================================================================


set(_DownloadProjectDir "${CMAKE_CURRENT_LIST_DIR}")

include(CMakeParseArguments)

function(download_project)

    set(options QUIET)
    set(oneValueArgs
        PROJ
        PREFIX
        DOWNLOAD_DIR
        SOURCE_DIR
        BINARY_DIR
        # Prevent the following from being passed through
        CONFIGURE_COMMAND
        BUILD_COMMAND
        INSTALL_COMMAND
        TEST_COMMAND
    )
    set(multiValueArgs "")

    cmake_parse_arguments(DL_ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Hide output if requested
    if (DL_ARGS_QUIET)
        set(OUTPUT_QUIET "OUTPUT_QUIET")
    else()
        unset(OUTPUT_QUIET)
        message(STATUS "Downloading/updating ${DL_ARGS_PROJ}")
    endif()

    # Set up where we will put our temporary CMakeLists.txt file and also
    # the base point below which the default source and binary dirs will be.
    # The prefix must always be an absolute path.
    if (NOT DL_ARGS_PREFIX)
        set(DL_ARGS_PREFIX "${CMAKE_BINARY_DIR}")
    else()
        get_filename_component(DL_ARGS_PREFIX "${DL_ARGS_PREFIX}" ABSOLUTE
                               BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    endif()
    if (NOT DL_ARGS_DOWNLOAD_DIR)
        set(DL_ARGS_DOWNLOAD_DIR "${DL_ARGS_PREFIX}/${DL_ARGS_PROJ}-download")
    endif()

    # Ensure the caller can know where to find the source and build directories
    if (NOT DL_ARGS_SOURCE_DIR)
        set(DL_ARGS_SOURCE_DIR "${DL_ARGS_PREFIX}/${DL_ARGS_PROJ}-src")
    endif()
    if (NOT DL_ARGS_BINARY_DIR)
        set(DL_ARGS_BINARY_DIR "${DL_ARGS_PREFIX}/${DL_ARGS_PROJ}-build")
    endif()
    set(${DL_ARGS_PROJ}_SOURCE_DIR "${DL_ARGS_SOURCE_DIR}" PARENT_SCOPE)
    set(${DL_ARGS_PROJ}_BINARY_DIR "${DL_ARGS_BINARY_DIR}" PARENT_SCOPE)

    # The way that CLion manages multiple configurations, it causes a copy of
    # the CMakeCache.txt to be copied across due to it not expecting there to
    # be a project within a project.  This causes the hard-coded paths in the
    # cache to be copied and builds to fail.  To mitigate this, we simply
    # remove the cache if it exists before we configure the new project.  It
    # is safe to do so because it will be re-generated.  Since this is only
    # executed at the configure step, it should not cause additional builds or
    # downloads.
    file(REMOVE "${DL_ARGS_DOWNLOAD_DIR}/CMakeCache.txt")

    # Create and build a separate CMake project to carry out the download.
    # If we've already previously done these steps, they will not cause
    # anything to be updated, so extra rebuilds of the project won't occur.
    # Make sure to pass through CMAKE_MAKE_PROGRAM in case the main project
    # has this set to something not findable on the PATH.
    configure_file("${_DownloadProjectDir}/DownloadProject.CMakeLists.cmake.in"
                   "${DL_ARGS_DOWNLOAD_DIR}/CMakeLists.txt")
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}"
                        -D "CMAKE_MAKE_PROGRAM:FILE=${CMAKE_MAKE_PROGRAM}"
                        .
                    RESULT_VARIABLE result
                    ${OUTPUT_QUIET}
                    WORKING_DIRECTORY "${DL_ARGS_DOWNLOAD_DIR}"
    )
    if(result)
        message(FATAL_ERROR "CMake step for ${DL_ARGS_PROJ} failed: ${result}")
    endif()
    execute_process(COMMAND ${CMAKE_COMMAND} --build .
                    RESULT_VARIABLE result
                    ${OUTPUT_QUIET}
                    WORKING_DIRECTORY "${DL_ARGS_DOWNLOAD_DIR}"
    )
    if(result)
        message(FATAL_ERROR "Build step for ${DL_ARGS_PROJ} failed: ${result}")
    endif()

endfunction()
