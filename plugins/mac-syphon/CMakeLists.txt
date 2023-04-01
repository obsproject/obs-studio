cmake_minimum_required(VERSION 3.22...3.25)

legacy_check()

option(ENABLE_SYPHON "Enable Syphon sharing support" ON)
if(NOT ENABLE_SYPHON)
  target_disable(mac-syphon)
  target_disable_feature(mac-syphon "Syphon sharing support")
  return()
else()
  target_enable_feature(mac-syphon "Syphon sharing support")
endif()

find_library(COCOA Cocoa)
find_library(IOSURF IOSurface)
find_library(SCRIPTINGBRIDGE ScriptingBridge)
mark_as_advanced(COCOA IOSURF SCRIPTINGBRIDGE)

add_library(syphon-framework STATIC EXCLUDE_FROM_ALL )
add_library(Syphon::Framework ALIAS syphon-framework)

target_sources(
  syphon-framework
  PRIVATE syphon-framework/Syphon_Prefix.pch
          syphon-framework/Syphon.h
          syphon-framework/SyphonBuildMacros.h
          syphon-framework/SyphonCFMessageReceiver.m
          syphon-framework/SyphonCFMessageReceiver.h
          syphon-framework/SyphonCFMessageSender.h
          syphon-framework/SyphonCFMessageSender.m
          syphon-framework/SyphonClient.m
          syphon-framework/SyphonClient.h
          syphon-framework/SyphonClientConnectionManager.m
          syphon-framework/SyphonClientConnectionManager.h
          syphon-framework/SyphonDispatch.c
          syphon-framework/SyphonDispatch.h
          syphon-framework/SyphonIOSurfaceImage.m
          syphon-framework/SyphonIOSurfaceImage.h
          syphon-framework/SyphonImage.m
          syphon-framework/SyphonImage.h
          syphon-framework/SyphonMachMessageReceiver.m
          syphon-framework/SyphonMachMessageReceiver.h
          syphon-framework/SyphonMachMessageSender.m
          syphon-framework/SyphonMachMessageSender.h
          syphon-framework/SyphonMessageQueue.m
          syphon-framework/SyphonMessageQueue.h
          syphon-framework/SyphonMessageReceiver.m
          syphon-framework/SyphonMessageReceiver.h
          syphon-framework/SyphonMessageSender.m
          syphon-framework/SyphonMessageSender.h
          syphon-framework/SyphonMessaging.m
          syphon-framework/SyphonMessaging.h
          syphon-framework/SyphonOpenGLFunctions.c
          syphon-framework/SyphonOpenGLFunctions.h
          syphon-framework/SyphonPrivate.m
          syphon-framework/SyphonPrivate.h
          syphon-framework/SyphonServer.m
          syphon-framework/SyphonServer.h
          syphon-framework/SyphonServerConnectionManager.m
          syphon-framework/SyphonServerConnectionManager.h
          syphon-framework/SyphonServerDirectory.m
          syphon-framework/SyphonServerDirectory.h)

target_compile_options(
  syphon-framework
  PRIVATE -include "${CMAKE_CURRENT_SOURCE_DIR}/syphon-framework/Syphon_Prefix.pch" -Wno-error=deprecated-declarations
  PUBLIC -Wno-error=newline-eof -Wno-error=deprecated-implementations)

target_compile_definitions(syphon-framework PUBLIC "SYPHON_UNIQUE_CLASS_NAME_PREFIX=OBS_")

target_link_libraries(syphon-framework PUBLIC OpenGL::GL ${COCOA} ${IOSURF})

set_target_properties(syphon-framework PROPERTIES FOLDER "plugins/mac-syphon" PREFIX "")

add_library(mac-syphon MODULE)
add_library(OBS::syphon ALIAS mac-syphon)

target_sources(mac-syphon PRIVATE syphon.m plugin-main.c)

target_compile_options(mac-syphon PRIVATE -include "${CMAKE_CURRENT_SOURCE_DIR}/syphon-framework/Syphon_Prefix.pch"
                                          -fobjc-arc)

target_link_libraries(mac-syphon PRIVATE OBS::libobs Syphon::Framework ${SCRIPTINGBRIDGE})

set_target_properties_obs(
  mac-syphon
  PROPERTIES FOLDER "plugins"
             PREFIX ""
             XCODE_ATTRIBUTE_CLANG_WARN_SUSPICIOUS_IMPLICIT_CONVERSION YES
             XCODE_ATTRIBUTE_GCC_WARN_SHADOW YES)
