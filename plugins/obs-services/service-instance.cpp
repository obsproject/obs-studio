// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "service-instance.hpp"

#include <util/dstr.hpp>

#include "service-config.hpp"
#include "services-json-util.hpp"

ServiceInstance::ServiceInstance(const OBSServices::Service &service_)
	: service(service_)
{
	uint32_t flags = 0;

	/* Common can only be "set" if set to true in the schema */
	if (!service.common.value_or(false))
		flags |= OBS_SERVICE_UNCOMMON;

	/* Generate supported protocols string.
	 * A vector is used to check already added protocol because std::string's
	 * find() does not work well with protocols like RTMP and RTMPS*/
	std::vector<std::string> addedProtocols;
	std::string protocols;
	for (size_t i = 0; i < service.servers.size(); i++) {
		std::string protocol =
			ServerProtocolToStdString(service.servers[i].protocol);

		bool alreadyAdded = false;
		for (size_t i = 0; i < addedProtocols.size(); i++)
			alreadyAdded |= (addedProtocols[i] == protocol);

		if (!alreadyAdded) {
			if (!protocols.empty())
				protocols += ";";

			protocols += protocol;
			addedProtocols.push_back(protocol);
		}
	}
	supportedProtocols = bstrdup(protocols.c_str());

	/* Fill service info and register it */
	info.type_data = this;
	info.id = service.id.c_str();

	info.get_name = InfoGetName;
	info.create = InfoCreate;
	info.destroy = InfoDestroy;
	info.update = InfoUpdate;

	info.get_connect_info = InfoGetConnectInfo;

	info.get_protocol = InfoGetProtocol;

	if (service.supportedCodecs.has_value()) {
		if (service.supportedCodecs.value().video.has_value())
			info.get_supported_video_codecs =
				InfoGetSupportedVideoCodecs;

		if (service.supportedCodecs.value().audio.has_value())
			info.get_supported_audio_codecs =
				InfoGetSupportedAudioCodecs;
	}

	info.can_try_to_connect = InfoCanTryToConnect;

	info.flags = flags;

	info.get_defaults2 = InfoGetDefault2;
	info.get_properties2 = InfoGetProperties2;

	info.supported_protocols = supportedProtocols;

	obs_register_service(&info);
}

OBSServices::RistProperties ServiceInstance::GetRISTProperties() const
{
	return service.rist.value_or(OBSServices::RistProperties{true, false});
}

OBSServices::SrtProperties ServiceInstance::GetSRTProperties() const
{
	return service.srt.value_or(OBSServices::SrtProperties{true, false});
}

static inline void AddSupportedVideoCodecs(
	const std::map<std::string,
		       std::vector<OBSServices::ProtocolSupportedVideoCodec>>
		&map,
	const std::string &key, std::string &codecs)
{
	if (map.count(key) == 0)
		return;

	for (auto &codec : map.at(key)) {
		if (!codecs.empty())
			codecs += ";";

		codecs += SupportedVideoCodecToStdString(codec);
	}
}

char **
ServiceInstance::GetSupportedVideoCodecs(const std::string &protocol) const
{
	std::string codecs;
	auto *prtclCodecs = &service.supportedCodecs.value().video.value();

	AddSupportedVideoCodecs(*prtclCodecs, "*", codecs);
	AddSupportedVideoCodecs(*prtclCodecs, protocol, codecs);

	if (codecs.empty())
		return nullptr;

	return strlist_split(codecs.c_str(), ';', false);
}

static inline void AddSupportedAudioCodecs(
	const std::map<std::string,
		       std::vector<OBSServices::ProtocolSupportedAudioCodec>>
		&map,
	const std::string &key, std::string &codecs)
{
	if (map.count(key) == 0)
		return;

	for (auto &codec : map.at(key)) {
		if (!codecs.empty())
			codecs += ";";

		codecs += SupportedAudioCodecToStdString(codec);
	}
}

char **
ServiceInstance::GetSupportedAudioCodecs(const std::string &protocol) const
{
	std::string codecs;
	auto *prtclCodecs = &service.supportedCodecs.value().audio.value();

	AddSupportedAudioCodecs(*prtclCodecs, "*", codecs);
	AddSupportedAudioCodecs(*prtclCodecs, protocol, codecs);

	if (codecs.empty())
		return nullptr;

	return strlist_split(codecs.c_str(), ';', false);
}

const char *ServiceInstance::GetName()
{
	return service.name.c_str();
}

void ServiceInstance::GetDefaults(obs_data_t *settings)
{
	std::string protocol =
		ServerProtocolToStdString(service.servers[0].protocol);
	obs_data_set_default_string(settings, "protocol", protocol.c_str());
	obs_data_set_default_string(settings, "server",
				    service.servers[0].url.c_str());
}

bool ModifiedProtocolCb(void *service_, obs_properties_t *props,
			obs_property_t *, obs_data_t *settings)
{
	const OBSServices::Service *service =
		reinterpret_cast<OBSServices::Service *>(service_);
	std::string protocol = obs_data_get_string(settings, "protocol");
	obs_property_t *p = obs_properties_get(props, "server");

	if (protocol.empty())
		return false;

	OBSServices::ServerProtocol proto = StdStringToServerProtocol(protocol);

	obs_property_list_clear(p);
	for (size_t i = 0; i < service->servers.size(); i++) {
		const OBSServices::Server *server = &service->servers[i];
		if (server->protocol != proto)
			continue;

		obs_property_list_add_string(p, server->name.c_str(),
					     server->url.c_str());
	}

	obs_property_t *propGetStreamKey =
		obs_properties_get(props, "get_stream_key");

	std::unordered_map<std::string, obs_property_t *> properties;
#define ADD_TO_MAP(property) \
	properties.emplace(property, obs_properties_get(props, property))
	ADD_TO_MAP("stream_id");
	ADD_TO_MAP("username");
	ADD_TO_MAP("password");
	ADD_TO_MAP("encrypt_passphrase");
#undef ADD_TO_MAP

	if (propGetStreamKey)
		obs_property_set_visible(propGetStreamKey, false);

	for (auto const &[key, val] : properties)
		obs_property_set_visible(val, false);

	switch (proto) {
	case OBSServices::ServerProtocol::RTMP:
	case OBSServices::ServerProtocol::RTMPS:
	case OBSServices::ServerProtocol::HLS:
		obs_property_set_description(
			properties["stream_id"],
			obs_module_text("Services.StreamID.Key"));
		obs_property_set_visible(properties["stream_id"], true);
		if (propGetStreamKey)
			obs_property_set_visible(propGetStreamKey, true);
		break;
	case OBSServices::ServerProtocol::SRT: {
		bool hasProps = service->srt.has_value();
		obs_property_set_description(
			properties["stream_id"],
			obs_module_text("Services.StreamID"));
		obs_property_set_visible(properties["stream_id"],
					 hasProps ? service->srt->streamId
						  : true);
		obs_property_set_visible(
			properties["encrypt_passphrase"],
			hasProps ? service->srt->encryptPassphrase : false);
		break;
	}
	case OBSServices::ServerProtocol::RIST: {
		bool hasProps = service->rist.has_value();
		obs_property_set_visible(
			properties["encrypt_passphrase"],
			hasProps ? service->rist->encryptPassphrase : true);
		obs_property_set_visible(
			properties["username"],
			hasProps ? service->rist->srpUsernamePassword : false);
		obs_property_set_visible(
			properties["password"],
			hasProps ? service->rist->srpUsernamePassword : false);
		break;
	}
	}

	return true;
}

obs_properties_t *ServiceInstance::GetProperties()
{
	obs_properties_t *ppts = obs_properties_create();
	obs_property_t *p;
	obs_property_t *uniqueProtocol;

	if (service.moreInfoLink) {
		BPtr<char> url = bstrdup(service.moreInfoLink->c_str());

		p = obs_properties_add_button(
			ppts, "more_info", obs_module_text("Services.MoreInfo"),
			nullptr);
		obs_property_button_set_type(p, OBS_BUTTON_URL);
		obs_property_button_set_url(p, url);
	}

	p = obs_properties_add_list(ppts, "protocol",
				    obs_module_text("Services.Protocol"),
				    OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);

	uniqueProtocol = obs_properties_add_text(ppts, "unique_protocol",
						 "placeholder", OBS_TEXT_INFO);
	obs_property_set_visible(uniqueProtocol, false);

	obs_properties_add_list(ppts, "server",
				obs_module_text("Services.Server"),
				OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	for (size_t i = 0; i < service.servers.size(); i++) {
		std::string protocol =
			ServerProtocolToStdString(service.servers[i].protocol);

		if (protocol.empty() ||
		    !obs_is_output_protocol_registered(protocol.c_str()))
			continue;

		bool alreadyListed = false;

		for (size_t i = 0; i < obs_property_list_item_count(p); i++)
			alreadyListed |=
				(strcmp(obs_property_list_item_string(p, i),
					protocol.c_str()) == 0);

		if (!alreadyListed)
			obs_property_list_add_string(p, protocol.c_str(),
						     protocol.c_str());
	}

	if (obs_property_list_item_count(p) == 1) {
		DStr info;
		dstr_catf(info, obs_module_text("Services.Protocol.OnlyOne"),
			  obs_property_list_item_string(p, 0));

		obs_property_set_visible(p, false);
		obs_property_set_visible(uniqueProtocol, true);
		obs_property_set_description(uniqueProtocol, info);
	}

	obs_property_set_modified_callback2(p, ModifiedProtocolCb,
					    (void *)&service);

	obs_properties_add_text(ppts, "stream_id",
				obs_module_text("Services.StreamID"),
				OBS_TEXT_PASSWORD);

	if (service.streamKeyLink) {
		BPtr<char> url = bstrdup(service.streamKeyLink->c_str());

		p = obs_properties_add_button(
			ppts, "get_stream_key",
			obs_module_text("Services.GetStreamKey"), nullptr);
		obs_property_button_set_type(p, OBS_BUTTON_URL);
		obs_property_button_set_url(p, url);
	}

	obs_properties_add_text(ppts, "username",
				obs_module_text("Services.Username"),
				OBS_TEXT_DEFAULT);
	obs_properties_add_text(ppts, "password",
				obs_module_text("Services.Password"),
				OBS_TEXT_PASSWORD);
	obs_properties_add_text(ppts, "encrypt_passphrase",
				obs_module_text("Services.EncryptPassphrase"),
				OBS_TEXT_PASSWORD);

	return ppts;
}
