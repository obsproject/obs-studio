# Install script for directory: /home/subtomic/Documents/GitHub/Neural-Studio/build/_deps/openxr-sdk-src/src/loader

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "/home/subtomic/Documents/GitHub/Neural-Studio/build/_deps/openxr-sdk-build/src/loader/openxr.pc")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Loader" OR NOT CMAKE_INSTALL_COMPONENT)
  foreach(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libopenxr_loader.so.1.0.32"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libopenxr_loader.so.1"
      )
    if(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      file(RPATH_CHECK
           FILE "${file}"
           RPATH "")
    endif()
  endforeach()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES
    "/home/subtomic/Documents/GitHub/Neural-Studio/build/_deps/openxr-sdk-build/src/loader/libopenxr_loader.so.1.0.32"
    "/home/subtomic/Documents/GitHub/Neural-Studio/build/_deps/openxr-sdk-build/src/loader/libopenxr_loader.so.1"
    )
  foreach(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libopenxr_loader.so.1.0.32"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libopenxr_loader.so.1"
      )
    if(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "${file}")
      endif()
    endif()
  endforeach()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Loader" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/home/subtomic/Documents/GitHub/Neural-Studio/build/_deps/openxr-sdk-build/src/loader/libopenxr_loader.so")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "CMakeConfigs" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/openxr/OpenXRTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/openxr/OpenXRTargets.cmake"
         "/home/subtomic/Documents/GitHub/Neural-Studio/build/_deps/openxr-sdk-build/src/loader/CMakeFiles/Export/30f7f7b4fca785de0f5072a0d39157da/OpenXRTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/openxr/OpenXRTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/openxr/OpenXRTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/openxr" TYPE FILE FILES "/home/subtomic/Documents/GitHub/Neural-Studio/build/_deps/openxr-sdk-build/src/loader/CMakeFiles/Export/30f7f7b4fca785de0f5072a0d39157da/OpenXRTargets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^()$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/openxr" TYPE FILE FILES "/home/subtomic/Documents/GitHub/Neural-Studio/build/_deps/openxr-sdk-build/src/loader/CMakeFiles/Export/30f7f7b4fca785de0f5072a0d39157da/OpenXRTargets-noconfig.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "CMakeConfigs" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/openxr" TYPE FILE FILES
    "/home/subtomic/Documents/GitHub/Neural-Studio/build/_deps/openxr-sdk-build/src/loader/OpenXRConfig.cmake"
    "/home/subtomic/Documents/GitHub/Neural-Studio/build/_deps/openxr-sdk-build/src/loader/OpenXRConfigVersion.cmake"
    )
endif()

