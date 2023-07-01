// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "restream-config.hpp"

#ifdef OAUTH_ENABLED
#include <util/platform.h>
#endif

constexpr const char *STREAM_KEY_LINK =
	"https://restream.io/settings/streaming-setup?from=OBS";
constexpr const char *AUTODETECT_INGEST = "rtmp://live.restream.io/live";

constexpr const char *BANDWIDTH_TEST = "?test=true";

RestreamConfig::RestreamConfig(obs_data_t *settings, obs_service_t *self)
	: typeData(reinterpret_cast<RestreamService *>(
		  obs_service_get_type_data(self)))
#ifdef OAUTH_ENABLED
	  ,
	  serviceObj(self)

#endif
{
#ifdef OAUTH_ENABLED
	if (!obs_data_has_user_value(settings, "uuid")) {
		BPtr<char> newUuid = os_generate_uuid();
		obs_data_set_string(settings, "uuid", newUuid);
	}

	uuid = obs_data_get_string(settings, "uuid");
#endif

	Update(settings);
}

void RestreamConfig::Update(obs_data_t *settings)
{
	server = obs_data_get_string(settings, "server");

#ifdef OAUTH_ENABLED
	std::string newUuid = obs_data_get_string(settings, "uuid");

	if (newUuid != uuid) {
		if (oauth) {
			typeData->ReleaseOAuth(uuid, serviceObj);
			oauth = nullptr;
		}

		uuid = newUuid;
	}
	if (!oauth)
		oauth = typeData->GetOAuth(uuid, serviceObj);

	if (!oauth->Connected()) {
		streamKey = obs_data_get_string(settings, "stream_key");
	}
#else
	streamKey = obs_data_get_string(settings, "stream_key");
	bandwidthTestStreamKey = streamKey + BANDWIDTH_TEST;
#endif
}

RestreamConfig::~RestreamConfig()
{
#ifdef OAUTH_ENABLED
	typeData->ReleaseOAuth(uuid, serviceObj);
#endif
}

void RestreamConfig::InfoGetDefault(obs_data_t *settings)
{
	obs_data_set_string(settings, "server", AUTODETECT_INGEST);
}

const char *RestreamConfig::ConnectInfo(uint32_t type)
{
	switch ((enum obs_service_connect_info)type) {
	case OBS_SERVICE_CONNECT_INFO_SERVER_URL:
		if (!server.empty())
			return server.c_str();

		break;
	case OBS_SERVICE_CONNECT_INFO_STREAM_KEY:
#ifdef OAUTH_ENABLED
		if (oauth->Connected() && streamKey.empty()) {
			if (oauth->GetStreamKey(streamKey))
				bandwidthTestStreamKey =
					streamKey + BANDWIDTH_TEST;
		}
#endif
		if (bandwidthTest ? !bandwidthTestStreamKey.empty()
				  : !streamKey.empty()) {
			return bandwidthTest ? bandwidthTestStreamKey.c_str()
					     : streamKey.c_str();
		}
		break;
	default:
		break;
	}

	return nullptr;
}

bool RestreamConfig::CanTryToConnect()
{
#ifdef OAUTH_ENABLED
	if (oauth->Connected()) {
		bool ret = oauth->GetStreamKey(streamKey);
		if (!streamKey.empty())
			bandwidthTestStreamKey = streamKey + BANDWIDTH_TEST;

		return ret;
	} else {
		return !streamKey.empty();
	}
#else
	if (server.empty() || streamKey.empty())
		return false;
#endif

	return true;
}

#ifdef OAUTH_ENABLED
static bool ConnectCb(obs_properties_t *props, obs_property_t *, void *priv)
{
	RestreamApi::ServiceOAuth *oauth =
		reinterpret_cast<RestreamApi::ServiceOAuth *>(priv);

	if (!oauth->Login())
		return false;

	obs_property_set_visible(obs_properties_get(props, "connect"), false);
	obs_property_set_visible(obs_properties_get(props, "disconnect"), true);
	obs_property_set_visible(obs_properties_get(props, "use_stream_key"),
				 false);
	obs_property_set_visible(obs_properties_get(props, "stream_key"),
				 false);
	obs_property_set_visible(obs_properties_get(props, "get_stream_key"),
				 false);
	obs_property_set_visible(obs_properties_get(props, "chat_addon"), true);

	return true;
}

static bool UseStreamKeyCb(obs_properties_t *props, obs_property_t *, void *)
{
	obs_property_set_visible(obs_properties_get(props, "connect"),
				 obs_frontend_is_browser_available());
	obs_property_set_visible(obs_properties_get(props, "disconnect"),
				 false);
	obs_property_set_visible(obs_properties_get(props, "use_stream_key"),
				 false);
	obs_property_set_visible(obs_properties_get(props, "stream_key"), true);
	obs_property_set_visible(obs_properties_get(props, "get_stream_key"),
				 true);
	obs_property_set_visible(obs_properties_get(props, "chat_addon"),
				 false);

	return true;
}

static bool DisconnectCb(obs_properties_t *props, obs_property_t *, void *priv)
{
	RestreamApi::ServiceOAuth *oauth =
		reinterpret_cast<RestreamApi::ServiceOAuth *>(priv);

	if (!oauth->SignOut())
		return false;

	if (!obs_frontend_is_browser_available())
		return UseStreamKeyCb(props, nullptr, nullptr);

	obs_property_set_visible(obs_properties_get(props, "connect"), true);
	obs_property_set_visible(obs_properties_get(props, "disconnect"),
				 false);
	obs_property_set_visible(obs_properties_get(props, "use_stream_key"),
				 true);
	obs_property_set_visible(obs_properties_get(props, "stream_key"),
				 false);
	obs_property_set_visible(obs_properties_get(props, "get_stream_key"),
				 false);
	obs_property_set_visible(obs_properties_get(props, "chat_addon"),
				 false);

	return true;
}
#endif

obs_properties_t *RestreamConfig::GetProperties()
{
	obs_properties_t *ppts = obs_properties_create();
	obs_property_t *p;

#ifdef OAUTH_ENABLED
	bool connected = oauth->Connected();
#endif

	p = obs_properties_add_list(ppts, "server",
				    obs_module_text("Restream.Server"),
				    OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);

	std::vector<RestreamApi::Ingest> ingests = typeData->GetIngests();

	if (ingests.empty()) {
		obs_property_list_add_string(p, "Autodetect",
					     AUTODETECT_INGEST);
	} else {

		for (const RestreamApi::Ingest &ingest : ingests) {
			obs_property_list_add_string(p, ingest.name.c_str(),
						     ingest.rtmpUrl.c_str());
		}
	}

	p = obs_properties_add_text(ppts, "stream_key",
				    obs_module_text("Restream.StreamKey"),
				    OBS_TEXT_PASSWORD);
#ifdef OAUTH_ENABLED
	obs_property_set_visible(p, !streamKey.empty() && !connected);
#endif

	p = obs_properties_add_button(ppts, "get_stream_key",
				      obs_module_text("Restream.GetStreamKey"),
				      nullptr);
	obs_property_button_set_type(p, OBS_BUTTON_URL);
	obs_property_button_set_url(p, (char *)STREAM_KEY_LINK);

#ifdef OAUTH_ENABLED
	obs_property_set_visible(p, !streamKey.empty() && !connected);

	p = obs_properties_add_button2(ppts, "connect",
				       obs_module_text("Restream.Auth.Connect"),
				       ConnectCb, oauth);
	obs_property_set_visible(p, !connected);

	p = obs_properties_add_button2(
		ppts, "disconnect", obs_module_text("Restream.Auth.Disconnect"),
		DisconnectCb, oauth);
	obs_property_set_visible(p, connected);

	p = obs_properties_add_button(ppts, "use_stream_key",
				      obs_module_text("Restream.UseStreamKey"),
				      UseStreamKeyCb);
	obs_property_set_visible(p, streamKey.empty() && !connected);

	if (!obs_frontend_is_browser_available() && !connected)
		UseStreamKeyCb(ppts, nullptr, nullptr);
#endif

	return ppts;
}
