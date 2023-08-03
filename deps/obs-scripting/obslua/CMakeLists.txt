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

target_compile_options(obslua PRIVATE "$<$<C_COMPILER_ID:AppleClang,Clang>:-Wno-error=shorten-64-to-32>")
target_compile_definitions(obslua PRIVATE SWIG_TYPE_TABLE=obslua SWIG_LUA_INTERPRETER_NO_DEBUG
                                          $<$<BOOL:${ENABLE_UI}>:ENABLE_UI>)

target_link_libraries(obslua PRIVATE OBS::scripting OBS::cstrcache OBS::libobs Luajit::Luajit
                                     $<$<BOOL:ENABLE_UI>:OBS::frontend-api>)

if(OS_WINDOWS)
  target_compile_options(obslua PRIVATE /wd4054 /wd4197 /wd4244 /wd4267)
elseif(OS_MACOS)
  set_property(TARGET obslua PROPERTY XCODE_ATTRIBUTE_STRIP_STYLE non-global)
  target_compile_options(obslua PRIVATE -Wno-unreachable-code)
elseif(OS_LINUX OR OS_FREEBSD)
  target_compile_options(obslua PRIVATE -Wno-maybe-uninitialized)
endif()

set_property(
  TARGET obslua
  APPEND
  PROPERTY SWIG_COMPILE_DEFINITIONS "SWIG_TYPE_TABLE=obslua" "SWIG_LUA_INTERPRETER_NO_DEBUG"
           "$<$<BOOL:${ENABLE_UI}>:ENABLE_UI>")

set_target_properties_obs(obslua PROPERTIES FOLDER scripting)
