if(NOT DEFINED APPDATA_RELEASE_DATE)
  if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
      COMMAND git log --tags -1 --pretty=%cd --date=short
      OUTPUT_VARIABLE APPDATA_RELEASE_DATE
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      OUTPUT_STRIP_TRAILING_WHITESPACE)
  else()
    file(TIMESTAMP "${CMAKE_SOURCE_DIR}/CMakeLists.txt" APPDATA_RELEASE_DATE "%Y-%m-%d")
  endif()
endif()

if(NOT DEFINED GIT_HASH)
  if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
      COMMAND git rev-parse HEAD
      OUTPUT_VARIABLE GIT_HASH
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      OUTPUT_STRIP_TRAILING_WHITESPACE)
  else()
    set(GIT_HASH "master")
  endif()
endif()

configure_file(com.obsproject.Studio.metainfo.xml.in com.obsproject.Studio.metainfo.xml)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/com.obsproject.Studio.metainfo.xml
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo)

install(FILES com.obsproject.Studio.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)

install(
  FILES icons/obs-logo-128.png
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/128x128/apps
  RENAME com.obsproject.Studio.png)

install(
  FILES icons/obs-logo-256.png
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/256x256/apps
  RENAME com.obsproject.Studio.png)

install(
  FILES icons/obs-logo-512.png
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/512x512/apps
  RENAME com.obsproject.Studio.png)

install(
  FILES icons/obs-logo-scalable.svg
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps
  RENAME com.obsproject.Studio.svg)
