#pragma once

#include <string_view>
namespace OBSClipboard {
inline constexpr std::string_view SceneItems = "application/x-obs-studio-scene-items+json";
inline constexpr std::string_view SourceFilters = "application/x-obs-studio-source-filters+json";
inline constexpr std::string_view SceneItemTransform = "application/x-obs-studio-scene-item-transform+json";
inline constexpr std::string_view SceneItemTransition = "application/x-obs-studio-scene-item-transition+json";

inline constexpr int PayloadVersion = 1;
} // namespace OBSClipboard