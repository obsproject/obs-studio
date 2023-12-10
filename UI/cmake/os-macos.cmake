if(NOT XCODE)
  target_add_resource(obs-studio "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos/Assets.xcassets")
endif()

target_sources(obs-studio PRIVATE platform-osx.mm forms/OBSPermissions.ui window-permissions.cpp window-permissions.hpp)
target_compile_options(
  obs-studio PRIVATE -Wno-error=float-conversion -Wno-error=implicit-int-conversion -Wno-error=shorten-64-to-32
                     -Wno-quoted-include-in-framework-header -Wno-comma)

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
