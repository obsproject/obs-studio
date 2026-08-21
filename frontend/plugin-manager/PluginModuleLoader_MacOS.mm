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

#include <obs.h>
#include <util/dstr.h>
#include <util/platform.h>

#include <string>
#include <string_view>
#include <vector>

// macOS 3rd-Party Plugin Locations
//
// * User Path:                 <Environment Variable>
//     * Binary:                <Environment Variable>/<Plugin>.plugin/Contents/MacOS
//     * Data:                  <Environment Variable>/<Plugin>.plugin/Contents/Resources
//
// * Root Path:                 <User>/Library/Application Support
//     * Binary:                <Root Path>/obs-studio/plugins/<Plugin>.plugin/Contents/MacOS/<Plugin>
//     * Data:                  <Root Path>/obs-studio/plugins/<Plugin>.plugin/Contents/Resources
//
// * Legacy User Path:          <Environment Variable> + <Data Environment Variable>
//     * Binary:                <Environment Variable>/<Plugin>.plugin/Contents/MacOS/<Plugin>
//     * Data:                  <Data Environment Variable>/<Plugin>.plugin/Contents/Resources
//
// * Legacy System Root Path:   /Library/Application Support
//     * Legacy binary:         <Legacy System Root Path>/obs-studio/plugins/<Plugin>/bin/<Plugin>.so
//     * Legacy data:           <Legacy System Root Path>/obs-studio/plugins/<Plugin>/data
//
// * Legacy User Root Path:     <User>/Library/Application Support
//     * Legacy binary:         <Legacy User Root Path>/obs-studio/plugins/<Plugin>/bin/<Plugin>.so
//     * Legacy data:           <Legacy User Root Path>/obs-studio/plugins/<Plugin>/data
//

using State = OBS::PluginManager::State;
using ModuleType = obs_runtime_module_type;

constexpr std::string_view kPluginPathSuffix {"obs-studio/plugins/%module%.plugin"};
constexpr std::string_view kUserPluginPathSuffix {"/%module%.plugin"};
constexpr std::string_view kLegacyPluginPathSuffix {"obs-studio/plugins/%module%"};

#ifdef __aarch64__
constexpr bool kIsAppleSilicon = true;
#else
constexpr bool kIsAppleSilicon = false;
#endif

namespace {
    State loadPluginsFromPath(const std::string &pathString, ModuleType type, ModuleList &failedModules)
    {
        std::string binaryPath {pathString};
        std::string dataPath {pathString};

        switch (type) {
            case MODULE_TYPE_LEGACY_PLUGIN: {
                binaryPath.append("/bin");
                dataPath.append("/data");
                break;
            }
            default: {
                binaryPath.append("/Contents/MacOS");
                dataPath.append("/Contents/Resources");
                break;
            }
        }

        ModuleLoadInfo info = {
            .path_info = {.binary = binaryPath.c_str(), .data = dataPath.c_str()},
            .type = type,
            .name = nullptr
        };

        int failedModuleCount = loadPluginsByInfo(info, failedModules);

        State result = (failedModuleCount > 0) ? State::PartialFailure : State::Success;

        return result;
    }

    State loadPluginsFromLegacyPaths(const std::string &binaryPathString, const std::string &dataPathString,
                                     ModuleList &failedModules)
    {
        std::string binaryPath {binaryPathString};
        std::string dataPath {dataPathString};

        binaryPath.append("/%module%.plugin/Contents/MacOS");
        dataPath.append("/%module%.plugin/Contents/Resources");

        ModuleLoadInfo info = {
            .path_info = {.binary = binaryPath.c_str(), .data = dataPath.c_str()},
            .type = MODULE_TYPE_LEGACY_PLUGIN,
            .name = nullptr
        };

        int failedModuleCount = loadPluginsByInfo(info, failedModules);

        State result = (failedModuleCount > 0) ? State::PartialFailure : State::Success;

        return result;
    }

}  // namespace

namespace OBS {
    State PluginManager::loadPlugins(bool usePortableMode __unused)
    {
        State userPluginState = State::Failure;
        std::string userPluginPath = getEnvironmentVariable(kPathVariable);

        if (!userPluginPath.empty()) {
            userPluginPath.append(kUserPluginPathSuffix);

            userPluginState = loadPluginsFromPath(userPluginPath, MODULE_TYPE_PLUGIN, failedModules_);
        } else {
            userPluginState = State::Success;
        }

        State libraryPluginState = State::Failure;

        std::string appLibraryPath {};
        appLibraryPath.resize(PATH_MAX);
        int bufferSize = os_get_config_path(appLibraryPath.data(), appLibraryPath.size(), kPluginPathSuffix.data());
        if (bufferSize > 0) {
            appLibraryPath.resize(bufferSize);

            libraryPluginState = loadPluginsFromPath(appLibraryPath, MODULE_TYPE_PLUGIN, failedModules_);
        }

        return (userPluginState && libraryPluginState) ? State::Success : State::PartialFailure;
    }

    State PluginManager::loadLegacyPlugins(bool usePortableMode __unused)
    {
        State userPluginState = State::Failure;

        std::string userPluginPath = getEnvironmentVariable(kLegacyBinaryPathVariable);
        std::string userDataPath = getEnvironmentVariable(kLegacyDataPathVariable);

        if (!userPluginPath.empty() && !userDataPath.empty()) {
            userPluginState = loadPluginsFromLegacyPaths(userPluginPath, userDataPath, failedModules_);
        } else {
            userPluginState = State::Success;
        }

        State legacyPluginState = State::Failure;

        if constexpr (!kIsAppleSilicon) {
            State systemLibraryState = State::Failure;

            std::string systemLibraryPath {};
            systemLibraryPath.resize(PATH_MAX);
            int bufferSize = os_get_program_data_path(systemLibraryPath.data(), systemLibraryPath.size(),
                                                      kLegacyPluginPathSuffix.data());

            if (bufferSize > 0) {
                systemLibraryPath.resize(bufferSize);
                systemLibraryState = loadPluginsFromPath(systemLibraryPath, MODULE_TYPE_LEGACY_PLUGIN, failedModules_);
            }

            State appLibrayState = State::Failure;
            std::string appLibraryPath {};
            appLibraryPath.resize(PATH_MAX);
            bufferSize =
                os_get_config_path(appLibraryPath.data(), appLibraryPath.size(), kLegacyPluginPathSuffix.data());

            if (bufferSize > 0) {
                appLibraryPath.resize(bufferSize);
                appLibrayState = loadPluginsFromPath(appLibraryPath, MODULE_TYPE_LEGACY_PLUGIN, failedModules_);
            }

            legacyPluginState = (systemLibraryState && appLibrayState) ? State::Success : State::PartialFailure;
        } else {
            legacyPluginState = State::Success;
        }

        return (userPluginState && legacyPluginState) ? State::Success : State::PartialFailure;
    }
}  // namespace OBS
