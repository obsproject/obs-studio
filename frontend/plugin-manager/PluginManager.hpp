#pragma once

#include <vector>
#include <string>
#include <filesystem>

#include <obs-module.h>

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
private:
	std::vector<ModuleInfo> modules_ = {};
	std::vector<std::string> disabledSources_ = {};
	std::vector<std::string> disabledOutputs_ = {};
	std::vector<std::string> disabledServices_ = {};
	std::vector<std::string> disabledEncoders_ = {};
	std::filesystem::path getConfigFilePath_();
	void loadModules_();
	void saveModules_();
	void disableModules_();
	void addModuleTypes_();
	void linkUnloadedModules_();

public:
	void preLoad();
	void postLoad();
	void open();

	friend void addModuleToPluginManagerImpl(void *param, obs_module_t *newModule);
};

void addModuleToPluginManagerImpl(void *param, obs_module_t *newModule);

}; // namespace OBS

// Anonymous namespace function to add module to plugin manager
// via libobs's module enumeration.
namespace {
inline void addModuleToPluginManager(void *param, obs_module_t *newModule)
{
	OBS::addModuleToPluginManagerImpl(param, newModule);
}
} // namespace
