include_directories(SYSTEM ${obs_SOURCE_DIR}/libobs)

find_library(COCOA Cocoa)
include_directories(${COCOA})

link_libraries(libobs ${COCOA})

add_executable(test test.mm)
