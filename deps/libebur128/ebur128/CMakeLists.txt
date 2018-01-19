set(BUILD_STATIC_LIBS       ON  CACHE BOOL "Build static library")
set(WITH_STATIC_PIC         ON  CACHE BOOL "Compile static library with -fPIC flag")
set(ENABLE_INTERNAL_QUEUE_H ON CACHE BOOL "Use own queue.h")

#### queue.h
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/queuetest.c
"#include <sys/queue.h>\nLIST_HEAD(listhead, entry) head;\nint main() { return 0; }")
try_compile(HAS_QUEUE ${CMAKE_CURRENT_BINARY_DIR}
                      ${CMAKE_CURRENT_BINARY_DIR}/queuetest.c)

set(SUMMARY_HAS_QUEUE ${HAS_QUEUE} CACHE INTERNAL "")

if(ENABLE_INTERNAL_QUEUE_H)
  include_directories(SYSTEM queue)
endif()


if(MSVC)
  add_definitions(-D_USE_MATH_DEFINES)
  add_definitions(/arch:SSE2)
  add_definitions(-D __SSE2_MATH__)
endif()


set(EBUR128_VERSION_MAJOR 1)
set(EBUR128_VERSION 1.2.3)

#### static
if(BUILD_STATIC_LIBS)
  add_library(ebur128_static STATIC ebur128.c)
  if(NOT MSVC)
    set_property(TARGET ebur128_static PROPERTY OUTPUT_NAME ebur128)
  endif()
endif()

if(WITH_STATIC_PIC)
  set_property(TARGET ebur128_static PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()

#### shared
# set source file for library, which includes def file if using MSVC
set(EBUR128_SHARED_SOURCE ebur128.c)
if(MSVC)
  set(EBUR128_SHARED_SOURCE ${EBUR128_SHARED_SOURCE} ebur128.def)
endif()

add_library(ebur128 SHARED ${EBUR128_SHARED_SOURCE})
set_target_properties(ebur128 PROPERTIES
    SOVERSION ${EBUR128_VERSION_MAJOR}
    VERSION ${EBUR128_VERSION})

find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
  target_link_libraries(ebur128 ${MATH_LIBRARY})
endif()

set(EBUR128_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "")

install(FILES ebur128.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
if(BUILD_STATIC_LIBS)
  install(TARGETS ebur128 ebur128_static DESTINATION ${CMAKE_INSTALL_LIBDIR})
else()
  install(TARGETS ebur128 DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

#### pkg-config
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libebur128.pc.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/libebur128.pc @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libebur128.pc"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
