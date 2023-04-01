cmake_minimum_required(VERSION 3.22...3.25)

option(ENABLE_SCRIPTING_PYTHON "Enable Python scripting support" ON)

if(ENABLE_SCRIPTING_PYTHON)
  add_subdirectory(obspython)
  find_package(Python 3.8...<3.12 COMPONENTS Interpreter Development)
else()
  target_disable_feature(obs-scripting "Python scripting support")
endif()

target_sources(obs-scripting PRIVATE obs-scripting-python.c obs-scripting-python.h obs-scripting-python-import.h)

target_compile_definitions(
  obs-scripting
  PRIVATE ENABLE_SCRIPTING PYTHON_LIB="$<TARGET_LINKER_FILE_NAME:Python::Python>"
  PUBLIC Python_FOUND)

add_custom_command(
  OUTPUT swig/swigpyrun.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory swig
  COMMAND ${CMAKE_COMMAND} -E env "SWIG_LIB=${SWIG_DIR}" ${SWIG_EXECUTABLE} -python
          $<IF:$<BOOL:${OS_LINUX}>,-py3,-py3-stable-abi> -external-runtime swig/swigpyrun.h
  COMMENT "obs-scripting - generating Python 3 SWIG interface headers")

target_sources(obs-scripting PRIVATE swig/swigpyrun.h)
set_source_files_properties(swig/swigpyrun.h PROPERTIES GENERATED ON)

if(OS_WINDOWS)
  target_sources(obs-scripting PRIVATE obs-scripting-python-import.c)

  get_target_property(_python_include_directory Python::Python INTERFACE_INCLUDE_DIRECTORIES)
  target_include_directories(obs-scripting PRIVATE ${_python_include_directory})
elseif(OS_MACOS)
  target_sources(obs-scripting PRIVATE obs-scripting-python-import.c)

  get_target_property(_python_include_directory Python::Python INTERFACE_INCLUDE_DIRECTORIES)
  target_include_directories(obs-scripting PRIVATE ${_python_include_directory})

  target_link_options(obs-scripting PRIVATE LINKER:-undefined LINKER:dynamic_lookup)
elseif(OS_LINUX OR OS_FREEBSD)
  target_link_libraries(obs-scripting PRIVATE Python::Python)
endif()
