find_package(nlohmann_json 3.11 REQUIRED)

set(OBS_PLATFORM_INSTALL_PATH "${CMAKE_INSTALL_PREFIX}/")
set(OBS_PLATFORM_LIBRARY_PATH "${CMAKE_INSTALL_LIBDIR}/")
set(OBS_PLATFORM_DATA_PATH "${CMAKE_INSTALL_DATAROOTDIR}/")
set(OBS_PLATFORM_CORE_MODULE_PATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/obs-modules/core")
set(OBS_PLATFORM_CORE_DATA_PATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/obs/obs-modules/core")
set(OBS_PLATFORM_PLUGIN_MODULE_PATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/obs-modules/plugins")
set(OBS_PLATFORM_PLUGIN_DATA_PATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/obs/obs-modules/plugins")

target_sources(
  obs-studio
  PRIVATE
    plugin-manager/PluginManager.cpp
    plugin-manager/PluginManager.hpp
    plugin-manager/PluginManagerWindow.cpp
    plugin-manager/PluginManagerWindow.hpp
    plugin-manager/PluginModuleLoader.hpp
)

if(OS_WINDOWS)
  target_sources(obs-studio PRIVATE plugin-manager/PluginModuleLoader_Windows.cpp)
elseif(OS_MACOS)
  target_sources(obs-studio PRIVATE plugin-manager/PluginModuleLoader_MacOS.mm)
elseif(OS_LINUX)
  configure_file(plugin-manager/LoaderPaths_Linux.hpp.in LoaderPaths_Linux.hpp @ONLY)
  target_sources(obs-studio PRIVATE plugin-manager/PluginModuleLoader_Linux.cpp LoaderPaths_Linux.hpp)
elseif(OS_FREEBSD OR OS_OPENBSD)
  configure_file(plugin-manager/LoaderPaths_BSD.hpp.in LoaderPaths_BSD.hpp @ONLY)
  target_sources(obs-studio PRIVATE plugin-manager/PluginModuleLoader_BSD.cpp LoaderPaths_BSD.hpp)
endif()

target_link_libraries(obs-studio PRIVATE nlohmann_json::nlohmann_json)
