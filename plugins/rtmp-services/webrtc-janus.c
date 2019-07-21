#include <obs-module.h>

struct webrtc_janus {
	char *server;
	char *room;
	char *password;
	char *codec;
	char *output;
};

static const char *webrtc_janus_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "Janus WebRTC Server";
}

static void webrtc_janus_update(void *data, obs_data_t *settings)
{
	struct webrtc_janus *service = data;

	bfree(service->server);
	bfree(service->room);
	bfree(service->password);
	bfree(service->codec);
	bfree(service->output);

	service->server = bstrdup(obs_data_get_string(settings, "server"));
	service->room = bstrdup(obs_data_get_string(settings, "room"));
	service->password = bstrdup(obs_data_get_string(settings, "password"));
	service->codec = bstrdup(obs_data_get_string(settings, "codec"));
	service->output = bstrdup("janus_output");
}

static void webrtc_janus_destroy(void *data)
{
	struct webrtc_janus *service = data;

	bfree(service->server);
	bfree(service->room);
	bfree(service->password);
	bfree(service->codec);
	bfree(service->output);
	bfree(service);
}

static void *webrtc_janus_create(obs_data_t *settings, obs_service_t *service)
{
	struct webrtc_janus *data = bzalloc(sizeof(struct webrtc_janus));
	webrtc_janus_update(data, settings);

	UNUSED_PARAMETER(service);
	return data;
}

static obs_properties_t *webrtc_janus_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *ppts = obs_properties_create();
	obs_property_t *p;

	obs_properties_add_text(ppts, "server", "Server Name",
				OBS_TEXT_DEFAULT);
	obs_properties_add_text(ppts, "room", "Server Room", OBS_TEXT_DEFAULT);

	obs_properties_add_text(ppts, "username", "Username", OBS_TEXT_DEFAULT);
	obs_properties_add_text(ppts, "password", "Stream Key",
				OBS_TEXT_DEFAULT);

	obs_properties_add_text(ppts, "codec", "Codec", OBS_TEXT_DEFAULT);
	obs_properties_add_text(ppts, "protocol", "Protocol", OBS_TEXT_DEFAULT);

	// obs_properties_add_list(ppts, "codec", "Codec", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	// obs_property_list_add_string(obs_properties_get(ppts, "codec"), "Automatic", "");
	// obs_property_list_add_string(obs_properties_get(ppts, "codec"), "H264", "h264");
	// obs_property_list_add_string(obs_properties_get(ppts, "codec"), "VP8", "vp8");
	// obs_property_list_add_string(obs_properties_get(ppts, "codec"), "VP9", "vp9");

	p = obs_properties_get(ppts, "server");
	obs_property_set_visible(p, true);

	p = obs_properties_get(ppts, "room");
	obs_property_set_visible(p, true);

	p = obs_properties_get(ppts, "username");
	obs_property_set_visible(p, false);

	p = obs_properties_get(ppts, "password");
	obs_property_set_visible(p, true);

	p = obs_properties_get(ppts, "codec");
	obs_property_set_visible(p, true);

	p = obs_properties_get(ppts, "protocol");
	obs_property_set_visible(p, false);

	return ppts;
}

static const char *webrtc_janus_url(void *data)
{
	struct webrtc_janus *service = data;
	return service->server;
}

static const char *webrtc_janus_key(void *data)
{
	UNUSED_PARAMETER(data);
	return "";
}

static const char *webrtc_janus_room(void *data)
{
	struct webrtc_janus *service = data;
	return service->room;
}

static const char *webrtc_janus_username(void *data)
{
	UNUSED_PARAMETER(data);
	return "";
}

static const char *webrtc_janus_password(void *data)
{
	struct webrtc_janus *service = data;
	return service->password;
}

static const char *webrtc_janus_codec(void *data)
{
	struct webrtc_janus *service = data;
	if (strcmp(service->codec, "Automatic") == 0)
		return "";
	return service->codec;
}

static const char *webrtc_janus_protocol(void *data)
{
	// struct webrtc_janus *service = data;
	// if (strcmp(service->protocol, "Automatic") == 0)
	// 	return "";
	// return service->protocol;

	UNUSED_PARAMETER(data);
	return "";
}

static const char *webrtc_janus_get_output_type(void *data)
{
	struct webrtc_janus *service = data;
	return service->output;
}

struct obs_service_info webrtc_janus_service = {
	.id = "webrtc_janus",
	.get_name = webrtc_janus_name,
	.create = webrtc_janus_create,
	.destroy = webrtc_janus_destroy,
	.update = webrtc_janus_update,
	.get_properties = webrtc_janus_properties,
	.get_url = webrtc_janus_url,
	.get_key = webrtc_janus_key,
	.get_room = webrtc_janus_room,
	.get_username = webrtc_janus_username,
	.get_password = webrtc_janus_password,
	.get_codec = webrtc_janus_codec,
	.get_protocol = webrtc_janus_protocol,
	.get_output_type = webrtc_janus_get_output_type};
