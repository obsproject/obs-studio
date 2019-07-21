#include <obs-module.h>

struct webrtc_evercast {
	char *server;
	char *room;
	char *password;
	char *codec;
	char *output;
};

static const char *webrtc_evercast_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "Evercast";
}

static void webrtc_evercast_update(void *data, obs_data_t *settings)
{
	struct webrtc_evercast *service = data;

	bfree(service->server);
	bfree(service->room);
	bfree(service->password);
	bfree(service->codec);
	bfree(service->output);

	service->server = bstrdup(obs_data_get_string(settings, "server"));
	service->room = bstrdup(obs_data_get_string(settings, "room"));
	service->password = bstrdup(obs_data_get_string(settings, "password"));
	service->codec = bstrdup(obs_data_get_string(settings, "codec"));
	service->output = bstrdup("evercast_output");
}

static void webrtc_evercast_destroy(void *data)
{
	struct webrtc_evercast *service = data;

	bfree(service->server);
	bfree(service->room);
	bfree(service->password);
	bfree(service->codec);
	bfree(service->output);
	bfree(service);
}

static void *webrtc_evercast_create(obs_data_t *settings,
				    obs_service_t *service)
{
	struct webrtc_evercast *data = bzalloc(sizeof(struct webrtc_evercast));
	webrtc_evercast_update(data, settings);

	UNUSED_PARAMETER(service);
	return data;
}

static obs_properties_t *webrtc_evercast_properties(void *unused)
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

static const char *webrtc_evercast_url(void *data)
{
	struct webrtc_evercast *service = data;
	return service->server;
}

static const char *webrtc_evercast_key(void *data)
{
	UNUSED_PARAMETER(data);
	return "";
}

static const char *webrtc_evercast_room(void *data)
{
	struct webrtc_evercast *service = data;
	return service->room;
}

static const char *webrtc_evercast_username(void *data)
{
	UNUSED_PARAMETER(data);
	return "";
}

static const char *webrtc_evercast_password(void *data)
{
	struct webrtc_evercast *service = data;
	return service->password;
}

static const char *webrtc_evercast_codec(void *data)
{
	struct webrtc_evercast *service = data;
	if (strcmp(service->codec, "Automatic") == 0)
		return "";
	return service->codec;
}

static const char *webrtc_evercast_protocol(void *data)
{
	// struct webrtc_evercast *service = data;
	// if (strcmp(service->protocol, "Automatic") == 0)
	// 	return "";
	// return service->protocol;

	UNUSED_PARAMETER(data);
	return "";
}

static const char *webrtc_evercast_get_output_type(void *data)
{
	struct webrtc_evercast *service = data;
	return service->output;
}

struct obs_service_info webrtc_evercast_service = {
	.id = "webrtc_evercast",
	.get_name = webrtc_evercast_name,
	.create = webrtc_evercast_create,
	.destroy = webrtc_evercast_destroy,
	.update = webrtc_evercast_update,
	.get_properties = webrtc_evercast_properties,
	.get_url = webrtc_evercast_url,
	.get_key = webrtc_evercast_key,
	.get_room = webrtc_evercast_room,
	.get_password = webrtc_evercast_password,
	.get_codec = webrtc_evercast_codec,
	.get_protocol = webrtc_evercast_protocol,
	.get_output_type = webrtc_evercast_get_output_type};
