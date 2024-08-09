include(cmake/feature-sparkle.cmake)

target_sources(obs-studio PRIVATE platform-osx.mm forms/OBSPermissions.ui window-permissions.cpp window-permissions.hpp)
target_compile_options(obs-studio PRIVATE -Wno-quoted-include-in-framework-header -Wno-comma)

target_sources(obs-studio PRIVATE system-info-macos.mm)

set_source_files_properties(platform-osx.mm PROPERTIES COMPILE_FLAGS -fobjc-arc)

if(CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 14.0.3)
  target_compile_options(obs-studio PRIVATE -Wno-error=unqualified-std-cast-call)
endif()

# cmake-format: off
target_link_libraries(
  obs-studio
  PRIVATE "$<LINK_LIBRARY:FRAMEWORK,AppKit.framework>"
          "$<LINK_LIBRARY:FRAMEWORK,ApplicationServices.framework>"
          "$<LINK_LIBRARY:FRAMEWORK,AVFoundation.framework>")
# cmake-format: on
