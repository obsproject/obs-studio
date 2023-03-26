target_sources(obs-studio PRIVATE platform-x11.cpp)
target_compile_definitions(obs-studio PRIVATE OBS_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")
target_link_libraries(obs-studio PRIVATE Qt::GuiPrivate procstat)

if(TARGET OBS::python)
  find_package(Python REQUIRED COMPONENTS Interpreter Development)
  target_link_libraries(obs-studio PRIVATE Python::Python)
  target_link_options(obs-studio PRIVATE LINKER:-no-as-needed)
endif()
