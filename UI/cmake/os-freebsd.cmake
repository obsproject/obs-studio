target_sources(obs-studio PRIVATE platform-x11.cpp)
target_compile_definitions(obs-studio PRIVATE OBS_INSTALL_PREFIX="${OBS_INSTALL_PREFIX}")
target_link_libraries(obs-studio PRIVATE Qt::GuiPrivate Qt::DBus procstat)

find_package(glib REQUIRED)
target_link_libraries(obs-studio PRIVATE glib::glib)

target_sources(obs-studio PRIVATE system-info-posix.cpp)

if(TARGET OBS::python)
  find_package(Python REQUIRED COMPONENTS Interpreter Development)
  target_link_libraries(obs-studio PRIVATE Python::Python)
  target_link_options(obs-studio PRIVATE LINKER:-no-as-needed)
endif()

configure_file(cmake/linux/com.obsproject.Studio.metainfo.xml.in com.obsproject.Studio.metainfo.xml)

install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/com.obsproject.Studio.metainfo.xml"
  DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/metainfo"
)

install(FILES cmake/linux/com.obsproject.Studio.desktop DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications")

install(
  FILES cmake/linux/icons/obs-logo-128.png
  DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/128x128/apps"
  RENAME com.obsproject.Studio.png
)

install(
  FILES cmake/linux/icons/obs-logo-256.png
  DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/256x256/apps"
  RENAME com.obsproject.Studio.png
)

install(
  FILES cmake/linux/icons/obs-logo-512.png
  DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/512x512/apps"
  RENAME com.obsproject.Studio.png
)

install(
  FILES cmake/linux/icons/obs-logo-scalable.svg
  DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps"
  RENAME com.obsproject.Studio.svg
)
