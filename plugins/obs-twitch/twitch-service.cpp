// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "twitch-service.hpp"

#include "twitch-config.hpp"

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

	obs_register_service(&info);
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
