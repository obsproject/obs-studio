// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "twitch-config.hpp"

#ifdef OAUTH_ENABLED
#include <util/platform.h>
#endif

constexpr const char *STREAM_KEY_LINK =
	"https://dashboard.twitch.tv/settings/stream";
constexpr const char *DEFAULT_RTMPS_INGEST = "rtmps://live.twitch.tv/app";
constexpr const char *DEFAULT_RTMP_INGEST = "rtmp://live.twitch.tv/app";

constexpr const char *BANDWIDTH_TEST = "?bandwidthtest=true";

TwitchConfig::TwitchConfig(obs_data_t *settings, obs_service_t *self)
	: typeData(reinterpret_cast<TwitchService *>(
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

void TwitchConfig::Update(obs_data_t *settings)
{
	protocol = obs_data_get_string(settings, "protocol");

	std::string server = obs_data_get_string(settings, "server");
	serverAuto = server == "auto";
	if (!serverAuto)
		serverUrl = obs_data_get_string(settings, "server");

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

	oauth->SetAddon((TwitchBrowserWidget::Addon)obs_data_get_int(
		settings, "chat_addon"));
#else
	streamKey = obs_data_get_string(settings, "stream_key");
	bandwidthTestStreamKey = streamKey + BANDWIDTH_TEST;
#endif
	enforceBandwidthTest = obs_data_get_bool(settings, "enforce_bwtest");
}

TwitchConfig::~TwitchConfig()
{
#ifdef OAUTH_ENABLED
	typeData->ReleaseOAuth(uuid, serviceObj);
#endif
}

void TwitchConfig::InfoGetDefault(obs_data_t *settings)
{
	obs_data_set_string(settings, "protocol", "RTMPS");
	obs_data_set_string(settings, "server", "auto");
}

const char *TwitchConfig::ConnectInfo(uint32_t type)
{
	switch ((enum obs_service_connect_info)type) {
	case OBS_SERVICE_CONNECT_INFO_SERVER_URL:
		if (serverAuto) {
			std::vector<TwitchApi::Ingest> ingests =
				typeData->GetIngests(true);
			if (ingests.empty()) {
				return protocol == "RTMPS"
					       ? DEFAULT_RTMPS_INGEST
					       : DEFAULT_RTMP_INGEST;
			}

			serverUrl = protocol == "RTMPS"
					    ? ingests[0].url_template_secure
					    : ingests[0].url_template;
		}

		if (!serverUrl.empty())
			return serverUrl.c_str();

		break;
	case OBS_SERVICE_CONNECT_INFO_STREAM_KEY:
#ifdef OAUTH_ENABLED
		if (oauth->Connected() && streamKey.empty()) {
			if (oauth->GetStreamKey(streamKey))
				bandwidthTestStreamKey =
					streamKey + BANDWIDTH_TEST;
		}
#endif
		if (BandwidthTestEnabled() ? !bandwidthTestStreamKey.empty()
					   : !streamKey.empty()) {
			return BandwidthTestEnabled()
				       ? bandwidthTestStreamKey.c_str()
				       : streamKey.c_str();
		}
		break;
	default:
		break;
	}

	return nullptr;
}

bool TwitchConfig::CanTryToConnect()
{
#ifdef OAUTH_ENABLED
	if (!serverAuto && serverUrl.empty())
		return false;

	if (oauth->Connected()) {
		bool ret = oauth->GetStreamKey(streamKey);
		if (!streamKey.empty())
			bandwidthTestStreamKey = streamKey + BANDWIDTH_TEST;

		return ret;
	} else {
		return !streamKey.empty();
	}
#else
	if ((!serverAuto && serverUrl.empty()) || streamKey.empty())
		return false;
#endif

	return true;
}

static bool ModifiedProtocolCb(void *priv, obs_properties_t *props,
			       obs_property_t *, obs_data_t *settings)
{
	TwitchService *typeData = reinterpret_cast<TwitchService *>(priv);

	std::string protocol = obs_data_get_string(settings, "protocol");
	obs_property_t *p = obs_properties_get(props, "server");

	if (protocol.empty())
		return false;

	std::vector<TwitchApi::Ingest> ingests = typeData->GetIngests();
	obs_property_list_clear(p);

	obs_property_list_add_string(p, obs_module_text("Twitch.Ingest.Auto"),
				     "auto");

	if (!ingests.empty()) {
		for (const TwitchApi::Ingest &ingest : ingests) {
			obs_property_list_add_string(
				p, ingest.name.c_str(),
				protocol == "RTMPS"
					? ingest.url_template_secure.c_str()
					: ingest.url_template.c_str());
		}
	}

	obs_property_list_add_string(p,
				     obs_module_text("Twitch.Ingest.Default"),
				     protocol == "RTMPS" ? DEFAULT_RTMPS_INGEST
							 : DEFAULT_RTMP_INGEST);

	return true;
}

#ifdef OAUTH_ENABLED

static inline void AddDisplayName(obs_properties_t *props,
				  TwitchApi::ServiceOAuth *oauth)
{
	TwitchApi::User info;
	if (oauth->GetUser(info)) {
		std::string displayName = "<b>";
		displayName += info.displayName;
		displayName += "</b>";
		obs_property_t *p =
			obs_properties_get(props, "connected_account");
		obs_property_set_long_description(p, displayName.c_str());
		obs_property_set_visible(p, true);
	}
}

static bool ConnectCb(obs_properties_t *props, obs_property_t *, void *priv)
{
	TwitchApi::ServiceOAuth *oauth =
		reinterpret_cast<TwitchApi::ServiceOAuth *>(priv);

	if (!oauth->Login())
		return false;

	AddDisplayName(props, oauth);

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
	obs_property_set_visible(obs_properties_get(props, "connected_account"),
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
	TwitchApi::ServiceOAuth *oauth =
		reinterpret_cast<TwitchApi::ServiceOAuth *>(priv);

	if (!oauth->SignOut())
		return false;

	if (!obs_frontend_is_browser_available())
		return UseStreamKeyCb(props, nullptr, nullptr);

	obs_property_set_visible(obs_properties_get(props, "connect"), true);
	obs_property_set_visible(obs_properties_get(props, "disconnect"),
				 false);
	obs_property_set_visible(obs_properties_get(props, "use_stream_key"),
				 true);
	obs_property_set_visible(obs_properties_get(props, "connected_account"),
				 false);
	obs_property_set_visible(obs_properties_get(props, "stream_key"),
				 false);
	obs_property_set_visible(obs_properties_get(props, "get_stream_key"),
				 false);
	obs_property_set_visible(obs_properties_get(props, "chat_addon"),
				 false);

	return true;
}
#endif

obs_properties_t *TwitchConfig::GetProperties()
{
	obs_properties_t *ppts = obs_properties_create();
	obs_property_t *p;

#ifdef OAUTH_ENABLED
	bool connected = oauth->Connected();
	p = obs_properties_add_text(
		ppts, "connected_account",
		obs_module_text("Twitch.Auth.ConnectedAccount"), OBS_TEXT_INFO);
	obs_property_set_visible(p, false);
	if (connected)
		AddDisplayName(ppts, oauth);
#endif

	p = obs_properties_add_list(ppts, "protocol",
				    obs_module_text("Twitch.Protocol"),
				    OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);

	if (obs_is_output_protocol_registered("RTMPS"))
		obs_property_list_add_string(p, "RTMPS", "RTMPS");

	if (obs_is_output_protocol_registered("RTMP"))
		obs_property_list_add_string(p, "RTMP", "RTMP");

	obs_property_set_modified_callback2(p, ModifiedProtocolCb, typeData);

	obs_properties_add_list(ppts, "server",
				obs_module_text("Twitch.Server"),
				OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	p = obs_properties_add_text(ppts, "stream_key",
				    obs_module_text("Twitch.StreamKey"),
				    OBS_TEXT_PASSWORD);
#ifdef OAUTH_ENABLED
	obs_property_set_visible(p, !streamKey.empty() && !connected);
#endif

	p = obs_properties_add_button(ppts, "get_stream_key",
				      obs_module_text("Twitch.GetStreamKey"),
				      nullptr);
	obs_property_button_set_type(p, OBS_BUTTON_URL);
	obs_property_button_set_url(p, (char *)STREAM_KEY_LINK);

#ifdef OAUTH_ENABLED
	obs_property_set_visible(p, !streamKey.empty() && !connected);

	p = obs_properties_add_list(ppts, "chat_addon",
				    obs_module_text("Twitch.Addon"),
				    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(p, obs_module_text("Twitch.Addon.None"),
				  TwitchBrowserWidget::NONE);
	obs_property_list_add_int(p, obs_module_text("Twitch.Addon.BTTV"),
				  TwitchBrowserWidget::BTTV);
	obs_property_list_add_int(p, obs_module_text("Twitch.Addon.FFZ"),
				  TwitchBrowserWidget::FFZ);
	obs_property_list_add_int(p, obs_module_text("Twitch.Addon.Both"),
				  TwitchBrowserWidget::BOTH);
	obs_property_set_visible(p, connected);

	p = obs_properties_add_button2(ppts, "connect",
				       obs_module_text("Twitch.Auth.Connect"),
				       ConnectCb, oauth);
	obs_property_set_visible(p, !connected);

	p = obs_properties_add_button2(
		ppts, "disconnect", obs_module_text("Twitch.Auth.Disconnect"),
		DisconnectCb, oauth);
	obs_property_set_visible(p, connected);

	p = obs_properties_add_button(ppts, "use_stream_key",
				      obs_module_text("Twitch.UseStreamKey"),
				      UseStreamKeyCb);
	obs_property_set_visible(p, streamKey.empty() && !connected);

	if (!obs_frontend_is_browser_available() && !connected)
		UseStreamKeyCb(ppts, nullptr, nullptr);
#endif

	obs_properties_add_bool(
		ppts, "enforce_bwtest",
		obs_module_text("Twitch.EnforceBandwidthTestMode"));

	return ppts;
}
