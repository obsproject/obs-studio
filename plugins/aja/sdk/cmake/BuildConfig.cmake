set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_DEBUG_POSTFIX d)
# set(CMAKE_OSX_ARCHITECTURES arm64 x86_64)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.12" CACHE STRING "Minimum OS X deployment version" FORCE)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# TODO(paulh): Re-enable when we start using the CMake version.h.in file
# string(TIMESTAMP DATETIME_NOW "\"%A %b %d, %Y (%m/%d/%Y @ %H:%M:%S GMT+8)\"")
# string(TIMESTAMP DATETIME_NOW "\"%m/%d/%Y +8:%H:%M:%S\"")
# set(AJA_NTV2_SDK_VERSION_MAJOR 16)
# set(AJA_NTV2_SDK_VERSION_MINOR 0)
# set(AJA_NTV2_SDK_VERSION_POINT 1)
# set(AJA_NTV2_SDK_BUILD_NUMBER 0)
# set(AJA_NTV2_SDK_BUILD_DATETIME ${DATETIME_NOW})
# set(AJA_NTV2_SDK_BUILD_TYPE "\"d\"")

# control where the static and shared libraries are built so that on windows
# we don't need to tinker with the path to run the executable
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
#option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

# General compiler flags
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

    if((CMAKE_BUILD_TYPE STREQUAL Debug))
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Od /RTC1 /W3")
    elseif(CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2 /W3")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if((CMAKE_BUILD_TYPE STREQUAL Debug) OR (CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo))
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O0 -g")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall -Wextra")

        # Turn on all warnings in Release builds
        # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall -Wextra -pedantic -Werror")
        # Remove these switches as we fix the underlying code
        # set(CMAKE_CXX_FLAGS
        #     "${CMAKE_CXX_FLAGS}\
        #     -Wno-language-extension-token\
        #     -Wno-microsoft-enum-value\
        #     -Wno-sign-compare\
        #     -Wno-deprecated-declarations\
        #     -Wno-bitwise-conditional-parentheses\
        #     -Wno-unused-private-field\
        #     -Wno-unused-parameter\
        #     -Wno-misleading-indentation\
        #     -Wno-deprecated-register\
        #     -Wno-deprecated-declarations\
        #     -Wno-pointer-to-int-cast\
        #     -Wno-unused-const-variable\
        #     -Wno-implicit-const-int-float-conversion\
        #     -Wno-unused-variable\
        #     -Wno-missing-braces\
        #     -Wno-format\
        #     -Wno-dangling-gsl\
        #     -Wno-unused-function")
    endif()
endif()

option(AJA_BUILD_OPENSOURCE "Build NTV2 SDK as open-source (exclude internal apps/libs)" ON)

option(BUILD_AJA_QT_BASED "Build NTV2 Demos and Apps which depend upon Qt" ON)

# apps
option(BUILD_AJA_APPS "Build AJA NTV2 applications" ON)
option(BUILD_AJA_DEMOS "Build AJA NTV2 Demos" ON)
option(BUILD_AJA_DRIVER "Build AJA NTV2 driver" ON)
option(BUILD_AJA_LIBS "Build AJA libraries" ON)
option(BUILD_AJA_TOOLS "Build AJA NTV2 tools" ON)

# libs
option(BUILD_AJAANC "Build ajaanc library" OFF)
option(BUILD_AJACC "Build ajacc library" ON)
option(BUILD_AJACONV "Build ajaconv library" ON)
option(BUILD_AJAGUI "Build ajagui library" ON)
option(BUILD_AJANTV2 "Build ajantv2 library" ON)
option(BUILD_AJASTREAMS "Build ajastreams library" ON)

# plug-ins
option(BUILD_AJA_PLUGINS "Build AJA Plug-ins" ON)

# demos with additional deps
option(BUILD_AJA_CC_DEMOS "Build NTV2 Demos which depend upon AJACC" ON)
option(BUILD_AJA_CUDA_DEMOS "Build NTV2 Demos which depend upon NVIDIA CUDA" ON)
option(BUILD_AJA_HEVC_DEMOS "Build NTV2 Demos which depend upon the M31 HEVC codec" ON)
option(BUILD_AJA_OPENGL_DEMOS "Build NTV2 Demos which depend upon OpenGL" ON)

# unit tests
option(BUILD_AJA_UNIT_TESTS "Build unit tests" ON)

if (AJA_BUILD_OPENSOURCE)
    message("Building MIT-licensed AJA NTV2 SDK components")

    set(BUILD_AJA_APPS  OFF)
    set(BUILD_AJA_DEMOS  OFF)
    set(BUILD_AJA_DRIVER OFF)
    set(BUILD_AJA_LIBS  ON)
    set(BUILD_AJA_TOOLS OFF)

    set(BUILD_AJAANC OFF)
    set(BUILD_AJACC OFF)
    set(BUILD_AJACONV OFF)
    set(BUILD_AJAGUI OFF)
    set(BUILD_AJASTREAMS OFF)
    set(BUILD_AJANTV2 ON)

    set(BUILD_AJA_CC_DEMOS OFF)
    set(BUILD_AJA_CUDA_DEMOS OFF)
    set(BUILD_AJA_HEVC_DEMOS OFF)
    set(BUILD_AJA_OPENGL_DEMOS OFF)

    set(BUILD_AJA_PLUGINS OFF)

	set(BUILD_AJA_UNIT_TESTS OFF)
endif()
