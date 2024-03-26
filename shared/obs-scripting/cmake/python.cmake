cmake_minimum_required(VERSION 3.22...3.25)

option(ENABLE_SCRIPTING_PYTHON "Enable Python scripting support" ON)

if(ENABLE_SCRIPTING_PYTHON)
  add_subdirectory(obspython)

  if(OS_WINDOWS)
    find_package(Python 3.8...<3.11 REQUIRED Interpreter Development)
  elseif(OS_LINUX OR OS_FREEBSD OR OS_OPENBSD)
    find_package(Python 3.8 REQUIRED Interpreter Development)
  else()
    find_package(Python 3.8...<3.12 REQUIRED Interpreter Development)
  endif()

  add_custom_command(
    OUTPUT swig/swigpyrun.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory swig
    COMMAND
      ${CMAKE_COMMAND} -E env "SWIG_LIB=${SWIG_DIR}" ${SWIG_EXECUTABLE} -python
      $<$<PLATFORM_ID:Windows,Darwin>:-py3-stable-abi> -external-runtime swig/swigpyrun.h
    COMMENT "obs-scripting - generating Python 3 SWIG interface headers"
  )

  set_source_files_properties(swig/swigpyrun.h PROPERTIES GENERATED TRUE)

  target_sources(
    obs-scripting
    PRIVATE
      $<$<BOOL:${ENABLE_UI}>:obs-scripting-python-frontend.c>
      $<$<PLATFORM_ID:Windows,Darwin>:obs-scripting-python-import.c>
      obs-scripting-python-import.h
      obs-scripting-python.c
      obs-scripting-python.h
      swig/swigpyrun.h
  )

  target_compile_definitions(
    obs-scripting
    PRIVATE ENABLE_SCRIPTING PYTHON_LIB="$<TARGET_LINKER_FILE_NAME:Python::Python>"
    PUBLIC Python_FOUND
  )

  target_include_directories(
    obs-scripting
    PRIVATE "$<$<PLATFORM_ID:Windows,Darwin>:$<TARGET_PROPERTY:Python::Python,INTERFACE_INCLUDE_DIRECTORIES>>"
  )

  target_link_libraries(obs-scripting PRIVATE $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:Python::Python>)

  target_link_options(obs-scripting PRIVATE $<$<PLATFORM_ID:Darwin>:LINKER:-undefined,dynamic_lookup>)
else()
  target_disable_feature(obs-scripting "Python scripting support")
endif()
