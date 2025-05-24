#include "OBSBasic.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>

#include <QMessageBox>

#include <qt-wrappers.hpp>
#include <plugin-manager/OBSPluginManager.hpp>

// MARK: Constant Expressions

constexpr std::string_view OBSPluginManagerPath = "/obs-studio/basic/plugin-manager/";
constexpr std::string_view OBSPluginManagerModulesFile = "modules.json";

extern bool restart;

std::string OBSBasic::PMModulesPath()
{
	std::string modulesFile;
	modulesFile.reserve(App()->userPluginManagerLocation.u8string().size() + OBSPluginManagerPath.size() +
		OBSPluginManagerModulesFile.size());
	modulesFile.append(App()->userPluginManagerLocation.u8string()).append(OBSPluginManagerPath).append(OBSPluginManagerModulesFile);

	return modulesFile;
}

void OBSBasic::PMLoadModules()
{
	// TODO: Make this function safe for corrupt files.
	auto modulesFile = PMModulesPath();
	if (std::filesystem::exists(modulesFile)) {
		std::ifstream jsonFile(modulesFile);
		nlohmann::json data = nlohmann::json::parse(jsonFile);
		pmModules.clear();
		for (auto it : data) {
			pmModules.push_back({
				it["display_name"],
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
				{}
			});
		}
	}
	
}

void OBSBasic::PMSaveModules()
{
	// TODO: Make this function safe
	auto modulesFile = PMModulesPath();
	std::ofstream outFile(modulesFile);
	nlohmann::json data = nlohmann::json::array();

	for (auto const& module : pmModules) {
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

void OBSBasic::PMAddNewModule(void* param, obs_module_t* newModule)
{
	auto obsBasic = static_cast<OBSBasic*>(param);
	std::string moduleName = obs_get_module_file_name(newModule);
	moduleName = moduleName.substr(0, moduleName.rfind("."));

	if (!obs_get_module_allow_disable(moduleName.c_str()))
		return;

	const char* display_name = obs_get_module_name(newModule);
	std::string module_name = moduleName;
	const char* id = obs_get_module_id(newModule);
	const char* version = obs_get_module_version(newModule);

	auto it = std::find_if(
		obsBasic->pmModules.begin(),
		obsBasic->pmModules.end(),
		[&](OBSModuleInfo module) {
			return module.module_name == moduleName;
		});

	if (it == obsBasic->pmModules.end()) {
		obsBasic->pmModules.push_back({
			display_name ? display_name : "",
			module_name,
			id ? id : "",
			version ? version : "",
			true,
			true
		});
	} else {
		it->display_name = display_name ? display_name : "";
		it->module_name = module_name;
		it->id = id ? id : "";
		it->version = version ? version : "";
	}
}

void OBSBasic::PMAddModuleTypes()
{
	// TODO: Simplify this.
	const char* source_id;
	int i = 0;
	while (obs_enum_source_types(i, &source_id)) {
		i += 1;
		obs_module_t* module = obs_source_get_module(source_id);
		if (!module) {
			continue;
		}
		std::string moduleName = obs_get_module_file_name(module);
		moduleName = moduleName.substr(0, moduleName.rfind("."));
		auto it = std::find_if(pmModules.begin(), pmModules.end(), [moduleName](OBSModuleInfo const& m) {
			return m.module_name == moduleName;
		});
		if (it != pmModules.end()) {
			it->sourcesLoaded.push_back(source_id);
		}
	}

	const char* output_id;
	i = 0;
	while (obs_enum_output_types(i, &output_id)) {
		i += 1;
		obs_module_t* module = obs_source_get_module(output_id);
		if (!module) {
			continue;
		}
		std::string moduleName = obs_get_module_file_name(module);
		moduleName = moduleName.substr(0, moduleName.rfind("."));
		auto it = std::find_if(pmModules.begin(), pmModules.end(), [moduleName](OBSModuleInfo const& m) {
			return m.module_name == moduleName;
			});
		if (it != pmModules.end()) {
			it->outputsLoaded.push_back(output_id);
		}
	}

	const char* encoder_id;
	i = 0;
	while (obs_enum_encoder_types(i, &encoder_id)) {
		i += 1;
		obs_module_t* module = obs_source_get_module(encoder_id);
		if (!module) {
			continue;
		}
		std::string moduleName = obs_get_module_file_name(module);
		moduleName = moduleName.substr(0, moduleName.rfind("."));
		auto it = std::find_if(pmModules.begin(), pmModules.end(), [moduleName](OBSModuleInfo const& m) {
			return m.module_name == moduleName;
			});
		if (it != pmModules.end()) {
			it->encodersLoaded.push_back(encoder_id);
		}
	}

	const char* service_id;
	i = 0;
	while (obs_enum_service_types(i, &service_id)) {
		i += 1;
		obs_module_t* module = obs_source_get_module(service_id);
		if (!module) {
			continue;
		}
		std::string moduleName = obs_get_module_file_name(module);
		moduleName = moduleName.substr(0, moduleName.rfind("."));
		auto it = std::find_if(pmModules.begin(), pmModules.end(), [moduleName](OBSModuleInfo const& m) {
			return m.module_name == moduleName;
			});
		if (it != pmModules.end()) {
			it->servicesLoaded.push_back(service_id);
		}
	}

	for (auto& pmm : pmModules) {
		if (pmm.enabledAtLaunch) {
			pmm.sources = pmm.sourcesLoaded;
			pmm.encoders = pmm.encodersLoaded;
			pmm.outputs = pmm.outputsLoaded;
			pmm.services = pmm.servicesLoaded;
		} else {
			for (auto const& source : pmm.sources) {
				pmDisabledSources.push_back(source);
			}
			for (auto const& output : pmm.outputs) {
				pmDisabledOutputs.push_back(output);
			}
			for (auto const& encoder : pmm.encoders) {
				pmDisabledEncoders.push_back(encoder);
			}
			for (auto const& service : pmm.services) {
				pmDisabledServices.push_back(service);
			}
		}

	}
}

void OBSBasic::PMDisableModules()
{
	for (const auto& module : pmModules)
	{
		if (!module.enabled) {
			obs_add_disabled_module(module.module_name.c_str());
		}
	}
}

bool OBSBasic::PMSourceDisabled(obs_source_t* source) const
{
	std::string sourceId = obs_source_get_id(source);
	return std::find(pmDisabledSources.begin(), pmDisabledSources.end(), sourceId) != pmDisabledSources.end();
}

bool OBSBasic::PMOutputDisabled(obs_output_t* output) const
{
	std::string outputId = obs_output_get_id(output);
	return std::find(pmDisabledOutputs.begin(), pmDisabledOutputs.end(), outputId) != pmDisabledOutputs.end();
}

bool OBSBasic::PMEncoderDisabled(obs_encoder_t* encoder) const
{
	std::string encoderId = obs_encoder_get_id(encoder);
	return std::find(pmDisabledEncoders.begin(), pmDisabledEncoders.end(), encoderId) != pmDisabledEncoders.end();
}

bool OBSBasic::PMServiceDisabled(obs_service_t* service) const
{
	std::string serviceId = obs_service_get_id(service);
	return std::find(pmDisabledServices.begin(), pmDisabledServices.end(), serviceId) != pmDisabledServices.end();
}
// MARK: - Qt Slot Functions

void OBSBasic::on_actionOpenPluginManager_triggered()
{
	blog(LOG_INFO, "Plugin Manager Clicked!");
	OBSPluginManager pm(pmModules, this);
	auto res = pm.exec();
	if (res == QDialog::Accepted) {
		pmModules = pm.result();
		PMSaveModules();

		bool changed = false;

		for (auto const& module : pmModules) {
			if (module.enabled != module.enabledAtLaunch) {
				changed = true;
				break;
			}
		}

		if (changed) {
			QMessageBox::StandardButton button =
				OBSMessageBox::question(this, QTStr("PluginManager.Restart"), QTStr("PluginManager.NeedsRestart"));

			if (button == QMessageBox::Yes) {
				restart = true;
				close();
			}
		}
	}
}
