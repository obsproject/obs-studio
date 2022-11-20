option(ENABLE_SCRIPTING_LUA "Enable Lua scripting support" ON)
option(ENABLE_SCRIPTING_PYTHON "Enable Python scripting support" ON)

if(OS_WINDOWS AND ARCH_ARM64)
  set(ENABLE_SCRIPTING_LUA OFF)
endif()

if(NOT ENABLE_SCRIPTING)
  obs_status(DISABLED "obs-scripting")
  return()
endif()

project(obs-scripting)

if(ENABLE_SCRIPTING_LUA)
  add_subdirectory(obslua)
  find_package(Luajit)

  if(NOT TARGET Luajit::Luajit)
    obs_status(FATAL_ERROR "obs-scripting -> Luajit not found.")
    return()
  else()
    obs_status(STATUS "obs-scripting -> Luajit found.")
  endif()
else()
  obs_status(DISABLED "Luajit support")
endif()

if(ENABLE_SCRIPTING_PYTHON)
  add_subdirectory(obspython)
  if(OS_WINDOWS)
    find_package(PythonWindows)
  else()
    find_package(Python COMPONENTS Interpreter Development)
  endif()

  if(NOT TARGET Python::Python)
    obs_status(FATAL_ERROR "obs-scripting -> Python not found.")
    return()
  else()
    obs_status(STATUS "obs-scripting -> Python ${Python_VERSION} found.")
  endif()
else()
  obs_status(DISABLED "Python support")
endif()

if(NOT TARGET Luajit::Luajit AND NOT TARGET Python::Python)
  obs_status(WARNING "obs-scripting -> No supported scripting libraries found.")
  return()
endif()

if(OS_MACOS)
  find_package(SWIG 4 REQUIRED)
elseif(OS_POSIX)
  find_package(SWIG 3 REQUIRED)
elseif(OS_WINDOWS)
  find_package(SwigWindows 3 REQUIRED)
endif()

add_library(obs-scripting SHARED)
add_library(OBS::scripting ALIAS obs-scripting)

target_sources(
  obs-scripting
  PUBLIC obs-scripting.h
  PRIVATE obs-scripting.c cstrcache.cpp cstrcache.h obs-scripting-logging.c
          obs-scripting-callback.h)

target_link_libraries(obs-scripting PRIVATE OBS::libobs)

target_compile_features(obs-scripting PRIVATE cxx_auto_type)

target_include_directories(obs-scripting PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                                                ${CMAKE_BINARY_DIR}/config)

if(OS_WINDOWS)
  set(MODULE_DESCRIPTION "OBS Studio scripting module")
  configure_file(${CMAKE_SOURCE_DIR}/cmake/bundle/windows/obs-module.rc.in
                 obs-scripting.rc)

  target_sources(obs-scripting PRIVATE obs-scripting.rc)

  target_link_libraries(obs-scripting PRIVATE OBS::w32-pthreads)

elseif(OS_MACOS)
  target_link_libraries(obs-scripting PRIVATE objc)
endif()

set_target_properties(
  obs-scripting
  PROPERTIES FOLDER "scripting"
             VERSION "${OBS_VERSION_MAJOR}"
             SOVERSION "1")

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/swig)

if(TARGET Luajit::Luajit)
  add_custom_command(
    OUTPUT swig/swigluarun.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E env "SWIG_LIB=${SWIG_DIR}" ${SWIG_EXECUTABLE}
            -lua -external-runtime swig/swigluarun.h
    COMMENT "obs-scripting - generating Luajit SWIG interface headers")

  set_source_files_properties(swig/swigluarun.h PROPERTIES GENERATED ON)

  target_link_libraries(obs-scripting PRIVATE Luajit::Luajit)

  target_sources(
    obs-scripting
    PRIVATE obs-scripting-lua.c obs-scripting-lua.h obs-scripting-lua-source.c
            ${CMAKE_CURRENT_BINARY_DIR}/swig/swigluarun.h)

  target_include_directories(obs-scripting PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

  if(ENABLE_UI)
    target_link_libraries(obs-scripting PRIVATE OBS::frontend-api)

    target_sources(obs-scripting PRIVATE obs-scripting-lua-frontend.c)

    target_compile_definitions(obs-scripting PRIVATE UI_ENABLED=ON)
  endif()

endif()

if(TARGET Python::Python)
  add_custom_command(
    OUTPUT swig/swigpyrun.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    PRE_BUILD
    COMMAND
      ${CMAKE_COMMAND} -E env "SWIG_LIB=${SWIG_DIR}" ${SWIG_EXECUTABLE} -python
      $<IF:$<AND:$<BOOL:${OS_POSIX}>,$<NOT:$<BOOL:${OS_MACOS}>>>,-py3,-py3-stable-abi>
      -external-runtime swig/swigpyrun.h
    COMMENT "obs-scripting - generating Python 3 SWIG interface headers")

  set_source_files_properties(swig/swigpyrun.h PROPERTIES GENERATED ON)

  target_sources(
    obs-scripting
    PRIVATE obs-scripting-python.c obs-scripting-python.h
            obs-scripting-python-import.h
            ${CMAKE_CURRENT_BINARY_DIR}/swig/swigpyrun.h)

  target_include_directories(obs-scripting PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

  get_filename_component(_PYTHON_PATH "${Python_LIBRARIES}" PATH)
  get_filename_component(_PYTHON_FILE "${Python_LIBRARIES}" NAME)

  string(REGEX REPLACE "\\.[^.]*$" "" _PYTHON_FILE ${_PYTHON_FILE})

  if(OS_WINDOWS)
    string(REGEX REPLACE "_d" "" _PYTHON_FILE ${_PYTHON_FILE})
  endif()
  set(OBS_SCRIPT_PYTHON_PATH "${_PYTHON_FILE}")

  unset(_PYTHON_FILE)
  unset(_PYTHON_PATH)

  if(OS_WINDOWS OR OS_MACOS)
    target_include_directories(obs-scripting PRIVATE ${Python_INCLUDE_DIRS})

    target_sources(obs-scripting PRIVATE obs-scripting-python-import.c)
    if(OS_MACOS)
      target_link_options(obs-scripting PRIVATE -undefined dynamic_lookup)
    endif()
  else()
    target_link_libraries(obs-scripting PRIVATE Python::Python)
  endif()

  if(ENABLE_UI)
    target_link_libraries(obs-scripting PRIVATE OBS::frontend-api)

    target_sources(obs-scripting PRIVATE obs-scripting-python-frontend.c)

    target_compile_definitions(obs-scripting PRIVATE UI_ENABLED=ON)
  endif()
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/obs-scripting-config.h.in
               ${CMAKE_BINARY_DIR}/config/obs-scripting-config.h)

target_sources(obs-scripting
               PUBLIC ${CMAKE_BINARY_DIR}/config/obs-scripting-config.h)

setup_binary_target(obs-scripting)
