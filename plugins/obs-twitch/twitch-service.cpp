// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "twitch-service.hpp"

#ifdef OAUTH_ENABLED
#include <obs-frontend-api.h>
#endif

#include "twitch-config.hpp"

#ifdef OAUTH_ENABLED
constexpr const char *DATA_FILENAME = "obs-twitch.json";
#endif

TwitchService::TwitchService()
{
	info.type_data = this;
	info.free_type_data = InfoFreeTypeData;

	info.id = "twitch";
	info.supported_protocols = "RTMPS;RTMP";

	info.get_name = InfoGetName;
	info.create = InfoCreate;
	info.destroy = InfoDestroy;
	info.update = InfoUpdate;

	info.get_connect_info = InfoGetConnectInfo;

	info.get_protocol = InfoGetProtocol;

	info.get_supported_video_codecs = InfoGetSupportedVideoCodecs;

	info.can_try_to_connect = InfoCanTryToConnect;

	info.flags = 0;

	info.get_defaults = TwitchConfig::InfoGetDefault;
	info.get_properties = InfoGetProperties;

	info.can_bandwidth_test = InfoCanBandwidthTest;
	info.enable_bandwidth_test = InfoEnableBandwidthTest;
	info.bandwidth_test_enabled = InfoBandwidthTestEnabled;

	info.get_max_codec_bitrate = InfoGetMaxCodecBitrate;

	info.apply_encoder_settings2 = InfoApplySettings2;

	obs_register_service(&info);

#ifdef OAUTH_ENABLED
	obs_frontend_add_event_callback(OBSEvent, this);
#endif
}

void TwitchService::Register()
{
	new TwitchService();
}

std::vector<TwitchApi::Ingest> TwitchService::GetIngests(bool refresh)
{
	if (!ingests.empty() && !refresh)
		return ingests;

	std::string output;
	std::string errorStr;
	long responseCode = 0;
	std::string userAgent("obs-twitch ");
	userAgent += obs_get_version_string();

	CURLcode curlCode = GetRemoteFile(
		userAgent.c_str(), "https://ingest.twitch.tv/ingests", output,
		errorStr, &responseCode, "application/x-www-form-urlencoded",
		"", nullptr, std::vector<std::string>(), nullptr, 5);

	RequestError error;
	if (curlCode != CURLE_OK && curlCode != CURLE_HTTP_RETURNED_ERROR) {
		error = RequestError(RequestErrorType::CURL_REQUEST_FAILED,
				     errorStr);
		blog(LOG_ERROR, "[obs-twitch][%s] %s: %s", __FUNCTION__,
		     error.message.c_str(), error.error.c_str());
		return {};
	}

	TwitchApi::IngestResponse response;
	switch (responseCode) {
	case 200:
		try {
			response = nlohmann::json::parse(output);
			ingests = response.ingests;

			for (TwitchApi::Ingest &ingest : ingests) {
#define REMOVE_STREAMKEY_PLACEHOLDER(url) \
	url = url.substr(0, url.find("/{stream_key}"))

				REMOVE_STREAMKEY_PLACEHOLDER(
					ingest.url_template);
				REMOVE_STREAMKEY_PLACEHOLDER(
					ingest.url_template_secure);

#undef REMOVE_STREAMKEY_PLACEHOLDER
			}

			return ingests;
		} catch (nlohmann::json::exception &e) {
			error = RequestError(
				RequestErrorType::JSON_PARSING_FAILED,
				e.what());
			blog(LOG_ERROR, "[obs-twitch][%s] %s: %s", __FUNCTION__,
			     error.message.c_str(), error.error.c_str());
		}
		break;
	default:
		error = RequestError(
			RequestErrorType::UNMANAGED_HTTP_RESPONSE_CODE,
			errorStr);
		blog(LOG_ERROR, "[obs-twitch][%s] HTTP status: %ld",
		     __FUNCTION__, responseCode);
		break;
	}

	return {};
}

#ifdef OAUTH_ENABLED
void TwitchService::OBSEvent(obs_frontend_event event, void *priv)
{
	TwitchService *self = reinterpret_cast<TwitchService *>(priv);

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

void TwitchService::SaveOAuthsData()
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
		     "[obs-twitch][%s] Failed to save integrations data",
		     __FUNCTION__);
};

TwitchApi::ServiceOAuth *TwitchService::GetOAuth(const std::string &uuid,
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
			     "[obs-twitch][%s] Failed to open integrations data: %s",
			     __FUNCTION__, dataPath.c_str());
		}
	}

	if (oauths.count(uuid) == 0) {
		OBSDataAutoRelease oauthData =
			obs_data_get_obj(data, uuid.c_str());
		oauths.emplace(uuid,
			       std::make_shared<TwitchApi::ServiceOAuth>());
		oauths[uuid]->Setup(oauthData, deferUiFunction);
		oauthsRefCounter.emplace(uuid, 0);
		oauths[uuid]->AddBondedService(service);
	}

	oauthsRefCounter[uuid] += 1;
	return oauths[uuid].get();
}

void TwitchService::ReleaseOAuth(const std::string &uuid,
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
