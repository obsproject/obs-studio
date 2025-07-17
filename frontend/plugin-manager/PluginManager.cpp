#include "PluginManager.hpp"
#include "PluginManagerWindow.hpp"

#include <OBSApp.hpp>
#include <widgets/OBSBasic.hpp>
#include <qt-wrappers.hpp>

#include <QMessageBox>

#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>

extern bool restart;

namespace OBS {

constexpr std::string_view OBSPluginManagerPath = "obs-studio/plugin_manager";
constexpr std::string_view OBSPluginManagerModulesFile = "modules.json";

void PluginManager::preLoad()
{
	loadModules_();
	disableModules_();
}

void PluginManager::postLoad()
{
	// Find any new modules and add to Plugin Manager.
	obs_enum_modules(PluginManager::addModule_, this);
	// Get list of valid module types.
	addModuleTypes_();
	saveModules_();
}

std::filesystem::path PluginManager::getConfigFilePath_()
{
	std::filesystem::path path = App()->userPluginManagerSettingsLocation / std::filesystem::u8path(OBSPluginManagerPath) / std::filesystem::u8path(OBSPluginManagerModulesFile);
	return path;
}

void PluginManager::loadModules_()
{
	// TODO: Make this function safe for corrupt files.
	auto modulesFile = getConfigFilePath_();
	if (std::filesystem::exists(modulesFile)) {
		std::ifstream jsonFile(modulesFile);
		nlohmann::json data = nlohmann::json::parse(jsonFile);
		modules_.clear();
		for (auto it : data) {
			modules_.push_back({it["display_name"],
					   it["module_name"],
					   it["id"],
					   it["version"],
					   it["enabled"],
					   it["enabled"],
					   it["sources"],
					   it["outputs"],
					   it["encoders"],
					   it["services"],
					   {},
					   {},
					   {},
					   {}});
		}
	}
}

void PluginManager::saveModules_()
{
	// TODO: Make this function safe
	auto modulesFile = getConfigFilePath_();
	std::ofstream outFile(modulesFile);
	nlohmann::json data = nlohmann::json::array();

	for (auto const &module : modules_) {
		nlohmann::json modData;
		modData["display_name"] = module.display_name;
		modData["module_name"] = module.module_name;
		modData["id"] = module.id;
		modData["version"] = module.version;
		modData["enabled"] = module.enabled;
		modData["sources"] = module.sources;
		modData["outputs"] = module.outputs;
		modData["encoders"] = module.encoders;
		modData["services"] = module.services;
		data.push_back(modData);
	}
	outFile << std::setw(4) << data << std::endl;
}

void PluginManager::addModule_(void *param, obs_module_t *newModule)
{
	auto instance = static_cast<PluginManager *>(param);
	std::string moduleName = obs_get_module_file_name(newModule);
	moduleName = moduleName.substr(0, moduleName.rfind("."));

	if (!obs_get_module_allow_disable(moduleName.c_str()))
		return;

	const char *display_name = obs_get_module_name(newModule);
	std::string module_name = moduleName;
	const char *id = obs_get_module_id(newModule);
	const char *version = obs_get_module_version(newModule);

	auto it = std::find_if(instance->modules_.begin(), instance->modules_.end(),
			       [&](ModuleInfo module) { return module.module_name == moduleName; });

	if (it == instance->modules_.end()) {
		instance->modules_.push_back({display_name ? display_name : "", module_name, id ? id : "",
					     version ? version : "", true, true});
	} else {
		it->display_name = display_name ? display_name : "";
		it->module_name = module_name;
		it->id = id ? id : "";
		it->version = version ? version : "";
	}
}

void PluginManager::addModuleTypes_()
{
	// TODO: Simplify this.
	const char *source_id;
	int i = 0;
	while (obs_enum_source_types(i, &source_id)) {
		i += 1;
		obs_module_t *module = obs_source_get_module(source_id);
		if (!module) {
			continue;
		}
		std::string moduleName = obs_get_module_file_name(module);
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
		obs_module_t *module = obs_source_get_module(output_id);
		if (!module) {
			continue;
		}
		std::string moduleName = obs_get_module_file_name(module);
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
		obs_module_t *module = obs_source_get_module(encoder_id);
		if (!module) {
			continue;
		}
		std::string moduleName = obs_get_module_file_name(module);
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
		obs_module_t *module = obs_source_get_module(service_id);
		if (!module) {
			continue;
		}
		std::string moduleName = obs_get_module_file_name(module);
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
	for (const auto &module : modules_) {
		if (!module.enabled) {
			obs_add_disabled_module(module.module_name.c_str());
		}
	}
}

bool PluginManager::isModuleDisabledFor(obs_source_t *source) const
{
	std::string sourceId = obs_source_get_id(source);
	return std::find(disabledSources_.begin(), disabledSources_.end(), sourceId) != disabledSources_.end();
}

bool PluginManager::isModuleDisabledFor(obs_output_t *output) const
{
	std::string outputId = obs_output_get_id(output);
	return std::find(disabledOutputs_.begin(), disabledOutputs_.end(), outputId) != disabledOutputs_.end();
}

bool PluginManager::isModuleDisabledFor(obs_encoder_t *encoder) const
{
	std::string encoderId = obs_encoder_get_id(encoder);
	return std::find(disabledEncoders_.begin(), disabledEncoders_.end(), encoderId) != disabledEncoders_.end();
}

bool PluginManager::isModuleDisabledFor(obs_service_t *service) const
{
	std::string serviceId = obs_service_get_id(service);
	return std::find(disabledServices_.begin(), disabledServices_.end(), serviceId) != disabledServices_.end();
}

void PluginManager::open()
{
	auto main = OBSBasic::Get();
	PluginManagerWindow pluginManagerWindow(modules_, main);
	auto result = pluginManagerWindow.exec();
	if (result == QDialog::Accepted) {
		modules_ = pluginManagerWindow.result();
		saveModules_();

		bool changed = false;

		for (auto const &module : modules_) {
			if (module.enabled != module.enabledAtLaunch) {
				changed = true;
				break;
			}
		}

		if (changed) {
			QMessageBox::StandardButton button = OBSMessageBox::question(
				main, QTStr("PluginManager.Restart"), QTStr("PluginManager.NeedsRestart"));

			if (button == QMessageBox::Yes) {
				restart = true;
				main->close();
			}
		}
	}
}

}; // namespace OBS
