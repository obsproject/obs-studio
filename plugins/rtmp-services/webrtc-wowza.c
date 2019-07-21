#include <obs-module.h>

struct webrtc_wowza {
	char *server;
	char *room;
	char *username;
	char *codec;
	char *protocol;
	char *output;
};

static const char *webrtc_wowza_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "Wowza Streaming Engine - WebRTC";
}

static void webrtc_wowza_update(void *data, obs_data_t *settings)
{
	struct webrtc_wowza *service = data;

	bfree(service->server);
	bfree(service->room);
	bfree(service->username);
	bfree(service->codec);
	bfree(service->protocol);
	bfree(service->output);

	service->server = bstrdup(obs_data_get_string(settings, "server"));
	service->room = bstrdup(obs_data_get_string(settings, "room"));
	service->username = bstrdup(obs_data_get_string(settings, "username"));
	service->codec = bstrdup(obs_data_get_string(settings, "codec"));
	service->protocol = bstrdup(obs_data_get_string(settings, "protocol"));
	service->output = bstrdup("wowza_output");
}

static void webrtc_wowza_destroy(void *data)
{
	struct webrtc_wowza *service = data;

	bfree(service->server);
	bfree(service->room);
	bfree(service->username);
	bfree(service->codec);
	bfree(service->protocol);
	bfree(service->output);
	bfree(service);
}

static void *webrtc_wowza_create(obs_data_t *settings, obs_service_t *service)
{
	struct webrtc_wowza *data = bzalloc(sizeof(struct webrtc_wowza));
	webrtc_wowza_update(data, settings);

	UNUSED_PARAMETER(service);
	return data;
}

static obs_properties_t *webrtc_wowza_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *ppts = obs_properties_create();
	obs_property_t *p;

	obs_properties_add_text(ppts, "server", "Server URL", OBS_TEXT_DEFAULT);
	obs_properties_add_text(ppts, "room", "Application Name",
				OBS_TEXT_DEFAULT);

	obs_properties_add_text(ppts, "username", "Stream Name",
				OBS_TEXT_DEFAULT);
	obs_properties_add_text(ppts, "password", "Password", OBS_TEXT_DEFAULT);

	obs_properties_add_text(ppts, "codec", "Codec", OBS_TEXT_DEFAULT);
	obs_properties_add_text(ppts, "protocol", "Protocol", OBS_TEXT_DEFAULT);

	// obs_properties_add_list(ppts, "codec", obs_module_text("Codec"), OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	// obs_property_list_add_string(obs_properties_get(ppts, "codec"), "Automatic", "");
	// obs_property_list_add_string(obs_properties_get(ppts, "codec"), "H264", "h264");
	// obs_property_list_add_string(obs_properties_get(ppts, "codec"), "VP8", "vp8");
	// obs_property_list_add_string(obs_properties_get(ppts, "codec"), "VP9", "vp9");

	// obs_properties_add_list(ppts, "protocol", "Protocol", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	// obs_property_list_add_string(obs_properties_get(ppts, "protocol"), "Automatic", "");
	// obs_property_list_add_string(obs_properties_get(ppts, "protocol"), "UDP", "UDP");
	// obs_property_list_add_string(obs_properties_get(ppts, "protocol"), "TCP", "TCP");

	p = obs_properties_get(ppts, "server");
	obs_property_set_visible(p, true);

	p = obs_properties_get(ppts, "room");
	obs_property_set_visible(p, true);

	p = obs_properties_get(ppts, "username");
	obs_property_set_visible(p, true);

	p = obs_properties_get(ppts, "password");
	obs_property_set_visible(p, false);

	p = obs_properties_get(ppts, "codec");
	obs_property_set_visible(p, true);

	p = obs_properties_get(ppts, "protocol");
	obs_property_set_visible(p, true);

	return ppts;
}

static const char *webrtc_wowza_url(void *data)
{
	struct webrtc_wowza *service = data;
	return service->server;
}

static const char *webrtc_wowza_key(void *data)
{
	UNUSED_PARAMETER(data);
	return "";
}

static const char *webrtc_wowza_room(void *data)
{
	struct webrtc_wowza *service = data;
	return service->room;
}

static const char *webrtc_wowza_username(void *data)
{
	struct webrtc_wowza *service = data;
	return service->username;
}

static const char *webrtc_wowza_password(void *data)
{
	UNUSED_PARAMETER(data);
	return "";
}

static const char *webrtc_wowza_codec(void *data)
{
	struct webrtc_wowza *service = data;
	if (strcmp(service->codec, "Automatic") == 0)
		return "";
	return service->codec;
}

static const char *webrtc_wowza_protocol(void *data)
{
	struct webrtc_wowza *service = data;
	if (strcmp(service->protocol, "Automatic") == 0)
		return "";
	return service->protocol;
}

static const char *webrtc_wowza_get_output_type(void *data)
{
	struct webrtc_wowza *service = data;
	return service->output;
}

struct obs_service_info webrtc_wowza_service = {
	.id = "webrtc_wowza",
	.get_name = webrtc_wowza_name,
	.create = webrtc_wowza_create,
	.destroy = webrtc_wowza_destroy,
	.update = webrtc_wowza_update,
	.get_properties = webrtc_wowza_properties,
	.get_url = webrtc_wowza_url,
	.get_key = webrtc_wowza_key,
	.get_room = webrtc_wowza_room,
	.get_username = webrtc_wowza_username,
	.get_password = webrtc_wowza_password,
	.get_codec = webrtc_wowza_codec,
	.get_protocol = webrtc_wowza_protocol,
	.get_output_type = webrtc_wowza_get_output_type};
