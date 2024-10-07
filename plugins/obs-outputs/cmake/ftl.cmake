find_package(CURL REQUIRED)
find_package(jansson REQUIRED)

add_library(ftl-sdk OBJECT)
add_library(OBS::ftl-sdk ALIAS ftl-sdk)

target_compile_definitions(ftl-sdk PUBLIC FTL_FOUND FTL_STATIC_COMPILE)

target_link_libraries(ftl-sdk PRIVATE jansson::jansson CURL::libcurl)

target_sources(
  ftl-sdk
  PRIVATE # cmake-format: sortable
          $<$<NOT:$<PLATFORM_ID:Windows>>:ftl-sdk/libftl/posix/socket.c>
          $<$<NOT:$<PLATFORM_ID:Windows>>:ftl-sdk/libftl/posix/threads.c>
          $<$<NOT:$<PLATFORM_ID:Windows>>:ftl-sdk/libftl/posix/threads.h>
          $<$<PLATFORM_ID:Windows>:ftl-sdk/libftl/win32/socket.c>
          $<$<PLATFORM_ID:Windows>:ftl-sdk/libftl/win32/threads.c>
          $<$<PLATFORM_ID:Windows>:ftl-sdk/libftl/win32/threads.h>
          ftl-sdk/libftl/ftl-sdk.c
          ftl-sdk/libftl/ftl_helpers.c
          ftl-sdk/libftl/ftl_private.h
          ftl-sdk/libftl/gettimeofday/gettimeofday.c
          ftl-sdk/libftl/gettimeofday/gettimeofday.h
          ftl-sdk/libftl/handshake.c
          ftl-sdk/libftl/hmac/hmac.c
          ftl-sdk/libftl/hmac/hmac.h
          ftl-sdk/libftl/hmac/sha2.c
          ftl-sdk/libftl/hmac/sha2.h
          ftl-sdk/libftl/ingest.c
          ftl-sdk/libftl/logging.c
          ftl-sdk/libftl/media.c
  PUBLIC ftl-sdk/libftl/ftl.h)

target_compile_options(
  ftl-sdk
  PRIVATE $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang,GNU>:-Wno-unused-parameter>
          $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang,GNU>:-Wno-unused-variable>
          $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang,GNU>:-Wno-sign-compare>
          $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang,GNU>:-Wno-pointer-sign>
          $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang,GNU>:-Wno-int-conversion>
          $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang>:-Wno-incompatible-function-pointer-types>
          $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang>:-Wno-implicit-int-conversion>
          $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang>:-Wno-shorten-64-to-32>
          $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang>:-Wno-macro-redefined>
          $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang>:-Wno-enum-conversion>
          $<$<COMPILE_LANG_AND_ID:C,GNU>:-Wno-extra>
          $<$<COMPILE_LANG_AND_ID:C,GNU>:-Wno-incompatible-pointer-types>
          $<$<COMPILE_LANG_AND_ID:C,GNU>:-Wno-builtin-macro-redefined>)

target_include_directories(
  ftl-sdk
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/ftl-sdk/libftl"
         "$<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/ftl-sdk/libftl/win32>"
         "$<$<NOT:$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/ftl-sdk/libftl/posix>")

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" AND CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 10)
  target_compile_options(ftl-sdk PRIVATE -Wno-error=enum-conversion -Wno-error=maybe-uninitialized)
endif()

target_sources(obs-outputs PRIVATE ftl-stream.c)
target_link_libraries(obs-outputs PRIVATE ftl-sdk)
target_enable_feature(obs-outputs "FTL protocol support")

set_target_properties(ftl-sdk PROPERTIES FOLDER plugins/obs-outputs POSITION_INDEPENDENT_CODE TRUE)
