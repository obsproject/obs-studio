/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

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

#include "window-basic-main.hpp"

#include <util/profiler.hpp>

#include <fstream>
#include <sstream>

#include <json11.hpp>

#include "platform.hpp"

#define SERVICE_PATH "streamService.json"

void OBSBasic::SaveService()
{
	if (!service)
		return;

	char serviceJsonPath[512];
	int ret = GetProfilePath(serviceJsonPath, sizeof(serviceJsonPath),
				 SERVICE_PATH);
	if (ret <= 0)
		return;

	OBSDataAutoRelease data = obs_data_create();
	OBSDataAutoRelease settings = obs_service_get_settings(service);

	obs_data_set_string(data, "type", obs_service_get_type(service));
	obs_data_set_obj(data, "settings", settings);

	if (!obs_data_save_json_safe(data, serviceJsonPath, "tmp", "bak"))
		blog(LOG_WARNING, "Failed to save service");
}

static bool service_available(const char *service)
{
	const char *val;
	size_t i = 0;

	while (obs_enum_service_types(i++, &val))
		if (strcmp(val, service) == 0)
			return true;

	return false;
}

static bool migrate_twitch_integration(config_t *basicConfig,
				       obs_data_t *settings)
{
	BPtr<char> uuid = os_generate_uuid();
	char pluginConfigpath[512];

	if (GetProfilePath(pluginConfigpath, sizeof(pluginConfigpath),
			   "obs-twitch.json") <= 0) {
		blog(LOG_ERROR,
		     "Failed to create obs-twitch plugin config path");
		return false;
	}

	OBSDataAutoRelease oauthData = obs_data_create();
	OBSDataAutoRelease settingsData = obs_data_create();

	obs_data_set_string(oauthData, "access_token",
			    config_get_string(basicConfig, "Twitch", "Token"));
	obs_data_set_int(oauthData, "expire_time",
			 config_get_uint(basicConfig, "Twitch", "ExpireTime"));
	obs_data_set_string(oauthData, "refresh_token",
			    config_get_string(basicConfig, "Twitch",
					      "RefreshToken"));
	obs_data_set_int(oauthData, "scope_version",
			 config_get_int(basicConfig, "Twitch", "ScopeVer"));

	obs_data_set_string(settingsData, "feed_uuid",
			    config_get_string(basicConfig, "Twitch", "UUID"));

	obs_data_set_int(settings, "chat_addon",
			 config_get_int(basicConfig, "Twitch", "AddonChoice"));
	obs_data_set_bool(settings, "enforce_bwtest",
			  obs_data_get_bool(settings, "bwtest"));

	OBSDataAutoRelease serviceData = obs_data_create();
	obs_data_set_obj(serviceData, "oauth", oauthData);
	obs_data_set_obj(serviceData, "settings", settingsData);

	OBSDataAutoRelease pluginData = obs_data_create();
	obs_data_set_obj(pluginData, uuid, serviceData);
	obs_data_set_string(settings, "uuid", uuid);

	if (!obs_data_save_json(pluginData, pluginConfigpath)) {
		blog(LOG_ERROR, "Failed to save migrated obs-twitch data");
		return false;
	}

	const char *dockStateStr =
		config_get_string(basicConfig, "Twitch", "DockState");
	config_set_string(basicConfig, "BasicWindow", "DockState",
			  dockStateStr);

	return true;
}

static bool migrate_restream_integration(config_t *basicConfig,
					 obs_data_t *settings)
{
	BPtr<char> uuid = os_generate_uuid();
	char pluginConfigpath[512];

	if (GetProfilePath(pluginConfigpath, sizeof(pluginConfigpath),
			   "obs-restream.json") <= 0) {
		blog(LOG_ERROR,
		     "Failed to create obs-restream plugin config path");
		return false;
	}

	OBSDataAutoRelease oauthData = obs_data_create();

	obs_data_set_string(oauthData, "access_token",
			    config_get_string(basicConfig, "Restream",
					      "Token"));
	obs_data_set_int(oauthData, "expire_time",
			 config_get_uint(basicConfig, "Restream",
					 "ExpireTime"));
	obs_data_set_string(oauthData, "refresh_token",
			    config_get_string(basicConfig, "Restream",
					      "RefreshToken"));
	obs_data_set_int(oauthData, "scope_version",
			 config_get_int(basicConfig, "Restream", "ScopeVer"));

	OBSDataAutoRelease serviceData = obs_data_create();
	obs_data_set_obj(serviceData, "oauth", oauthData);

	OBSDataAutoRelease pluginData = obs_data_create();
	obs_data_set_obj(pluginData, uuid, serviceData);
	obs_data_set_string(settings, "uuid", uuid);

	if (!obs_data_save_json(pluginData, pluginConfigpath)) {
		blog(LOG_ERROR, "Failed to save migrated obs-restream data");
		return false;
	}

	const char *dockStateStr =
		config_get_string(basicConfig, "Restream", "DockState");
	config_set_string(basicConfig, "BasicWindow", "DockState",
			  dockStateStr);

	return true;
}

static bool migrate_youtube_integration(config_t *basicConfig,
					obs_data_t *settings)
{
	BPtr<char> uuid = os_generate_uuid();
	char pluginConfigpath[512];

	if (GetProfilePath(pluginConfigpath, sizeof(pluginConfigpath),
			   "obs-youtube.json") <= 0) {
		blog(LOG_ERROR,
		     "Failed to create obs-youtube plugin config path");
		return false;
	}

	OBSDataAutoRelease oauthData = obs_data_create();
	OBSDataAutoRelease settingsData = obs_data_create();

	obs_data_set_string(oauthData, "access_token",
			    config_get_string(basicConfig, "YouTube", "Token"));
	obs_data_set_int(oauthData, "expire_time",
			 config_get_uint(basicConfig, "YouTube", "ExpireTime"));
	obs_data_set_string(oauthData, "refresh_token",
			    config_get_string(basicConfig, "YouTube",
					      "RefreshToken"));
	obs_data_set_int(oauthData, "scope_version",
			 config_get_int(basicConfig, "YouTube", "ScopeVer"));

	bool rememberSettings =
		config_get_bool(basicConfig, "YouTube", "RememberSettings");
	obs_data_set_bool(settingsData, "remember", rememberSettings);

	if (rememberSettings) {
		obs_data_set_string(settingsData, "title",
				    config_get_string(basicConfig, "YouTube",
						      "Title"));

		obs_data_set_string(settingsData, "description",
				    config_get_string(basicConfig, "YouTube",
						      "Description"));

		std::string priv =
			config_get_string(basicConfig, "YouTube", "Privacy");
		int privInt = 0 /* "private" */;
		if (priv == "public")
			privInt = 1;
		else if (priv == "unlisted")
			privInt = 2;
		obs_data_set_int(settingsData, "privacy", privInt);

		obs_data_set_string(settingsData, "category_id",
				    config_get_string(basicConfig, "YouTube",
						      "CategoryID"));

		std::string latency =
			config_get_string(basicConfig, "YouTube", "Latency");
		int latencyInt = 0 /* "normal" */;
		if (latency == "low")
			latencyInt = 1;
		else if (latency == "ultraLow")
			latencyInt = 2;
		obs_data_set_int(settingsData, "latency", latencyInt);

		obs_data_set_bool(settingsData, "dvr",
				  config_get_bool(basicConfig, "YouTube",
						  "DVR"));

		obs_data_set_bool(settingsData, "made_for_kids",
				  config_get_bool(basicConfig, "YouTube",
						  "MadeForKids"));

		obs_data_set_bool(settingsData, "schedule_for_later",
				  config_get_bool(basicConfig, "YouTube",
						  "ScheduleForLater"));

		obs_data_set_bool(settingsData, "auto_start",
				  config_get_bool(basicConfig, "YouTube",
						  "AutoStart"));

		obs_data_set_bool(settingsData, "auto_stop",
				  config_get_bool(basicConfig, "YouTube",
						  "AutoStop"));

		std::string projection =
			config_get_string(basicConfig, "YouTube", "Projection");
		obs_data_set_int(settingsData, "projection",
				 projection == "360" ? 1 : 0);

		obs_data_set_string(settingsData, "thumbnail_file",
				    config_get_string(basicConfig, "YouTube",
						      "ThumbnailFile"));
	}

	OBSDataAutoRelease serviceData = obs_data_create();
	obs_data_set_obj(serviceData, "oauth", oauthData);
	obs_data_set_obj(serviceData, "settings", settingsData);

	OBSDataAutoRelease pluginData = obs_data_create();
	obs_data_set_obj(pluginData, uuid, serviceData);
	obs_data_set_string(settings, "uuid", uuid);

	if (!obs_data_save_json(pluginData, pluginConfigpath)) {
		blog(LOG_ERROR, "Failed to save migrated obs-youtube data");
		return false;
	}

	const char *dockStateStr =
		config_get_string(basicConfig, "YouTube", "DockState");
	config_set_string(basicConfig, "BasicWindow", "DockState",
			  dockStateStr);

	return true;
}

static bool migrate_rtmp_common(config_t *basicConfig, obs_data_t *data)
{
	OBSDataAutoRelease settings = obs_data_get_obj(data, "settings");

	std::string matrixFilePath;
	if (!GetDataFilePath("services-conversion-matrix.json",
			     matrixFilePath)) {
		blog(LOG_ERROR, "Failed to find service conversion matrix");
		return false;
	}

	std::ifstream matrixFile(matrixFilePath);
	if (!matrixFile.is_open()) {
		blog(LOG_ERROR, "Failed to open service conversion matrix");
		return false;
	}

	std::string error;
	std::stringstream ss;
	ss << matrixFile.rdbuf();
	json11::Json matrix = json11::Json::parse(ss.str(), error);
	if (matrix.is_null()) {
		blog(LOG_ERROR, "Failed to parse service conversion matrix");
		return false;
	}

	std::string service = obs_data_get_string(settings, "service");
	std::string type = matrix[service].string_value();
	if (!service_available(type.c_str())) {
		blog(LOG_ERROR, "Service '%s' not found, migration failed",
		     type.c_str());
		return false;
	}

	blog(LOG_INFO, "Migrating 'rtmp_common' service to '%s'", type.c_str());
	obs_data_set_string(data, "type", type.c_str());

	bool integration = false;
	if (config_has_user_value(basicConfig, "Auth", "Type")) {
		std::string authType =
			config_get_string(basicConfig, "Auth", "Type");
		/* If authType is the same as service name, integration is enabled */
		integration = (service == authType);
	}

	if (integration) {
		/* Migrate integrations */
		if (type == "twitch") {
			if (!migrate_twitch_integration(basicConfig, settings))
				return false;
		} else if (type == "restream") {
			if (!migrate_restream_integration(basicConfig,
							  settings))
				return false;
		} else if (type == "youtube") {
			if (!migrate_youtube_integration(basicConfig, settings))
				return false;
		}
	} else if (type == "twitch" || type == "restream" ||
		   type == "youtube") {
		/* Those service use "stream_key" */
		obs_data_set_string(settings, "stream_key",
				    obs_data_get_string(data, "key"));
		obs_data_erase(settings, "key");
	} else if (type != "dacast" && type != "nimo_tv" &&
		   type != "showroom" && type != "younow") {
		/* Services from obs-services use "stream_id"
		 * other services than integration kept "key" */
		obs_data_set_string(settings, "stream_id",
				    obs_data_get_string(data, "key"));
		obs_data_erase(settings, "key");
	}
	obs_data_erase(settings, "bwtest");

	if (service == "YouNow") {
		obs_data_set_string(settings, "protocol", "FTL");
	} else if (service == "YouTube - HLS") {
		obs_data_set_string(settings, "protocol", "HLS");
	} else if (type == "twitch" || type == "dacast" || type == "nimo_tv" ||
		   type == "showroom") {
		obs_data_set_string(settings, "protocol", "RTMP");
	}

	if (!obs_data_has_user_value(settings, "protocol")) {
		std::string url = obs_data_get_string(settings, "server");
		std::string prefix = url.substr(0, url.find("://") + 3);
		obs_data_set_string(settings, "protocol",
				    prefix == "rtmps://" ? "RTMPS" : "RTMP");
	}

	obs_data_set_obj(data, "settings", settings);

	return true;
}

static bool migrate_rtmp_custom(obs_data_t *data)
{
	if (!service_available("custom_service")) {
		blog(LOG_ERROR,
		     "Service 'custom_service' not found, migration failed");
		return false;
	}

	blog(LOG_INFO, "Migrating 'rtmp_custom' service to 'custom_service'");

	obs_data_set_string(data, "type", "custom_service");
	OBSDataAutoRelease settings = obs_data_get_obj(data, "settings");

	obs_data_erase(settings, "use_auth");
	obs_data_erase(settings, "bwtest");

	std::string url = obs_data_get_string(settings, "server");

	std::string protocol;
	std::string prefix = url.substr(0, url.find("://") + 3);

	if (prefix == "rtmp://")
		protocol = "RTMP";
	else if (prefix == "rtmps://")
		protocol = "RTMPS";
	else if (prefix == "ftl://")
		protocol = "FTL";
	else if (prefix == "srt://")
		protocol = "SRT";
	else if (prefix == "rist://")
		protocol = "RIST";

	obs_data_set_string(settings, "protocol", protocol.c_str());

	/* Migrate encryption passphrase */
	std::string passphrase;
	if (protocol == "SRT") {
		passphrase = obs_data_get_string(settings, "password");
		obs_data_erase(settings, "username");
		obs_data_erase(settings, "password");
	} else if (protocol == "RIST") {
		passphrase = obs_data_get_string(settings, "key");

		obs_data_erase(settings, "key");
	}

	if (!passphrase.empty())
		obs_data_set_string(settings, "encrypt_passphrase",
				    passphrase.c_str());

	/* Migrate stream ID/key */
	if (protocol != "RIST") {
		std::string stream_id = obs_data_get_string(settings, "key");
		if (!stream_id.empty())
			obs_data_set_string(settings, "stream_id",
					    stream_id.c_str());

		obs_data_erase(settings, "key");
	}

	obs_data_set_obj(data, "settings", settings);

	return true;
}

bool OBSBasic::LoadService()
{
	char serviceJsonPath[512];
	int ret = GetProfilePath(serviceJsonPath, sizeof(serviceJsonPath),
				 SERVICE_PATH);
	if (ret <= 0)
		return false;

	OBSDataAutoRelease data =
		obs_data_create_from_json_file_safe(serviceJsonPath, "bak");

	if (!data) {
		char oldServiceJsonPath[512];

		if (GetProfilePath(oldServiceJsonPath,
				   sizeof(oldServiceJsonPath),
				   "service.json") <= 0)
			return false;

		data = obs_data_create_from_json_file_safe(oldServiceJsonPath,
							   "bak");

		if (!data)
			return false;

		/* Migration if old rtmp-services and network settings */
		std::string type = obs_data_get_string(data, "type");
		/* rtmp_common was set as a default value so type can be empty */
		if (type == "rtmp_common" || type.empty()) {
			if (!migrate_rtmp_common(basicConfig, data))
				return false;

		} else if (type == "rtmp_custom") {
			if (!migrate_rtmp_custom(data))
				return false;
		}

		if (!obs_data_save_json(data, serviceJsonPath))
			blog(LOG_WARNING, "Failed to save migrated service");
	}

	std::string type = obs_data_get_string(data, "type");

	if (!service_available(type.c_str())) {
		blog(LOG_ERROR,
		     "Service '%s' not found, fallback to empty custom service",
		     type.c_str());
		return false;
	}

	OBSDataAutoRelease settings = obs_data_get_obj(data, "settings");
	service = obs_service_create(type.c_str(), "default_service", settings,
				     nullptr);
	obs_service_release(service);

	if (!service)
		return false;

	/* Check service returns a protocol */
	const char *protocol = obs_service_get_protocol(service);
	if (!protocol) {
		blog(LOG_ERROR,
		     "Service '%s' did not returned a protocol, fallback to empty custom service",
		     type.c_str());
		return false;
	}

	/* Check if the protocol protocol is registered */
	if (!obs_is_output_protocol_registered(protocol)) {
		blog(LOG_ERROR,
		     "Protocol %s is not supported, fallback to empty custom service",
		     protocol);
		return false;
	}

	/* Enforce Opus on FTL if needed */
	if (strcmp(protocol, "FTL") == 0) {
		const char *option = config_get_string(
			basicConfig, "SimpleOutput", "StreamAudioEncoder");
		if (strcmp(option, "opus") != 0)
			config_set_string(basicConfig, "SimpleOutput",
					  "StreamAudioEncoder", "opus");

		option = config_get_string(basicConfig, "AdvOut",
					   "AudioEncoder");
		if (strcmp(obs_get_encoder_codec(option), "opus") != 0)
			config_set_string(basicConfig, "AdvOut", "AudioEncoder",
					  "ffmpeg_opus");
	}

	return true;
}

bool OBSBasic::InitService()
{
	ProfileScope("OBSBasic::InitService");

	if (LoadService())
		return true;

	service = obs_service_create("rtmp_common", "default_service", nullptr,
				     nullptr);
	if (!service)
		return false;
	obs_service_release(service);

	return true;
}

obs_service_t *OBSBasic::GetService()
{
	if (!service) {
		service =
			obs_service_create("rtmp_common", NULL, NULL, nullptr);
		obs_service_release(service);
	}
	return service;
}

void OBSBasic::SetService(obs_service_t *newService)
{
	if (!newService)
		return;

	ResetServiceBroadcastFlow();

	service = newService;

	LoadServiceBroadcastFlow();
}

void OBSBasic::ResetServiceBroadcastFlow()
{
	if (!serviceBroadcastFlow)
		return;

	serviceBroadcastFlow = nullptr;

	ui->broadcastButton->setVisible(false);
	ui->broadcastButton->disconnect(SIGNAL(clicked(bool)));
}

void OBSBasic::LoadServiceBroadcastFlow()
{
	if (serviceBroadcastFlow)
		return;

	for (int64_t i = 0; i < broadcastFlows.size(); i++) {
		if (broadcastFlows[i].GetBondedService() != service)
			continue;

		serviceBroadcastFlow = &broadcastFlows[i];
		ui->broadcastButton->setVisible(true);

		ui->broadcastButton->setChecked(
			serviceBroadcastFlow->BroadcastState() !=
			OBS_BROADCAST_NONE);

		connect(ui->broadcastButton, &QPushButton::clicked, this,
			&OBSBasic::ManageBroadcastButtonClicked);
		break;
	}
}

bool OBSBasic::AddBroadcastFlow(const obs_service_t *service_,
				const obs_frontend_broadcast_flow &flow)
{
	for (int64_t i = 0; i < broadcastFlows.size(); i++) {
		if (broadcastFlows[i].GetBondedService() != service_)
			continue;

		blog(LOG_WARNING,
		     "This service already has an broadcast flow added");
		return false;
	}

	broadcastFlows.append(OBSBroadcastFlow(service_, flow));

	LoadServiceBroadcastFlow();

	return true;
}

void OBSBasic::RemoveBroadcastFlow(const obs_service_t *service_)
{
	for (int64_t i = 0; i < broadcastFlows.size(); i++) {
		if (broadcastFlows[i].GetBondedService() != service_)
			continue;

		broadcastFlows.removeAt(i);

		if (GetService() == service_)
			ResetServiceBroadcastFlow();

		return;
	}

	blog(LOG_WARNING, "This service has no broadcast flow added to it");
}
