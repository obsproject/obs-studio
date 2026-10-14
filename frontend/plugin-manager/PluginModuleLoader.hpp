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

#pragma once

#include <obs.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

using FailureInfo = obs_module_failure_info;
using ModuleLoadInfo = obs_runtime_module_info;
using ModuleList = std::vector<std::string>;

inline constexpr std::string_view kPathVariable{"OBS_PLUGINS_PATH"};
inline constexpr std::string_view kLegacyBinaryPathVariable{"OBS_LEGACY_PLUGINS_PATH"};
inline constexpr std::string_view kLegacyDataPathVariable{"OBS_LEGACY_PLUGINS_DATA_PATH"};

inline std::string getEnvironmentVariable(std::string_view variableName)
{
	std::unique_ptr<char> variablePointer{};
	variablePointer.reset(getenv(variableName.data()));

	if (!variablePointer) {
		return {};
	}

	std::string result{variablePointer.release()};

	return result;
}

inline int loadPluginsByInfo(const ModuleLoadInfo &info, ModuleList &failedModules)
{
	FailureInfo result = {0};

	obs_load_plugins(const_cast<ModuleLoadInfo *>(std::addressof(info)), std::addressof(result));

	for (size_t i = 0; i < result.count; ++i) {
		const char *failedPluginName = result.failed_modules[i];

		if (failedPluginName && *failedPluginName) {
			failedModules.emplace_back(failedPluginName);
		}
	}

	obs_module_failure_info_free(std::addressof(result));

	return static_cast<int>(result.count);
}
