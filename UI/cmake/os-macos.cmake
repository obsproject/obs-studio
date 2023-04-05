if(NOT XCODE)
  target_add_resource(obs-studio "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos/Assets.xcassets")
endif()

find_library(APPKIT Appkit)
find_library(AVFOUNDATION AVFoundation)
find_library(APPLICATIONSERVICES ApplicationServices)
mark_as_advanced(APPKIT AVFOUNDATION APPLICATIONSERVICES)

target_sources(obs-studio PRIVATE platform-osx.mm forms/OBSPermissions.ui window-permissions.cpp window-permissions.hpp)

set_source_files_properties(platform-osx.mm PROPERTIES COMPILE_FLAGS -fobjc-arc)

target_link_libraries(obs-studio PRIVATE ${APPKIT} ${AVFOUNDATION} ${APPLICATIONSERVICES})

target_compile_options(
  obs-studio PRIVATE -Wno-error=float-conversion -Wno-error=implicit-int-conversion -Wno-error=shorten-64-to-32
                     -Wno-quoted-include-in-framework-header -Wno-comma)

if(CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 14.0.3)
  target_compile_options(obs-studio PRIVATE -Wno-error=unqualified-std-cast-call)
endif()
