cmake_minimum_required(VERSION 3.22...3.25)

option(ENABLE_SCRIPTING_LUA "Enable Lua scripting support" ON)

if(ENABLE_SCRIPTING_LUA)
  add_subdirectory(obslua)
  find_package(Luajit REQUIRED)
else()
  target_disable_feature(obs-scripting "Lua scripting support")
endif()

target_sources(obs-scripting PRIVATE obs-scripting-lua.c obs-scripting-lua.h obs-scripting-lua-source.c)
target_compile_definitions(obs-scripting PUBLIC LUAJIT_FOUND)

add_custom_command(
  OUTPUT swig/swigluarun.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory swig
  COMMAND ${CMAKE_COMMAND} -E env "SWIG_LIB=${SWIG_DIR}" ${SWIG_EXECUTABLE} -lua -external-runtime swig/swigluarun.h
  COMMENT "obs-scripting - generating Luajit SWIG interface headers")

target_sources(obs-scripting PRIVATE swig/swigluarun.h)
set_source_files_properties(swig/swigluarun.h PROPERTIES GENERATED ON)
set_source_files_properties(
  obs-scripting-lua.c obs-scripting-lua-source.c
  PROPERTIES COMPILE_OPTIONS "$<$<C_COMPILER_ID:AppleClang,Clang>:-Wno-error=shorten-64-to-32;-Wno-error=shadow>")

target_link_libraries(obs-scripting PRIVATE Luajit::Luajit)
