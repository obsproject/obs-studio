project(ajantv2)

if(CMAKE_CXX_STANDARD LESS 11)
    remove_definitions(
        -DAJA_USE_CPLUSPLUS11
        -DNTV2_USE_CPLUSPLUS11)
endif()

# TODO(paulh): Re-enable when we start using the CMake version.h.in file
# add_definitions(-DBUILDING_CMAKE)
remove_definitions(
    -DUNICODE
    -D_UNICODE
    -DQT_LARGEFILE_SUPPORT)

set(NTV2_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(AJALIBRARIES_ROOT ${NTV2_ROOT}/ajalibraries)
set(AJAANC_ROOT ${AJALIBRARIES_ROOT}/ajaanc)
set(AJABASE_ROOT ${AJALIBRARIES_ROOT}/ajabase)

set(AJANTV2_TARGET_INCLUDE_DIRS
includes
${AJAANC_ROOT}/includes
${AJALIBRARIES_ROOT}
${AJABASE_ROOT})

# TODO(paulh): Re-enable this if we decide to define version number inside of CMake
# version defines in ntv2enums.h populated with version.h values
# configure_file(includes/version.h.in includes/version.h @ONLY)

set(AJANTV2_HEADERS
    includes/ajaexport.h
    includes/ajatypes.h
    includes/basemachinecontrol.h
    includes/ntv2audiodefines.h
    includes/ntv2bft.h
    includes/ntv2bitfile.h
    includes/ntv2bitfilemanager.h
    includes/ntv2boardfeatures.h
    includes/ntv2boardscan.h
    includes/ntv2card.h
    includes/ntv2choosableboard.h
    includes/ntv2config2022.h
    includes/ntv2config2110.h
    includes/ntv2configts2022.h
    includes/ntv2cscmatrix.h
    includes/ntv2debug.h
    includes/ntv2debugmacros.h
    includes/ntv2devicefeatures.h
    includes/ntv2devicescanner.h
    includes/ntv2discover.h
    includes/ntv2driverinterface.h
    includes/ntv2endian.h
    includes/ntv2enhancedcsc.h
    includes/ntv2enums.h
    includes/ntv2fixed.h
    includes/ntv2formatdescriptor.h
    includes/ntv2konaflashprogram.h
    includes/ntv2m31enums.h
    includes/ntv2m31publicinterface.h
    includes/ntv2mailbox.h
    includes/ntv2mbcontroller.h
    includes/ntv2mcsfile.h
    includes/ntv2nubaccess.h
    includes/ntv2nubpktcom.h
    includes/ntv2nubtypes.h
    includes/ntv2publicinterface.h
    includes/ntv2registerexpert.h
    includes/ntv2registers2022.h
    includes/ntv2registers2110.h
    includes/ntv2registersmb.h
    includes/ntv2resample.h
    includes/ntv2routingexpert.h
    includes/ntv2rp188.h
    includes/ntv2rp215.h
    includes/ntv2serialcontrol.h
    includes/ntv2signalrouter.h
    includes/ntv2spiinterface.h
    includes/ntv2supportlogger.h
    includes/ntv2task.h
    includes/ntv2testpatterngen.h
    includes/ntv2transcode.h
    includes/ntv2tshelper.h
    includes/ntv2utf8.h
    includes/ntv2utils.h
    includes/ntv2verticalfilter.h
    includes/ntv2videodefines.h
    includes/ntv2virtualregisters.h
    includes/ntv2vpid.h
    includes/ntv2vpidfromspec.h
    # TODO(paulh): Re-enable when we start using the CMake version.h.in file
    # includes/version.h
    ${AJAANC_ROOT}/includes/ancillarydata.h
    ${AJAANC_ROOT}/includes/ancillarydatafactory.h
    ${AJAANC_ROOT}/includes/ancillarydata_cea608.h
    ${AJAANC_ROOT}/includes/ancillarydata_cea608_line21.h
    ${AJAANC_ROOT}/includes/ancillarydata_cea608_vanc.h
    ${AJAANC_ROOT}/includes/ancillarydata_cea708.h
    ${AJAANC_ROOT}/includes/ancillarydata_framestatusinfo524D.h
    ${AJAANC_ROOT}/includes/ancillarydata_framestatusinfo5251.h
    ${AJAANC_ROOT}/includes/ancillarydata_hdr_hdr10.h
    ${AJAANC_ROOT}/includes/ancillarydata_hdr_hlg.h
    ${AJAANC_ROOT}/includes/ancillarydata_hdr_sdr.h
    ${AJAANC_ROOT}/includes/ancillarydata_timecode.h
    ${AJAANC_ROOT}/includes/ancillarydata_timecode_atc.h
    ${AJAANC_ROOT}/includes/ancillarydata_timecode_vitc.h
    ${AJAANC_ROOT}/includes/ancillarylist.h)

set(AJANTV2_SOURCES
    src/ntv2anc.cpp
    src/ntv2audio.cpp
    src/ntv2autocirculate.cpp
    src/ntv2bitfile.cpp
    src/ntv2bitfilemanager.cpp
    src/ntv2card.cpp
    src/ntv2config2022.cpp
    src/ntv2config2110.cpp
    src/ntv2configts2022.cpp
    src/ntv2csclut.cpp
    src/ntv2cscmatrix.cpp
    src/ntv2debug.cpp
    src/ntv2devicefeatures.cpp
    src/ntv2devicescanner.cpp
    src/ntv2discover.cpp
    src/ntv2dma.cpp
    src/ntv2driverinterface.cpp
    src/ntv2dynamicdevice.cpp
    src/ntv2enhancedcsc.cpp
    src/ntv2formatdescriptor.cpp
    src/ntv2hdmi.cpp
    src/ntv2hevc.cpp
    src/ntv2interrupts.cpp
    src/ntv2konaflashprogram.cpp
    src/ntv2mailbox.cpp
    src/ntv2mbcontroller.cpp
    src/ntv2mcsfile.cpp
    src/ntv2nubaccess.cpp
    src/ntv2nubpktcom.cpp
    src/ntv2publicinterface.cpp
    src/ntv2register.cpp
    src/ntv2registerexpert.cpp
    src/ntv2resample.cpp
    src/ntv2routingexpert.cpp
    src/ntv2rp188.cpp
    src/ntv2rp215.cpp
    src/ntv2serialcontrol.cpp
    src/ntv2signalrouter.cpp
    src/ntv2spiinterface.cpp
    src/ntv2subscriptions.cpp
    src/ntv2supportlogger.cpp
    src/ntv2task.cpp
    src/ntv2testpatterngen.cpp
    src/ntv2transcode.cpp
    src/ntv2utf8.cpp
    src/ntv2utils.cpp
    src/ntv2verticalfilter.cpp
    src/ntv2vpid.cpp
    src/ntv2vpidfromspec.cpp
    ${AJAANC_ROOT}/src/ancillarydata.cpp
    ${AJAANC_ROOT}/src/ancillarydatafactory.cpp
    ${AJAANC_ROOT}/src/ancillarydata_cea608.cpp
    ${AJAANC_ROOT}/src/ancillarydata_cea608_line21.cpp
    ${AJAANC_ROOT}/src/ancillarydata_cea608_vanc.cpp
    ${AJAANC_ROOT}/src/ancillarydata_cea708.cpp
    ${AJAANC_ROOT}/src/ancillarydata_framestatusinfo524D.cpp
    ${AJAANC_ROOT}/src/ancillarydata_framestatusinfo5251.cpp
    ${AJAANC_ROOT}/src/ancillarydata_hdr_hdr10.cpp
    ${AJAANC_ROOT}/src/ancillarydata_hdr_hlg.cpp
    ${AJAANC_ROOT}/src/ancillarydata_hdr_sdr.cpp
    ${AJAANC_ROOT}/src/ancillarydata_timecode.cpp
    ${AJAANC_ROOT}/src/ancillarydata_timecode_atc.cpp
    ${AJAANC_ROOT}/src/ancillarydata_timecode_vitc.cpp
    ${AJAANC_ROOT}/src/ancillarylist.cpp)

if (AJA_BUILD_DRIVER STREQUAL ON)
    add_definitions(-DNTV2_BUILDING_DRIVER)
    set(AJANTV2_HEADERS
        ${AJANTV2_HEADERS}
        includes/ntv2driverprocamp.h)
    set(AJANTV2_SOURCES
        ${AJANTV2_SOURCES}
        src/ntv2driverprocamp.cpp)
endif()

if (AJA_BUILD_OPENSOURCE STREQUAL OFF)
    message("Added ajantv2 HEVC/M31 codec")
    set(AJANTV2_HEADERS
        ${AJANTV2_HEADERS}
        codecs/hevc/m31/ntv2m31.h
        codecs/hevc/m31/ntv2m31cparam.h
        codecs/hevc/m31/ntv2m31ehparam.h
        codecs/hevc/m31/ntv2m31vaparam.h
        codecs/hevc/m31/ntv2m31vinparam.h
        codecs/hevc/m31/ntv2m31viparam.h)
    set(AJANTV2_SOURCES
        ${AJANTV2_SOURCES}
        codecs/hevc/m31/ntv2m31.cpp
        codecs/hevc/m31/ntv2m31cparam.cpp
        codecs/hevc/m31/ntv2m31ehparam.cpp
        codecs/hevc/m31/ntv2m31vaparam.cpp
        codecs/hevc/m31/ntv2m31vinparam.cpp
        codecs/hevc/m31/ntv2m31viparam.cpp)
endif()

set(AJABASE_HEADERS
    ${AJABASE_ROOT}/common/ajarefptr.h
    ${AJABASE_ROOT}/common/audioutilities.h
    ${AJABASE_ROOT}/common/buffer.h
    ${AJABASE_ROOT}/common/bytestream.h
    ${AJABASE_ROOT}/common/circularbuffer.h
    ${AJABASE_ROOT}/common/cli_utils.h
    ${AJABASE_ROOT}/common/common.h
    ${AJABASE_ROOT}/common/dpxfileio.h
    ${AJABASE_ROOT}/common/dpx_hdr.h
    ${AJABASE_ROOT}/common/export.h
    ${AJABASE_ROOT}/common/guid.h
    ${AJABASE_ROOT}/common/options_popt.h
    ${AJABASE_ROOT}/common/performance.h
    ${AJABASE_ROOT}/common/pixelformat.h
    ${AJABASE_ROOT}/common/public.h
    ${AJABASE_ROOT}/common/rawfile.h
    ${AJABASE_ROOT}/common/testpatterngen.h
    ${AJABASE_ROOT}/common/timebase.h
    ${AJABASE_ROOT}/common/timecode.h
    ${AJABASE_ROOT}/common/timecodeburn.h
    ${AJABASE_ROOT}/common/timer.h
    ${AJABASE_ROOT}/common/types.h
    ${AJABASE_ROOT}/common/videotypes.h
    ${AJABASE_ROOT}/common/videoutilities.h
    ${AJABASE_ROOT}/common/wavewriter.h
    ${AJABASE_ROOT}/network/ip_socket.h
    ${AJABASE_ROOT}/network/tcp_socket.h
    ${AJABASE_ROOT}/network/udp_socket.h
    ${AJABASE_ROOT}/persistence/persistence.h
    ${AJABASE_ROOT}/persistence/sqlite3.h
    ${AJABASE_ROOT}/pnp/pnp.h
    ${AJABASE_ROOT}/system/atomic.h
    ${AJABASE_ROOT}/system/debug.h
    ${AJABASE_ROOT}/system/debugshare.h
    ${AJABASE_ROOT}/system/diskstatus.h
    ${AJABASE_ROOT}/system/event.h
    ${AJABASE_ROOT}/system/file_io.h
    ${AJABASE_ROOT}/system/info.h
    ${AJABASE_ROOT}/system/lock.h
    ${AJABASE_ROOT}/system/log.h
    ${AJABASE_ROOT}/system/make_unique_shim.h
    ${AJABASE_ROOT}/system/memory.h
    ${AJABASE_ROOT}/system/process.h
    ${AJABASE_ROOT}/system/system.h
    ${AJABASE_ROOT}/system/systemtime.h
    ${AJABASE_ROOT}/system/thread.h)

set(AJABASE_SOURCES
    ${AJABASE_ROOT}/common/audioutilities.cpp
    ${AJABASE_ROOT}/common/buffer.cpp
    ${AJABASE_ROOT}/common/common.cpp
    ${AJABASE_ROOT}/common/dpxfileio.cpp
    ${AJABASE_ROOT}/common/dpx_hdr.cpp
    ${AJABASE_ROOT}/common/guid.cpp
    ${AJABASE_ROOT}/common/options_popt.cpp
    ${AJABASE_ROOT}/common/performance.cpp
    ${AJABASE_ROOT}/common/pixelformat.cpp
    ${AJABASE_ROOT}/common/testpatterngen.cpp
    ${AJABASE_ROOT}/common/timebase.cpp
    ${AJABASE_ROOT}/common/timecode.cpp
    ${AJABASE_ROOT}/common/timecodeburn.cpp
    ${AJABASE_ROOT}/common/timer.cpp
    ${AJABASE_ROOT}/common/videoutilities.cpp
    ${AJABASE_ROOT}/common/wavewriter.cpp
    ${AJABASE_ROOT}/network/ip_socket.cpp
    ${AJABASE_ROOT}/network/tcp_socket.cpp
    ${AJABASE_ROOT}/network/udp_socket.cpp
    ${AJABASE_ROOT}/persistence/persistence.cpp
    ${AJABASE_ROOT}/persistence/sqlite3.c
    ${AJABASE_ROOT}/pnp/pnp.cpp
    ${AJABASE_ROOT}/system/atomic.cpp
    ${AJABASE_ROOT}/system/debug.cpp
    ${AJABASE_ROOT}/system/diskstatus.cpp
    ${AJABASE_ROOT}/system/event.cpp
    ${AJABASE_ROOT}/system/file_io.cpp
    ${AJABASE_ROOT}/system/info.cpp
    ${AJABASE_ROOT}/system/lock.cpp
    ${AJABASE_ROOT}/system/log.cpp
    ${AJABASE_ROOT}/system/memory.cpp
    ${AJABASE_ROOT}/system/process.cpp
    ${AJABASE_ROOT}/system/system.cpp
    ${AJABASE_ROOT}/system/systemtime.cpp
    ${AJABASE_ROOT}/system/thread.cpp)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_definitions(
        -DMSWindows
        -DAJA_WINDOWS
        -D_MBCS
        -D_CRT_SECURE_NO_WARNINGS
        -DCRT_NONSTDC_DEPRECATE
        -D_WINSOCK_DEPRECATED_NO_WARNINGS
        -DAJA_NO_AUTOIMPORT
        -DWIN32_LEAN_AND_MEAN)

    set(EXTRA_STATIC_DEFINES
        -DAJASTATIC)

    set(EXTRA_DYNAMIC_DEFINES
        -DAJADLL
        -DAJA_WINDLL
        -DAJADLL_BUILD)

    set(AJANTV2_TARGET_INCLUDE_DIRS
        ${AJANTV2_TARGET_INCLUDE_DIRS}
        src/win
        ${AJABASE_ROOT}/pnp/windows
        ${AJABASE_ROOT}/system/windows)

    set(AJANTV2_HEADERS
        ${AJANTV2_HEADERS}
        src/win/ntv2windriverinterface.h
        src/win/ntv2winpublicinterface.h)

    set(AJANTV2_SOURCES
        ${AJANTV2_SOURCES}
        # src/win/dllmain.cpp
        src/win/ntv2windriverinterface.cpp)

    set(AJABASE_HEADERS
        ${AJABASE_HEADERS}
        ${AJABASE_ROOT}/pnp/windows/pnpimpl.h
        ${AJABASE_ROOT}/system/windows/eventimpl.h
        ${AJABASE_ROOT}/system/windows/infoimpl.h
        ${AJABASE_ROOT}/system/windows/lockimpl.h
        ${AJABASE_ROOT}/system/windows/processimpl.h
        ${AJABASE_ROOT}/system/windows/threadimpl.h)

    set(AJABASE_SOURCES
        ${AJABASE_SOURCES}
        ${AJABASE_ROOT}/pnp/windows/pnpimpl.cpp
        ${AJABASE_ROOT}/system/windows/eventimpl.cpp
        ${AJABASE_ROOT}/system/windows/infoimpl.cpp
        ${AJABASE_ROOT}/system/windows/lockimpl.cpp
        ${AJABASE_ROOT}/system/windows/processimpl.cpp
        ${AJABASE_ROOT}/system/windows/threadimpl.cpp)

    set(EXTRA_LIBRARIES
        advapi32 comctl32 netapi32
        odbc32 psapi rpcrt4 setupapi shell32
        shlwapi user32 winmm ws2_32 wsock32)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_definitions(
        -DAJALinux
        -DAJA_LINUX)
    set(EXTRA_STATIC_DEFINES
        -DAJASTATIC)
    set(EXTRA_DYNAMIC_DEFINES
        -DAJADLL_BUILD
        -DAJADLL_BUILD)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

    set(AJANTV2_TARGET_INCLUDE_DIRS
        ${AJANTV2_TARGET_INCLUDE_DIRS}
        src/lin
        ${AJABASE_ROOT}/pnp/linux
        ${AJABASE_ROOT}/system/linux)

    set(AJANTV2_HEADERS
        ${AJANTV2_HEADERS}
        src/lin/ntv2linuxdriverinterface.h
        src/lin/ntv2linuxpublicinterface.h)

    set(AJANTV2_SOURCES
        ${AJANTV2_SOURCES}
        src/lin/ntv2linuxdriverinterface.cpp)

    set(AJABASE_HEADERS
        ${AJABASE_HEADERS}
        ${AJABASE_ROOT}/pnp/linux/pnpimpl.h
        ${AJABASE_ROOT}/system/linux/eventimpl.h
        ${AJABASE_ROOT}/system/linux/infoimpl.h
        ${AJABASE_ROOT}/system/linux/lockimpl.h
        ${AJABASE_ROOT}/system/linux/processimpl.h
        ${AJABASE_ROOT}/system/linux/threadimpl.h)

    set(AJABASE_SOURCES
        ${AJABASE_SOURCES}
        ${AJABASE_ROOT}/pnp/linux/pnpimpl.cpp
        ${AJABASE_ROOT}/system/linux/eventimpl.cpp
        ${AJABASE_ROOT}/system/linux/infoimpl.cpp
        ${AJABASE_ROOT}/system/linux/lockimpl.cpp
        ${AJABASE_ROOT}/system/linux/processimpl.cpp
        ${AJABASE_ROOT}/system/linux/threadimpl.cpp)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    add_definitions(
        -DAJAMac
        -DAJA_MAC)

    set(EXTRA_STATIC_DEFINES
        -DAJASTATIC)

    set(EXTRA_DYNAMIC_DEFINES
        -DAJADLL_BUILD
        -DAJADLL_BUILD)

    set(AJANTV2_TARGET_INCLUDE_DIRS
        ${AJANTV2_TARGET_INCLUDE_DIRS}
        src/mac
        ${AJABASE_ROOT}/pnp/mac
        ${AJABASE_ROOT}/system/mac
        /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include)

    set(AJANTV2_HEADERS
        ${AJANTV2_HEADERS}
        src/mac/ntv2macdriverinterface.h
        src/mac/ntv2macpublicinterface.h)

    set(AJANTV2_SOURCES
        ${AJANTV2_SOURCES}
        src/mac/ntv2macdriverinterface.cpp)

    set(AJABASE_HEADERS
        ${AJABASE_HEADERS}
        ${AJABASE_ROOT}/pnp/mac/devicenotifier.h
        ${AJABASE_ROOT}/pnp/mac/masterport.h
        ${AJABASE_ROOT}/pnp/mac/pnpimpl.h
        ${AJABASE_ROOT}/system/mac/eventimpl.h
        ${AJABASE_ROOT}/system/mac/infoimpl.h
        ${AJABASE_ROOT}/system/mac/lockimpl.h
        ${AJABASE_ROOT}/system/mac/processimpl.h
        ${AJABASE_ROOT}/system/mac/pthreadsextra.h
        ${AJABASE_ROOT}/system/mac/threadimpl.h)

    set(AJABASE_SOURCES
        ${AJABASE_SOURCES}
        ${AJABASE_ROOT}/pnp/mac/devicenotifier.cpp
        ${AJABASE_ROOT}/pnp/mac/masterport.cpp
        ${AJABASE_ROOT}/pnp/mac/pnpimpl.cpp
        ${AJABASE_ROOT}/system/mac/eventimpl.cpp
        ${AJABASE_ROOT}/system/mac/infoimpl.mm
        ${AJABASE_ROOT}/system/mac/lockimpl.cpp
        ${AJABASE_ROOT}/system/mac/processimpl.cpp
        ${AJABASE_ROOT}/system/mac/pthreadsextra.cpp
        ${AJABASE_ROOT}/system/mac/threadimpl.cpp)

    find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation)
    find_library(FOUNDATION_FRAMEWORK Foundation)
    find_library(IOKIT_FRAMEWORK IoKit)

    set(EXTRA_LIBRARIES
        ${CORE_FOUNDATION_FRAMEWORK}
        ${IOKIT_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK})
endif()

set(AJANTV2_TARGET_SOURCES
    ${AJANTV2_SOURCES}
    ${AJANTV2_HEADERS}
    ${AJABASE_SOURCES}
    ${AJABASE_HEADERS})

add_library(ajantv2shared SHARED ${AJANTV2_TARGET_SOURCES})
add_library(ajantv2 STATIC ${AJANTV2_TARGET_SOURCES})

target_include_directories(ajantv2shared PUBLIC ${AJANTV2_TARGET_INCLUDE_DIRS})
target_include_directories(ajantv2 PUBLIC ${AJANTV2_TARGET_INCLUDE_DIRS})

target_link_libraries(ajantv2shared PUBLIC ${EXTRA_LIBRARIES} ${EXTRA_DYNAMIC_LIBRARIES})
target_link_libraries(ajantv2 PUBLIC ${EXTRA_LIBRARIES})

target_compile_definitions(ajantv2shared PUBLIC ${EXTRA_DYNAMIC_DEFINES})
target_compile_definitions(ajantv2 PUBLIC ${EXTRA_STATIC_DEFINES})
