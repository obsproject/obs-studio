// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "services-manager.hpp"

#include <fstream>

std::shared_ptr<ServicesManager> manager = nullptr;

bool ServicesManager::RegisterServices()
{
	char *file = obs_module_config_path("services.json");
	std::ifstream servicesFile(file);
	OBSServices::ServicesJson json;
	bool fallback = false;

	bfree(file);

	if (servicesFile.is_open()) {
		try {
			json = nlohmann::json::parse(servicesFile);
		} catch (nlohmann::json::exception &e) {
			blog(LOG_WARNING, "[obs-services] %s", e.what());
			blog(LOG_WARNING,
			     "[obs-services] [RegisterServices] config path services file could not be parsed");
			fallback = true;
		}
	} else {
		blog(LOG_DEBUG, "[obs-services] [RegisterServices] "
				"config path services file not found");
		fallback = true;
	}

	if (fallback) {
		file = obs_module_file("services.json");
		servicesFile.open(file);
		bfree(file);

		if (!servicesFile.is_open()) {
			blog(LOG_ERROR, "[obs-services] [RegisterServices] "
					"services file not found");
			return false;
		}

		try {
			json = nlohmann::json::parse(servicesFile);
		} catch (nlohmann::json::exception &e) {
			blog(LOG_ERROR, "[obs-services] %s", e.what());
			blog(LOG_ERROR, "[obs-services] [RegisterServices] "
					"services file could not be parsed");
			return false;
		}
	}

	for (size_t i = 0; i < json.services.size(); i++) {
		const char *id = json.services[i].id.c_str();

		services.push_back(
			std::make_shared<ServiceInstance>(json.services[i]));

		blog(LOG_DEBUG,
		     "[obs-services] [RegisterServices] "
		     "registered service with id : %s",
		     id);
	}

	servicesFile.close();
	return true;
}

bool ServicesManager::Initialize()
{
	if (!manager) {
		manager = std::make_shared<ServicesManager>();
		return manager->RegisterServices();
	}
	return true;
}

void ServicesManager::Finalize()
{
	manager.reset();
}
