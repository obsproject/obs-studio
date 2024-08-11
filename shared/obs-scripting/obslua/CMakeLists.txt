cmake_minimum_required(VERSION 3.22...3.25)

legacy_check()

if(POLICY CMP0078)
  cmake_policy(SET CMP0078 NEW)
endif()

if(POLICY CMP0086)
  cmake_policy(SET CMP0086 NEW)
endif()

find_package(Luajit REQUIRED)
find_package(SWIG 4 REQUIRED)

include(UseSWIG)

set_source_files_properties(obslua.i PROPERTIES USE_TARGET_INCLUDE_DIRECTORIES TRUE)

swig_add_library(
  obslua
  LANGUAGE lua
  TYPE MODULE
  SOURCES obslua.i)

add_library(OBS::lua ALIAS obslua)

target_compile_options(
  obslua
  PRIVATE $<$<PLATFORM_ID:Windows>:/wd4054>
          $<$<PLATFORM_ID:Windows>:/wd4197>
          $<$<PLATFORM_ID:Windows>:/wd4244>
          $<$<PLATFORM_ID:Windows>:/wd4267>
          $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang>:-Wno-shorten-64-to-32>
          $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang>:-Wno-unreachable-code>
          $<$<COMPILE_LANG_AND_ID:C,GNU>:-Wno-maybe-uninitialized>)

target_compile_definitions(obslua PRIVATE SWIG_TYPE_TABLE=obslua SWIG_LUA_INTERPRETER_NO_DEBUG
                                          $<$<BOOL:${ENABLE_UI}>:ENABLE_UI>)

target_link_libraries(obslua PRIVATE OBS::cstrcache OBS::libobs OBS::scripting Luajit::Luajit
                                     $<$<BOOL:${ENABLE_UI}>:OBS::frontend-api>)

set_property(
  TARGET obslua
  APPEND
  PROPERTY SWIG_COMPILE_DEFINITIONS "SWIG_TYPE_TABLE=obslua" "SWIG_LUA_INTERPRETER_NO_DEBUG"
           "$<$<BOOL:${ENABLE_UI}>:ENABLE_UI>")

# cmake-format: off
set_target_properties_obs(obslua PROPERTIES FOLDER scripting XCODE_ATTRIBUTE_STRIP_STYLE non-global)
# cmake-format: on
