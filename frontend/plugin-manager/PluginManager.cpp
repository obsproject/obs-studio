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

#include "PluginManager.hpp"
#include "PluginManagerWindow.hpp"

#include <OBSApp.hpp>
#include <qt-wrappers.hpp>
#include <widgets/OBSBasic.hpp>

#include <QMessageBox>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>

extern bool restart;

namespace OBS {

void addModuleToPluginManagerImpl(void *param, obs_module_t *newModule)
{
	auto &instance = *static_cast<OBS::PluginManager *>(param);
	std::string moduleName = obs_get_module_file_name(newModule);
	moduleName = moduleName.substr(0, moduleName.rfind("."));

	if (!obs_is_core_module(newModule)) {
		return;
	}

	const char *display_name = obs_get_module_name(newModule);
	std::string module_name = moduleName;
	const char *id = obs_get_module_id(newModule);
	const char *version = obs_get_module_version(newModule);

	auto it = std::find_if(instance.modules_.begin(), instance.modules_.end(),
			       [&](OBS::ModuleInfo module) { return module.module_name == moduleName; });

	if (it == instance.modules_.end()) {
		instance.modules_.push_back({display_name ? display_name : "", module_name, id ? id : "",
					     version ? version : "", true, true});
	} else {
		it->display_name = display_name ? display_name : "";
		it->module_name = module_name;
		it->id = id ? id : "";
		it->version = version ? version : "";
	}
}

constexpr std::string_view OBSPluginManagerPath = "obs-studio/plugin_manager";
constexpr std::string_view OBSPluginManagerModulesFile = "modules.json";

void PluginManager::preLoad()
{
	loadModuleConfiguration_();
	disableModules_();
}

void PluginManager::postLoad()
{
	// Find any new modules and add to Plugin Manager.
	obs_enum_modules(addModuleToPluginManager, this);
	// Get list of valid module types.
	addModuleTypes_();
	saveModules_();
	// Add provided features from any unloaded modules
	linkUnloadedModules_();
}

void PluginManager::loadAllPlugins(bool usePortableMode)
{
	preLoad();
	bool coreModuleLoadSuccess = obs_load_core_modules();

	if (!coreModuleLoadSuccess) {
		loadState_ = State::Failure;
		//TODO: Replace with structured exception type - https://github.com/obsproject/obs-studio/issues/13394
		throw "Failed to load core OBS modules. OBS cannot run without these modules. Please try reinstalling OBS.";
		return;
	}

	if (loadMode_ == Mode::Full) {
		blog(LOG_INFO, "---------------------------------");
		State pluginState = loadPlugins(usePortableMode);
		State legacyPluginState = loadLegacyPlugins(usePortableMode);

		loadState_ = (pluginState && legacyPluginState) ? State::Success : State::PartialFailure;
	}

	blog(LOG_INFO, "---------------------------------");
	obs_log_loaded_modules();
	blog(LOG_INFO, "---------------------------------");
	obs_post_load_modules();
	postLoad();
}

std::filesystem::path PluginManager::getConfigFilePath_()
{
	std::filesystem::path path = App()->userPluginManagerSettingsLocation /
				     std::filesystem::u8path(OBSPluginManagerPath) /
				     std::filesystem::u8path(OBSPluginManagerModulesFile);
	return path;
}

void PluginManager::loadModuleConfiguration_()
{
	auto modulesFile = getConfigFilePath_();
	if (std::filesystem::exists(modulesFile)) {
		std::ifstream jsonFile(modulesFile);
		nlohmann::json data;
		try {
			data = nlohmann::json::parse(jsonFile);
		} catch (const nlohmann::json::parse_error &error) {
			modules_.clear();
			blog(LOG_ERROR, "Error loading modules config file: %s", error.what());
			blog(LOG_ERROR, "Generating new config file.");
			return;
		}
		modules_.clear();
		for (auto it : data) {
			ModuleInfo obsModule;
			try {
				obsModule = {it.at("display_name"),
					     it.at("module_name"),
					     it.at("id"),
					     it.at("version"),
					     it.at("enabled"),
					     it.at("enabled"),
					     it.at("sources"),
					     it.at("outputs"),
					     it.at("encoders"),
					     it.at("services"),
					     {},
					     {},
					     {},
					     {}};
			} catch (const nlohmann::json::out_of_range &error) {
				blog(LOG_WARNING, "Error loading module info: %s", error.what());
				continue;
			}
			modules_.push_back(obsModule);
		}
	}
}

void PluginManager::linkUnloadedModules_()
{
	for (const auto &moduleInfo : modules_) {
		if (!moduleInfo.enabled) {
			auto obsModule = obs_get_disabled_module(moduleInfo.module_name.c_str());
			if (!obsModule) {
				continue;
			}
			for (const auto &source : moduleInfo.sources) {
				obs_module_add_source(obsModule, source.c_str());
			}
			for (const auto &output : moduleInfo.outputs) {
				obs_module_add_output(obsModule, output.c_str());
			}
			for (const auto &encoder : moduleInfo.encoders) {
				obs_module_add_encoder(obsModule, encoder.c_str());
			}
			for (const auto &service : moduleInfo.services) {
				obs_module_add_service(obsModule, service.c_str());
			}
		}
	}
}

void PluginManager::saveModules_()
{
	auto modulesFile = getConfigFilePath_();
	std::ofstream outFile(modulesFile);
	nlohmann::json data = nlohmann::json::array();

	for (auto const &moduleInfo : modules_) {
		nlohmann::json modData;
		modData["display_name"] = moduleInfo.display_name;
		modData["module_name"] = moduleInfo.module_name;
		modData["id"] = moduleInfo.id;
		modData["version"] = moduleInfo.version;
		modData["enabled"] = moduleInfo.enabled;
		modData["sources"] = moduleInfo.sources;
		modData["outputs"] = moduleInfo.outputs;
		modData["encoders"] = moduleInfo.encoders;
		modData["services"] = moduleInfo.services;
		data.push_back(modData);
	}
	outFile << std::setw(4) << data << std::endl;
}

void PluginManager::addModuleTypes_()
{
	const char *source_id;
	int i = 0;
	while (obs_enum_source_types(i, &source_id)) {
		i += 1;
		obs_module_t *obsModule = obs_source_get_module(source_id);
		if (!obsModule) {
			continue;
		}
		std::string moduleName = obs_get_module_file_name(obsModule);
		moduleName = moduleName.substr(0, moduleName.rfind("."));
		auto it = std::find_if(modules_.begin(), modules_.end(),
				       [moduleName](ModuleInfo const &m) { return m.module_name == moduleName; });
		if (it != modules_.end()) {
			it->sourcesLoaded.push_back(source_id);
		}
	}

	const char *output_id;
	i = 0;
	while (obs_enum_output_types(i, &output_id)) {
		i += 1;
		obs_module_t *obsModule = obs_output_get_module(output_id);
		if (!obsModule) {
			continue;
		}
		std::string moduleName = obs_get_module_file_name(obsModule);
		moduleName = moduleName.substr(0, moduleName.rfind("."));
		auto it = std::find_if(modules_.begin(), modules_.end(),
				       [moduleName](ModuleInfo const &m) { return m.module_name == moduleName; });
		if (it != modules_.end()) {
			it->outputsLoaded.push_back(output_id);
		}
	}

	const char *encoder_id;
	i = 0;
	while (obs_enum_encoder_types(i, &encoder_id)) {
		i += 1;
		obs_module_t *obsModule = obs_encoder_get_module(encoder_id);
		if (!obsModule) {
			continue;
		}
		std::string moduleName = obs_get_module_file_name(obsModule);
		moduleName = moduleName.substr(0, moduleName.rfind("."));
		auto it = std::find_if(modules_.begin(), modules_.end(),
				       [moduleName](ModuleInfo const &m) { return m.module_name == moduleName; });
		if (it != modules_.end()) {
			it->encodersLoaded.push_back(encoder_id);
		}
	}

	const char *service_id;
	i = 0;
	while (obs_enum_service_types(i, &service_id)) {
		i += 1;
		obs_module_t *obsModule = obs_service_get_module(service_id);
		if (!obsModule) {
			continue;
		}
		std::string moduleName = obs_get_module_file_name(obsModule);
		moduleName = moduleName.substr(0, moduleName.rfind("."));
		auto it = std::find_if(modules_.begin(), modules_.end(),
				       [moduleName](ModuleInfo const &m) { return m.module_name == moduleName; });
		if (it != modules_.end()) {
			it->servicesLoaded.push_back(service_id);
		}
	}

	for (auto &moduleInfo : modules_) {
		if (moduleInfo.enabledAtLaunch) {
			moduleInfo.sources = moduleInfo.sourcesLoaded;
			moduleInfo.encoders = moduleInfo.encodersLoaded;
			moduleInfo.outputs = moduleInfo.outputsLoaded;
			moduleInfo.services = moduleInfo.servicesLoaded;
		} else {
			for (auto const &source : moduleInfo.sources) {
				disabledSources_.push_back(source);
			}
			for (auto const &output : moduleInfo.outputs) {
				disabledOutputs_.push_back(output);
			}
			for (auto const &encoder : moduleInfo.encoders) {
				disabledEncoders_.push_back(encoder);
			}
			for (auto const &service : moduleInfo.services) {
				disabledServices_.push_back(service);
			}
		}
	}
}

void PluginManager::disableModules_()
{
	for (const auto &moduleInfo : modules_) {
		if (!moduleInfo.enabled) {
			obs_add_disabled_module(moduleInfo.module_name.c_str());
		}
	}
}

void PluginManager::open()
{
	auto main = OBSBasic::Get();
	PluginManagerWindow pluginManagerWindow(modules_, failedModules_, main);

	if (loadState_ == State::PartialFailure) {
		pluginManagerWindow.setPage(PluginManagerWindow::Page::Failure);
	}

	auto result = pluginManagerWindow.exec();
	if (result == QDialog::Accepted) {
		modules_ = pluginManagerWindow.result();
		saveModules_();

		bool changed = false;

		for (auto const &moduleInfo : modules_) {
			if (moduleInfo.enabled != moduleInfo.enabledAtLaunch) {
				changed = true;
				break;
			}
		}

		if (changed) {
			QMessageBox::StandardButton button =
				OBSMessageBox::question(main, QTStr("Restart"), QTStr("NeedsRestart"));

			if (button == QMessageBox::Yes) {
				restart = true;
				main->close();
			}
		}
	}
}

}; // namespace OBS

bool operator&&(const OBS::PluginManager::State &lhs, const OBS::PluginManager::State &rhs)
{
	using State = OBS::PluginManager::State;

	if (lhs == State::Success && rhs == State::Success) {
		return true;
	}

	return false;
}

bool operator&&(const OBS::PluginManager::State &lhs, bool rhs)
{
	return (lhs == OBS::PluginManager::State::Success && rhs);
}

bool operator&&(bool lhs, const OBS::PluginManager::State &rhs)
{
	return (lhs && rhs == OBS::PluginManager::State::Success);
}

bool operator||(const OBS::PluginManager::State &lhs, const OBS::PluginManager::State &rhs)
{
	using State = OBS::PluginManager::State;

	if (lhs == State::Success || rhs == State::Success) {
		return true;
	}

	return false;
}

bool operator||(const OBS::PluginManager::State &lhs, bool rhs)
{
	return (lhs == OBS::PluginManager::State::Success || rhs);
}

bool operator||(bool lhs, const OBS::PluginManager::State &rhs)
{
	return (lhs || rhs == OBS::PluginManager::State::Success);
}
