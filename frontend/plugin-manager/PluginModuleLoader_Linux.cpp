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
#include <LoaderPaths_Linux.hpp>

#include <obs.h>
#include <util/dstr.h>
#include <util/platform.h>

#include <array>
#include <string>
#include <string_view>
#include <vector>

// Linux 3rd-Party Plugin Locations
//
// * User Path: <Environment Variable>
//     * Binary: <Environment Variable>/<Plugin>/<Plugin>.so
//     * Data: <Environment Variable>/<Plugin>/data
//
// * System Root Path: /usr
//     * Binary: <System Root Path>/<System Library>/obs-modules/plugins/<Plugin>.so
//     * Data: <System Root Path>/share/obs/obs-modules/plugins/<Plugin>
//
// * User Root Path: $XDG_DATA_HOME
//     * Binary: <User Root Path>/obs-studio/plugins/<Plugin>/<Plugin>.so
//     * Data: <User Root Path>/obs-studio/plugins/<Plugin>/data
//
// * Legacy User Path: <Environment Variable> + <Environment Data Variable>
//     * Binary: <Environment Variable>/<Plugin>.so
//     * Data: <Environment Data Variable>/<Plugin>
//
// * Legacy System Root Path: /usr
//     * Binary: <Legacy System Root Path>/<System Library>/obs-plugins/<Plugin>.so
//     * Data: <Legacy System Root Path>/share/obs/obs-plugins/<Plugin>
//
// * Legacy User Root Path: $XDG_CONFIG_HOME
//     * Binary: <Legacy User Root Path>/obs-studio/plugins/<Plugin>/bin/64bit/<Plugin>.so
//     * Data: <Legacy User Root Path>/obs-studio/plugins/<Plugin>/data
//
// * Legacy Fallback Path: <OBS Binary Location>
//     * Binary: <Legacy Fallback Path>/../../obs-plugins/64bit/<Plugin>.so
//     * Data: <Legacy System Root Path>/share/obs/obs-plugins/<Plugin>

using State = OBS::PluginManager::State;
using ModuleType = obs_runtime_module_type;

namespace Constants = OBS::Constants;

constexpr std::string_view kModulePathSuffix{"/%module%/"};
constexpr std::string_view kModuleDataPathSuffix{"/%module%/data/"};
constexpr std::string_view kXdgBinaryPath{"/obs-studio/plugins/%module%/"};
constexpr std::string_view kXdgDataPath{"/obs-studio/plugins/%module%/data/"};

constexpr std::string_view kUserBinaryPath{"/%module%"};
constexpr std::string_view kUserDataPath{"/%module%/data"};
constexpr std::string_view kLegacyXdgBinaryPath{"/obs-studio/plugins/%module%/bin/64bit"};
constexpr std::string_view kLegacyXdgDataPath{"/obs-studio/plugins/%module%/data"};

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
	State userPluginState = State::Failure;
	std::string userPluginPath = getEnvironmentVariable(kPathVariable);

	if (!userPluginPath.empty()) {
		std::string binaryPath{userPluginPath};
		std::string dataPath{userPluginPath};
		binaryPath.append(kModulePathSuffix);
		dataPath.append(kModuleDataPathSuffix);

		ModuleLoadInfo info = {.path_info = {.binary = binaryPath.c_str(), .data = dataPath.c_str()},
				       .type = MODULE_TYPE_PLUGIN,
				       .name = nullptr};

		userPluginState = pluginLoadHelper(info, failedModules_);
	} else {
		userPluginState = State::Success;
	}

	State systemPluginState = State::Failure;

	if (Constants::kPlatformPluginModulePath.size() && Constants::kPlatformPluginDataPath.size()) {
		std::string binaryPath{Constants::kPlatformPluginModulePath};
		std::string dataPath{Constants::kPlatformPluginDataPath};

		dataPath.append(kModulePathSuffix);

		ModuleLoadInfo info = {.path_info = {.binary = binaryPath.c_str(), .data = dataPath.c_str()},
				       .type = MODULE_TYPE_PLUGIN,
				       .name = nullptr};

		systemPluginState = pluginLoadHelper(info, failedModules_);
	} else {
		systemPluginState = State::Success;
	}

	State xdgPluginState = State::Failure;

	if (!usePortableMode) {
		std::string xdgDataHomePath = getEnvironmentVariable(Constants::kXDGDataHomeVariable);
		if (xdgDataHomePath.empty()) {
			std::string homePath = getEnvironmentVariable("HOME");
			if (!homePath.empty()) {
				xdgDataHomePath = homePath + "/.local/share";
			}
		}
		if (!xdgDataHomePath.empty()) {
			std::string binaryPath{xdgDataHomePath};
			std::string dataPath{xdgDataHomePath};
			binaryPath.append(kXdgBinaryPath);
			dataPath.append(kXdgDataPath);
			ModuleLoadInfo info = {.path_info = {.binary = binaryPath.c_str(), .data = dataPath.c_str()},
					       .type = MODULE_TYPE_PLUGIN,
					       .name = nullptr};

			xdgPluginState = pluginLoadHelper(info, failedModules_);
		} else {
			xdgPluginState = State::Success;
		}
	} else {
		xdgPluginState = State::Success;
	}

	return (userPluginState && systemPluginState && xdgPluginState) ? State::Success : State::PartialFailure;
}

State PluginManager::loadLegacyPlugins(bool usePortableMode)
{
	State userPluginState = State::Failure;

	std::string userPluginPath = getEnvironmentVariable(kLegacyBinaryPathVariable);
	std::string userDataPath = getEnvironmentVariable(kLegacyDataPathVariable);

	if (!userPluginPath.empty() && !userDataPath.empty()) {
		std::string binaryPath{userPluginPath};
		std::string dataPath{userDataPath};

		dataPath.append(kModulePathSuffix);

		ModuleLoadInfo info = {.path_info = {.binary = binaryPath.c_str(), .data = dataPath.c_str()},
				       .type = MODULE_TYPE_LEGACY_PLUGIN,
				       .name = nullptr};

		userPluginState = pluginLoadHelper(info, failedModules_);
	} else {
		userPluginState = State::Success;
	}

	State systemPluginState = State::Failure;

	{
		std::string binaryPath{Constants::kPlatformInstallPath};
		std::string dataPath{Constants::kPlatformInstallPath};
		binaryPath.append(Constants::kPlatformLibraryPath);
		binaryPath.append("obs-plugins/");
		dataPath.append(Constants::kPlatformDataPath);
		dataPath.append("obs/obs-plugins/%module%");

		ModuleLoadInfo info = {.path_info = {.binary = binaryPath.c_str(), .data = dataPath.c_str()},
				       .type = MODULE_TYPE_LEGACY_PLUGIN,
				       .name = nullptr};

		systemPluginState = pluginLoadHelper(info, failedModules_);
	}

	State xdgPluginState = State::Failure;

	if (!usePortableMode) {
		std::string xdgConfigHomePath = getEnvironmentVariable(Constants::kXDGConfigHomeVariable);
		if (xdgConfigHomePath.empty()) {
			std::string homePath = getEnvironmentVariable("HOME");
			if (!homePath.empty()) {
				xdgConfigHomePath = homePath + "/.config";
			}
		}
		if (!xdgConfigHomePath.empty()) {
			std::string binaryPath{xdgConfigHomePath};
			std::string dataPath{xdgConfigHomePath};
			binaryPath.append(kLegacyXdgBinaryPath);
			dataPath.append(kLegacyXdgDataPath);
			ModuleLoadInfo info = {.path_info = {.binary = binaryPath.c_str(), .data = dataPath.c_str()},
					       .type = MODULE_TYPE_LEGACY_PLUGIN,
					       .name = nullptr};

			xdgPluginState = pluginLoadHelper(info, failedModules_);
		} else {
			xdgPluginState = State::Success;
		}
	} else {
		xdgPluginState = State::Success;
	}

	return (userPluginState && systemPluginState && xdgPluginState) ? State::Success : State::PartialFailure;
}
} // namespace OBS
