// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "services-manager.hpp"

#include <fstream>
#include <inttypes.h>

#ifdef UPDATE_URL
#include <file-updater/file-updater.h>

constexpr const char *OBS_SERVICE_UPDATE_URL = (const char *)UPDATE_URL;
#endif

constexpr int OBS_SERVICES_FORMAT_VER = JSON_FORMAT_VER;

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

		if (json.formatVersion != OBS_SERVICES_FORMAT_VER) {
			blog(LOG_WARNING,
			     "[obs-services] [RegisterServices] "
			     "config path services file format version mismatch (file: %" PRId64
			     ", plugin: %d)",
			     json.formatVersion, OBS_SERVICES_FORMAT_VER);
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

		if (json.formatVersion != OBS_SERVICES_FORMAT_VER) {
			blog(LOG_ERROR,
			     "[obs-services] [RegisterServices] "
			     "services file format version mismatch (file: %" PRId64
			     ", plugin: %d)",
			     json.formatVersion, OBS_SERVICES_FORMAT_VER);
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

#ifdef UPDATE_URL
static bool ConfirmServicesFile(void *, file_download_data *file)
{
	std::string name(file->name);
	if (name != "services.json")
		return false;

	OBSServices::ServicesJson json;
	try {
		json = nlohmann::json::parse((char *)file->buffer.array);
	} catch (nlohmann::json::exception &e) {
		blog(LOG_DEBUG, "[obs-services][ConfirmServicesFile] %s",
		     e.what());
		return false;
	}

	return json.formatVersion == OBS_SERVICES_FORMAT_VER;
}
#endif

bool ServicesManager::Initialize()
{
	if (!manager) {
#ifdef UPDATE_URL
		BPtr<char> localDir = obs_module_file("");
		BPtr<char> cacheDir = obs_module_config_path("");
		std::string userAgent = "obs-services (libobs ";
		userAgent += obs_get_version_string();
		userAgent += ")";
		if (cacheDir) {
			update_info_t *update = update_info_create(
				"[obs-services][update] ", userAgent.c_str(),
				OBS_SERVICE_UPDATE_URL, localDir, cacheDir,
				ConfirmServicesFile, nullptr);
			update_info_destroy(update);
		}
#endif
		manager = std::make_shared<ServicesManager>();
		return manager->RegisterServices();
	}
	return true;
}

void ServicesManager::Finalize()
{
	manager.reset();
}
