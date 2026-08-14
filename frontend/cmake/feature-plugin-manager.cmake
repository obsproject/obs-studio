find_package(nlohmann_json 3.11 REQUIRED)

target_sources(
  obs-studio
  PRIVATE
    plugin-manager/PluginManager.cpp
    plugin-manager/PluginManager.hpp
    plugin-manager/PluginManagerWindow.cpp
    plugin-manager/PluginManagerWindow.hpp
)

target_link_libraries(obs-studio PRIVATE nlohmann_json::nlohmann_json)
