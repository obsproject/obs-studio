/******************************************************************************
    Copyright (C) 2025 by FiniteSingularity <finitesingularityttv@gmail.com>

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

#include <obs-module.h>

#include <filesystem>
#include <string>
#include <vector>

namespace OBS {

struct ModuleInfo {
	std::string display_name;
	std::string module_name;
	std::string id;
	std::string version;
	bool enabled;
	bool enabledAtLaunch;
	std::vector<std::string> sources;
	std::vector<std::string> outputs;
	std::vector<std::string> encoders;
	std::vector<std::string> services;
	std::vector<std::string> sourcesLoaded;
	std::vector<std::string> outputsLoaded;
	std::vector<std::string> encodersLoaded;
	std::vector<std::string> servicesLoaded;
};

class PluginManager {
public:
	using ModuleList = std::vector<std::string>;
	enum class Mode { CoreOnly, Full };
	enum class State { Failure, PartialFailure, Success };

private:
	Mode loadMode_{Mode::Full};
	State loadState_{State::Failure};

	std::vector<ModuleInfo> modules_ = {};
	ModuleList failedModules_{};
	ModuleList disabledSources_ = {};
	ModuleList disabledOutputs_ = {};
	ModuleList disabledServices_ = {};
	ModuleList disabledEncoders_ = {};
	std::filesystem::path getConfigFilePath_();
	void loadModuleConfiguration_();
	void saveModules_();
	void disableModules_();
	void addModuleTypes_();
	void linkUnloadedModules_();

	State loadPlugins(bool usePortableMode);
	State loadLegacyPlugins(bool usePortableMode);

public:
	Mode pluginMode() { return loadMode_; }
	void setPluginMode(Mode mode) { loadMode_ = mode; }

	State loadState() { return loadState_; }

	void disablePlugins();
	void preLoad();
	void postLoad();
	void loadAllPlugins(bool usePortableMode);
	void open();

	friend void addModuleToPluginManagerImpl(void *param, obs_module_t *newModule);
};

void addModuleToPluginManagerImpl(void *param, obs_module_t *newModule);

}; // namespace OBS

bool operator&&(const OBS::PluginManager::State &lhs, const OBS::PluginManager::State &rhs);
bool operator&&(const OBS::PluginManager::State &lhs, bool rhs);
bool operator&&(bool lhs, const OBS::PluginManager::State &rhs);

bool operator||(const OBS::PluginManager::State &lhs, const OBS::PluginManager::State &rhs);
bool operator||(const OBS::PluginManager::State &lhs, bool rhs);
bool operator||(bool lhs, const OBS::PluginManager::State &rhs);

// Anonymous namespace function to add module to plugin manager
// via libobs's module enumeration.
namespace {
inline void addModuleToPluginManager(void *param, obs_module_t *newModule)
{
	OBS::addModuleToPluginManagerImpl(param, newModule);
}
} // namespace
