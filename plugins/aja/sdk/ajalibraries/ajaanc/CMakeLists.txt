project(ajaanc)

set(NTV2_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(AJALIBRARIES_ROOT ${NTV2_ROOT}/ajalibraries)
set(AJAANC_ROOT ${AJALIBRARIES_ROOT}/ajaanc)
set(AJABASE_ROOT ${AJALIBRARIES_ROOT}/ajabase)
set(AJANTV2_ROOT ${AJALIBRARIES_ROOT}/ajantv2)

set(AJAANC_TARGET_INCLUDE_DIRS
    ${AJAANC_ROOT}/includes
    ${AJABASE_ROOT}/includes
    ${AJANTV2_ROOT}/includes
    ${AJALIBRARIES_ROOT})

set(AJAANC_HEADERS
    ${AJAANC_ROOT}/includes/
    ${AJAANC_ROOT}/includes/ancillarydata_cea608.h
    ${AJAANC_ROOT}/includes/ancillarydata_cea608_line21.h
    ${AJAANC_ROOT}/includes/ancillarydata_cea608_vanc.h
    ${AJAANC_ROOT}/includes/ancillarydata_cea708.h
    ${AJAANC_ROOT}/includes/ancillarydatafactory.h
    ${AJAANC_ROOT}/includes/ancillarydata_framestatusinfo524D.h
    ${AJAANC_ROOT}/includes/ancillarydata_framestatusinfo5251.h
    ${AJAANC_ROOT}/includes/ancillarydata.h
    ${AJAANC_ROOT}/includes/ancillarydata_hdr_hdr10.h
    ${AJAANC_ROOT}/includes/ancillarydata_hdr_hlg.h
    ${AJAANC_ROOT}/includes/ancillarydata_hdr_sdr.h
    ${AJAANC_ROOT}/includes/ancillarydata_timecode_atc.h
    ${AJAANC_ROOT}/includes/ancillarydata_timecode.h
    ${AJAANC_ROOT}/includes/ancillarydata_timecode_vitc.h
    ${AJAANC_ROOT}/includes/ancillarylist.h
)

set(AJAANC_SOURCES
    ${AJAANC_ROOT}/src/ancillarydata_cea608.cpp
    ${AJAANC_ROOT}/src/ancillarydata_cea608_line21.cpp
    ${AJAANC_ROOT}/src/ancillarydata_cea608_vanc.cpp
    ${AJAANC_ROOT}/src/ancillarydata_cea708.cpp
    ${AJAANC_ROOT}/src/ancillarydata.cpp
    ${AJAANC_ROOT}/src/ancillarydatafactory.cpp
    ${AJAANC_ROOT}/src/ancillarydata_framestatusinfo524D.cpp
    ${AJAANC_ROOT}/src/ancillarydata_framestatusinfo5251.cpp
    ${AJAANC_ROOT}/src/ancillarydata_hdr_hdr10.cpp
    ${AJAANC_ROOT}/src/ancillarydata_hdr_hlg.cpp
    ${AJAANC_ROOT}/src/ancillarydata_hdr_sdr.cpp
    ${AJAANC_ROOT}/src/ancillarydata_timecode_atc.cpp
    ${AJAANC_ROOT}/src/ancillarydata_timecode.cpp
    ${AJAANC_ROOT}/src/ancillarydata_timecode_vitc.cpp
    ${AJAANC_ROOT}/src/ancillarylist.cpp
)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_definitions(
        -DMSWindows
        -DAJA_WINDOWS)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_definitions(
        -DAJALinux
        -DAJA_LINUX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    add_definitions(
        -DAJAMac
        -DAJA_MAC)
endif()

set(AJAANC_TARGET_SOURCES ${AJAANC_SOURCES} ${AJAANC_HEADERS})

add_library(ajaancshared SHARED ${AJAANC_TARGET_SOURCES})
add_library(ajaanc STATIC ${AJAANC_TARGET_SOURCES})

target_include_directories(ajaancshared PUBLIC ${AJAANC_TARGET_INCLUDE_DIRS})
target_include_directories(ajaanc PUBLIC ${AJAANC_TARGET_INCLUDE_DIRS})

target_link_libraries(ajaancshared PUBLIC ajantv2shared)
target_link_libraries(ajaanc PUBLIC ajantv2)