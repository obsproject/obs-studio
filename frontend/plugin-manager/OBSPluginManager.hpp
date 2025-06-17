#pragma once

#include <vector>
#include <string>

#include <obs-module.h>

struct OBSModuleInfo {
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

class OBSPluginManager {
private:
	std::vector<OBSModuleInfo> modules = {};
	std::vector<std::string> disabledSources = {};
	std::vector<std::string> disabledOutputs = {};
	std::vector<std::string> disabledServices = {};
	std::vector<std::string> disabledEncoders = {};
	std::string ModulesPath();
	void LoadModules();
	void SaveModules();
	void DisableModules();
	void AddModuleTypes();
	static void AddNewModule(void *param, obs_module_t *newModule);

public:
	void PreLoad();
	void PostLoad();
	bool SourceDisabled(obs_source_t *source) const;
	bool OutputDisabled(obs_output_t *output) const;
	bool EncoderDisabled(obs_encoder_t *encoder) const;
	bool ServiceDisabled(obs_service_t *service) const;
	void OpenDialog();
};
