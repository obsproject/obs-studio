/******************************************************************************
    Copyright (C) 2026 by FiniteSingularity <finitesingularityttv@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "PluginModuleLoader.hpp"
#include "PluginManager.hpp"
#include <LoaderPaths_Flatpak.hpp>

#include <obs.h>
#include <util/dstr.h>
#include <util/platform.h>

#include <array>
#include <string>
#include <string_view>
#include <vector>

// Flatpak 3rd-Party Plugin Locations
//
// * Flatpak Extension Point Path: /app/plugins
//     * Binary: /app/plugins/<System Library Path>/obs-modules/plugins/<Plugin>.so
//     * Data: /app/plugins/<System Data Path>/share/obs/obs-modules/plugins/<Plugin>
//
// * Legacy Flatpak Root Path: /app/plugins
//     * Binary: <Legacy Flatpak Root Path>/<System Library Path>/obs-plugins/<Plugin>.so
//     * Data: <Legacy Flatpak Root Path>/share/obs/obs-plugins/<Plugin>
//
// * XDG Data Home Path:
//     * Binary: <XDG Data Home Path>/obs-studio/plugins/<Plugin>/<Plugin>.so
//     * Data: <XDG Data Home Path>/obs-studio/plugins/<Plugin>/data
//
// * XDG Config Home Path:
//     * Binary: <Legacy User Root Path>/obs-studio/plugins/<Plugin>/bin/64bit/<Plugin>.so
//     * Data: <Legacy User Root Path>/obs-studio/plugins/<Plugin>/data

using State = OBS::PluginManager::State;
using ModuleType = obs_runtime_module_type;

namespace Constants = OBS::Constants;

constexpr std::string_view kConfigBinaryPath{"/obs-studio/plugins/%module%/"};
constexpr std::string_view kConfigDataPath{"/obs-studio/plugins/%module%/data/"};
constexpr std::string_view kLegacyConfigBinaryPath{"/obs-studio/plugins/%module%/bin/64bit"};
constexpr std::string_view kLegacyConfigDataPath{"/obs-studio/plugins/%module%/data"};
constexpr std::string_view kFlatpakBasePath{"/app/plugins/"};

namespace {
State pluginLoadHelper(const ModuleLoadInfo &info, ModuleList &failedModules)
{
	int failedModuleCount = loadPluginsByInfo(info, failedModules);

	State result = (failedModuleCount > 0) ? State::PartialFailure : State::Success;

	return result;
}
} // namespace

namespace OBS {
State PluginManager::loadPlugins(bool usePortableMode)
{
	UNUSED_PARAMETER(usePortableMode);
	State flatpakPluginState = State::Failure;

	{
		std::string binaryPath{kFlatpakBasePath};
		std::string dataPath{kFlatpakBasePath};
		binaryPath.append(Constants::kPlatformLibraryPath);
		binaryPath.append("obs-modules/plugins/");
		dataPath.append(Constants::kPlatformDataPath);
		dataPath.append("obs-modules/plugins/%module%");
		ModuleLoadInfo info = {.path_info = {.binary = binaryPath.c_str(), .data = dataPath.c_str()},
				       .type = MODULE_TYPE_PLUGIN,
				       .name = nullptr};

		flatpakPluginState = pluginLoadHelper(info, failedModules_);
	}

	State xdgDataHomeState = State::Failure;
	std::string xdgDataHomePath = getEnvironmentVariable(Constants::kXDGDataHomeVariable);
	if (!xdgDataHomePath.empty()) {
		std::string binaryPath{xdgDataHomePath};
		std::string dataPath{xdgDataHomePath};
		binaryPath.append(kConfigBinaryPath);
		dataPath.append(kConfigDataPath);
		ModuleLoadInfo info = {.path_info = {.binary = binaryPath.c_str(), .data = dataPath.c_str()},
				       .type = MODULE_TYPE_PLUGIN,
				       .name = nullptr};

		xdgDataHomeState = pluginLoadHelper(info, failedModules_);
	} else {
		xdgDataHomeState = State::Success;
	}

	return (flatpakPluginState && xdgDataHomeState) ? State::Success : State::PartialFailure;
}

State PluginManager::loadLegacyPlugins(bool usePortableMode)
{
	UNUSED_PARAMETER(usePortableMode);
	State flatpakPluginState = State::Failure;

	{
		std::string binaryPath{kFlatpakBasePath};
		std::string dataPath{kFlatpakBasePath};
		binaryPath.append(Constants::kPlatformLibraryPath);
		binaryPath.append("obs-plugins/");
		dataPath.append(Constants::kPlatformDataPath);
		dataPath.append("obs/obs-plugins/%module%");
		ModuleLoadInfo info = {.path_info = {.binary = binaryPath.c_str(), .data = dataPath.c_str()},
				       .type = MODULE_TYPE_LEGACY_PLUGIN,
				       .name = nullptr};

		flatpakPluginState = pluginLoadHelper(info, failedModules_);
	}

	State xdgDataConfigState = State::Failure;
	std::string xdgConfigHomePath = getEnvironmentVariable(Constants::kXDGConfigHomeVariable);
	if (!xdgConfigHomePath.empty()) {
		std::string binaryPath{xdgConfigHomePath};
		std::string dataPath{xdgConfigHomePath};
		binaryPath.append(kLegacyConfigBinaryPath);
		dataPath.append(kLegacyConfigDataPath);
		ModuleLoadInfo info = {.path_info = {.binary = binaryPath.c_str(), .data = dataPath.c_str()},
				       .type = MODULE_TYPE_LEGACY_PLUGIN,
				       .name = nullptr};
		xdgDataConfigState = pluginLoadHelper(info, failedModules_);
	}

	return (flatpakPluginState && xdgDataConfigState) ? State::Success : State::PartialFailure;
}
} // namespace OBS
