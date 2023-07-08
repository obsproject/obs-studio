// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "youtube-service.hpp"

#ifdef OAUTH_ENABLED
#include <obs-frontend-api.h>
#endif

#include "youtube-config.hpp"

#ifdef OAUTH_ENABLED
constexpr const char *DATA_FILENAME = "obs-youtube.json";
#endif

YouTubeService::YouTubeService()
{
	info.type_data = this;
	info.free_type_data = InfoFreeTypeData;

	info.id = "youtube";
	info.supported_protocols = "RTMPS;RTMP;HLS";

	info.get_name = InfoGetName;
	info.create = InfoCreate;
	info.destroy = InfoDestroy;
	info.update = InfoUpdate;

	info.get_connect_info = InfoGetConnectInfo;

	info.get_protocol = InfoGetProtocol;

	info.can_try_to_connect = InfoCanTryToConnect;

	info.flags = 0;

	info.get_defaults = YouTubeConfig::InfoGetDefault;
	info.get_properties = InfoGetProperties;

	info.get_max_codec_bitrate = InfoGetMaxCodecBitrate;

	info.apply_encoder_settings2 = InfoApplySettings2;

#ifdef OAUTH_ENABLED
	info.can_bandwidth_test = InfoCanBandwidthTest;
	info.enable_bandwidth_test = InfoEnableBandwidthTest;
	info.bandwidth_test_enabled = InfoBandwidthTestEnabled;
#endif

	obs_register_service(&info);

#ifdef OAUTH_ENABLED
	obs_frontend_add_event_callback(OBSEvent, this);
#endif
}

void YouTubeService::Register()
{
	new YouTubeService();
}

#ifdef OAUTH_ENABLED
void YouTubeService::OBSEvent(obs_frontend_event event, void *priv)
{
	YouTubeService *self = reinterpret_cast<YouTubeService *>(priv);

	switch (event) {
	case OBS_FRONTEND_EVENT_PROFILE_CHANGED:
	case OBS_FRONTEND_EVENT_FINISHED_LOADING: {
		self->deferUiFunction = false;
		break;
	}
	case OBS_FRONTEND_EVENT_PROFILE_CHANGING:
		self->SaveOAuthsData();
		self->deferUiFunction = true;
		self->data = nullptr;
		break;
	case OBS_FRONTEND_EVENT_EXIT:
		self->SaveOAuthsData();
		obs_frontend_remove_event_callback(OBSEvent, priv);
		return;
	default:
		break;
	}

	if (self->oauths.empty())
		return;

	for (auto const &p : self->oauths)
		p.second->OBSEvent(event);
}

void YouTubeService::SaveOAuthsData()
{
	OBSDataAutoRelease saveData = obs_data_create();
	bool writeData = false;
	for (auto const &[uuid, oauth] : oauths) {
		OBSDataAutoRelease data = oauth->GetData();
		if (data == nullptr)
			continue;

		obs_data_set_obj(saveData, uuid.c_str(), data);
		writeData = true;
	}

	if (!writeData)
		return;

	BPtr<char> profilePath = obs_frontend_get_current_profile_path();
	std::string dataPath = profilePath.Get();
	dataPath += "/";
	dataPath += DATA_FILENAME;

	if (!obs_data_save_json_pretty_safe(saveData, dataPath.c_str(), "tmp",
					    "bak"))
		blog(LOG_ERROR,
		     "[obs-youtube][%s] Failed to save integrations data",
		     __FUNCTION__);
};

YouTubeApi::ServiceOAuth *YouTubeService::GetOAuth(const std::string &uuid,
						   obs_service_t *service)
{
	if (data == nullptr) {
		BPtr<char> profilePath =
			obs_frontend_get_current_profile_path();
		std::string dataPath = profilePath.Get();
		dataPath += "/";
		dataPath += DATA_FILENAME;

		data = obs_data_create_from_json_file_safe(dataPath.c_str(),
							   "bak");

		if (!data) {
			blog(LOG_DEBUG,
			     "[obs-youtube][%s] Failed to open integrations data: %s",
			     __FUNCTION__, dataPath.c_str());
		}
	}

	if (oauths.count(uuid) == 0) {
		OBSDataAutoRelease oauthData =
			obs_data_get_obj(data, uuid.c_str());
		oauths.emplace(uuid,
			       std::make_shared<YouTubeApi::ServiceOAuth>());
		oauths[uuid]->Setup(oauthData, deferUiFunction);
		oauthsRefCounter.emplace(uuid, 0);
		oauths[uuid]->AddBondedService(service);
	}

	oauthsRefCounter[uuid] += 1;
	return oauths[uuid].get();
}

void YouTubeService::ReleaseOAuth(const std::string &uuid,
				  obs_service_t *service)
{
	if (oauthsRefCounter.count(uuid) == 0)
		return;

	oauths[uuid]->RemoveBondedService(service);
	oauthsRefCounter[uuid] -= 1;

	if (oauthsRefCounter[uuid] != 0)
		return;

	oauths.erase(uuid);
	oauthsRefCounter.erase(uuid);
}
#endif
