#include <util/platform.h>
#include <util/dstr.h>
#include <obs-module.h>
#include <jansson.h>
#include <obs-config.h>

#include "rtmp-format-ver.h"
#include "service-specific/twitch.h"
#include "service-specific/nimotv.h"
#include "service-specific/showroom.h"
#include "service-specific/dacast.h"
#include "service-specific/amazon-ivs.h"

struct rtmp_common {
	char *service;
	char *protocol;
	char *server;
	char *key;

	struct obs_service_resolution *supported_resolutions;
	size_t supported_resolutions_count;
	int max_fps;

	char **video_codecs;
	char **audio_codecs;

	bool supports_additional_audio_track;
};

static const char *rtmp_common_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("StreamingServices");
}

static json_t *open_services_file(void);
static inline json_t *find_service(json_t *root, const char *name, const char **p_new_name);
static inline bool get_bool_val(json_t *service, const char *key);
static inline const char *get_string_val(json_t *service, const char *key);
static inline int get_int_val(json_t *service, const char *key);

extern void twitch_ingests_refresh(int seconds);
extern void amazon_ivs_ingests_refresh(int seconds);

static void ensure_valid_url(struct rtmp_common *service, json_t *json, obs_data_t *settings)
{
	json_t *servers = json_object_get(json, "servers");
	const char *top_url = NULL;
	json_t *server;
	size_t index;

	if (!service->server || !servers || !json_is_array(servers))
		return;
	if (astrstri(service->service, "Facebook") == NULL)
		return;

	json_array_foreach (servers, index, server) {
		const char *url = get_string_val(server, "url");
		if (!url)
			continue;

		if (!top_url)
			top_url = url;

		if (astrcmpi(service->server, url) == 0)
			return;
	}

	/* server was not found in server list, use first server instead */
	if (top_url) {
		bfree(service->server);
		service->server = bstrdup(top_url);
		obs_data_set_string(settings, "server", top_url);
	}
}

static void update_recommendations(struct rtmp_common *service, json_t *rec)
{
	json_t *sr = json_object_get(rec, "supported resolutions");
	if (sr && json_is_array(sr)) {
		DARRAY(struct obs_service_resolution) res_list;
		json_t *res_obj;
		size_t index;

		da_init(res_list);

		json_array_foreach (sr, index, res_obj) {
			if (!json_is_string(res_obj))
				continue;

			const char *res_str = json_string_value(res_obj);
			struct obs_service_resolution res;
			if (sscanf(res_str, "%dx%d", &res.cx, &res.cy) != 2)
				continue;
			if (res.cx <= 0 || res.cy <= 0)
				continue;

			da_push_back(res_list, &res);
		}

		if (res_list.num) {
			service->supported_resolutions = res_list.array;
			service->supported_resolutions_count = res_list.num;
		}
	}

	service->max_fps = get_int_val(rec, "max fps");
}

#define RTMP_PREFIX "rtmp://"
#define RTMPS_PREFIX "rtmps://"

static const char *get_protocol(json_t *service, obs_data_t *settings)
{
	const char *protocol = get_string_val(service, "protocol");
	if (protocol) {
		return protocol;
	}

	json_t *servers = json_object_get(service, "servers");
	if (!json_is_array(servers))
		return "RTMP";

	json_t *server = json_array_get(servers, 0);
	const char *url = get_string_val(server, "url");

	if (strncmp(url, RTMPS_PREFIX, strlen(RTMPS_PREFIX)) == 0) {
		obs_data_set_string(settings, "protocol", "RTMPS");
		return "RTMPS";
	}

	return "RTMP";
}

static void copy_info_to_settings(json_t *service, obs_data_t *settings);

static void rtmp_common_update(void *data, obs_data_t *settings)
{
	struct rtmp_common *service = data;

	bfree(service->supported_resolutions);
	if (service->video_codecs)
		bfree(service->video_codecs);
	if (service->audio_codecs)
		bfree(service->audio_codecs);
	bfree(service->service);
	bfree(service->protocol);
	bfree(service->server);
	bfree(service->key);

	service->service = bstrdup(obs_data_get_string(settings, "service"));
	service->protocol = bstrdup(obs_data_get_string(settings, "protocol"));
	service->server = bstrdup(obs_data_get_string(settings, "server"));
	service->key = bstrdup(obs_data_get_string(settings, "key"));
	service->supports_additional_audio_track = false;
	service->video_codecs = NULL;
	service->audio_codecs = NULL;
	service->supported_resolutions = NULL;
	service->supported_resolutions_count = 0;
	service->max_fps = 0;

	json_t *root = open_services_file();
	if (root) {
		const char *new_name;
		json_t *serv = find_service(root, service->service, &new_name);

		if (new_name) {
			bfree(service->service);
			service->service = bstrdup(new_name);
		}

		if ((service->protocol == NULL || service->protocol[0] == '\0')) {
			bfree(service->protocol);
			service->protocol = bstrdup(get_protocol(serv, settings));
		}

		if (serv) {
			copy_info_to_settings(serv, settings);

			json_t *rec = json_object_get(serv, "recommended");
			if (json_is_object(rec)) {
				update_recommendations(service, rec);
			}

			service->supports_additional_audio_track =
				get_bool_val(serv, "supports_additional_audio_track");
			ensure_valid_url(service, serv, settings);
		}
	}
	json_decref(root);
}

static void rtmp_common_destroy(void *data)
{
	struct rtmp_common *service = data;

	bfree(service->supported_resolutions);
	if (service->video_codecs)
		bfree(service->video_codecs);
	if (service->audio_codecs)
		bfree(service->audio_codecs);
	bfree(service->service);
	bfree(service->protocol);
	bfree(service->server);
	bfree(service->key);
	bfree(service);
}

static void *rtmp_common_create(obs_data_t *settings, obs_service_t *service)
{
	struct rtmp_common *data = bzalloc(sizeof(struct rtmp_common));
	rtmp_common_update(data, settings);

	UNUSED_PARAMETER(service);
	return data;
}

static inline const char *get_string_val(json_t *service, const char *key)
{
	json_t *str_val = json_object_get(service, key);
	if (!str_val || !json_is_string(str_val))
		return NULL;

	return json_string_value(str_val);
}

static inline int get_int_val(json_t *service, const char *key)
{
	json_t *integer_val = json_object_get(service, key);
	if (!integer_val || !json_is_integer(integer_val))
		return 0;

	return (int)json_integer_value(integer_val);
}

static inline bool get_bool_val(json_t *service, const char *key)
{
	json_t *bool_val = json_object_get(service, key);
	if (!bool_val || !json_is_boolean(bool_val))
		return false;

	return json_is_true(bool_val);
}

static bool is_protocol_available(json_t *service)
{
	const char *protocol = get_string_val(service, "protocol");
	if (protocol)
		return obs_is_output_protocol_registered(protocol);

	/* Test RTMP and RTMPS if no protocol found */
	json_t *servers;
	size_t index;
	json_t *server;
	const char *url;
	bool ret = false;

	servers = json_object_get(service, "servers");
	json_array_foreach (servers, index, server) {
		url = get_string_val(server, "url");

		if (strncmp(url, RTMP_PREFIX, strlen(RTMP_PREFIX)) == 0)
			ret |= obs_is_output_protocol_registered("RTMP");
		else if (strncmp(url, RTMPS_PREFIX, strlen(RTMPS_PREFIX)) == 0)
			ret |= obs_is_output_protocol_registered("RTMPS");
	}

	return ret;
}

static void add_service(obs_property_t *list, json_t *service, bool show_all, const char *cur_service)
{
	json_t *servers;
	const char *name;
	bool common;

	if (!json_is_object(service)) {
		blog(LOG_WARNING, "rtmp-common.c: [add_service] service "
				  "is not an object");
		return;
	}

	name = get_string_val(service, "name");
	if (!name) {
		blog(LOG_WARNING, "rtmp-common.c: [add_service] service "
				  "has no name");
		return;
	}

	common = get_bool_val(service, "common");
	if (!show_all && !common && strcmp(cur_service, name) != 0) {
		return;
	}

	servers = json_object_get(service, "servers");
	if (!servers || !json_is_array(servers)) {
		blog(LOG_WARNING,
		     "rtmp-common.c: [add_service] service "
		     "'%s' has no servers",
		     name);
		return;
	}

	obs_property_list_add_string(list, name, name);
}

static void add_services(obs_property_t *list, json_t *root, bool show_all, const char *cur_service)
{
	json_t *service;
	size_t index;

	if (!json_is_array(root)) {
		blog(LOG_WARNING, "rtmp-common.c: [add_services] JSON file "
				  "root is not an array");
		return;
	}

	json_array_foreach (root, index, service) {
		/* Skip service with non-available protocol */
		if (!is_protocol_available(service))
			continue;
		add_service(list, service, show_all, cur_service);
	}

	service = find_service(root, cur_service, NULL);
	if (!service && cur_service && *cur_service) {
		obs_property_list_insert_string(list, 0, cur_service, cur_service);
		obs_property_list_item_disable(list, 0, true);
	}
}

static json_t *open_json_file(const char *file)
{
	char *file_data = os_quick_read_utf8_file(file);
	json_error_t error;
	json_t *root;
	json_t *list;
	int format_ver;

	if (!file_data)
		return NULL;

	root = json_loads(file_data, JSON_REJECT_DUPLICATES, &error);
	bfree(file_data);

	if (!root) {
		blog(LOG_WARNING,
		     "rtmp-common.c: [open_json_file] "
		     "Error reading JSON file (%d): %s",
		     error.line, error.text);
		return NULL;
	}

	format_ver = get_int_val(root, "format_version");

	if (format_ver != RTMP_SERVICES_FORMAT_VERSION) {
		blog(LOG_DEBUG,
		     "rtmp-common.c: [open_json_file] "
		     "Wrong format version (%d), expected %d",
		     format_ver, RTMP_SERVICES_FORMAT_VERSION);
		json_decref(root);
		return NULL;
	}

	list = json_object_get(root, "services");
	if (list)
		json_incref(list);
	json_decref(root);

	if (!list) {
		blog(LOG_WARNING, "rtmp-common.c: [open_json_file] "
				  "No services list");
		return NULL;
	}

	return list;
}

static json_t *open_services_file(void)
{
	char *file;
	json_t *root = NULL;

	file = obs_module_config_path("services.json");
	if (file) {
		root = open_json_file(file);
		bfree(file);
	}

	if (!root) {
		file = obs_module_file("services.json");
		if (file) {
			root = open_json_file(file);
			bfree(file);
		}
	}

	return root;
}

static void build_service_list(obs_property_t *list, json_t *root, bool show_all, const char *cur_service)
{
	obs_property_list_clear(list);
	add_services(list, root, show_all, cur_service);
}

static void properties_data_destroy(void *data)
{
	json_t *root = data;
	if (root)
		json_decref(root);
}

static bool fill_twitch_servers_locked(obs_property_t *servers_prop)
{
	size_t count = twitch_ingest_count();

	obs_property_list_add_string(servers_prop, obs_module_text("Server.Auto"), "auto");

	if (count <= 1)
		return false;

	for (size_t i = 0; i < count; i++) {
		struct ingest twitch_ing = twitch_ingest(i);
		obs_property_list_add_string(servers_prop, twitch_ing.name, twitch_ing.url);
	}

	return true;
}

static inline bool fill_twitch_servers(obs_property_t *servers_prop)
{
	bool success;

	twitch_ingests_lock();
	success = fill_twitch_servers_locked(servers_prop);
	twitch_ingests_unlock();

	return success;
}

static bool fill_amazon_ivs_servers_locked(obs_property_t *servers_prop)
{
	struct dstr name_buffer = {0};
	size_t count = amazon_ivs_ingest_count();
	bool rtmps_available = obs_is_output_protocol_registered("RTMPS");

	if (rtmps_available) {
		obs_property_list_add_string(servers_prop, obs_module_text("Server.AutoRTMPS"), "auto-rtmps");
	}
	obs_property_list_add_string(servers_prop, obs_module_text("Server.AutoRTMP"), "auto-rtmp");

	if (count <= 1)
		return false;

	if (rtmps_available) {
		for (size_t i = 0; i < count; i++) {
			struct ingest amazon_ivs_ing = amazon_ivs_ingest(i);
			dstr_printf(&name_buffer, "%s (RTMPS)", amazon_ivs_ing.name);
			obs_property_list_add_string(servers_prop, name_buffer.array, amazon_ivs_ing.rtmps_url);
		}
	}

	for (size_t i = 0; i < count; i++) {
		struct ingest amazon_ivs_ing = amazon_ivs_ingest(i);
		dstr_printf(&name_buffer, "%s (RTMP)", amazon_ivs_ing.name);
		obs_property_list_add_string(servers_prop, name_buffer.array, amazon_ivs_ing.url);
	}

	dstr_free(&name_buffer);

	return true;
}

static inline bool fill_amazon_ivs_servers(obs_property_t *servers_prop)
{
	bool success;

	amazon_ivs_ingests_lock();
	success = fill_amazon_ivs_servers_locked(servers_prop);
	amazon_ivs_ingests_unlock();

	return success;
}

static void fill_servers(obs_property_t *servers_prop, json_t *service, const char *name)
{
	json_t *servers, *server;
	size_t index;

	obs_property_list_clear(servers_prop);

	servers = json_object_get(service, "servers");

	if (!json_is_array(servers)) {
		blog(LOG_WARNING,
		     "rtmp-common.c: [fill_servers] "
		     "Servers for service '%s' not a valid object",
		     name);
		return;
	}

	/* Assumption: Twitch should be RTMP only, so no RTMPS check */
	if (strcmp(name, "Twitch") == 0) {
		if (fill_twitch_servers(servers_prop))
			return;
	}

	/* Assumption:  Nimo TV should be RTMP only, so no RTMPS check in the ingest */
	if (strcmp(name, "Nimo TV") == 0) {
		obs_property_list_add_string(servers_prop, obs_module_text("Server.Auto"), "auto");
	}

	if (strcmp(name, "Amazon IVS") == 0) {
		if (fill_amazon_ivs_servers(servers_prop))
			return;
	}

	json_array_foreach (servers, index, server) {
		const char *server_name = get_string_val(server, "name");
		const char *url = get_string_val(server, "url");

		if (!server_name || !url)
			continue;

		/* Skip RTMPS server if protocol not registered */
		if ((!obs_is_output_protocol_registered("RTMPS")) && (strncmp(url, "rtmps://", 8) == 0))
			continue;

		obs_property_list_add_string(servers_prop, server_name, url);
	}
}

static void copy_string_from_json_if_available(json_t *service, obs_data_t *settings, const char *name)
{
	const char *string = get_string_val(service, name);
	if (string)
		obs_data_set_string(settings, name, string);
}

static void fill_more_info_link(json_t *service, obs_data_t *settings)
{
	copy_string_from_json_if_available(service, settings, "more_info_link");
}

static void fill_stream_key_link(json_t *service, obs_data_t *settings)
{
	copy_string_from_json_if_available(service, settings, "stream_key_link");
}

static void update_protocol(json_t *service, obs_data_t *settings)
{
	const char *protocol = get_string_val(service, "protocol");
	if (protocol) {
		obs_data_set_string(settings, "protocol", protocol);
		return;
	}

	json_t *servers = json_object_get(service, "servers");
	if (!json_is_array(servers))
		return;

	json_t *server = json_array_get(servers, 0);
	const char *url = get_string_val(server, "url");

	if (strncmp(url, RTMPS_PREFIX, strlen(RTMPS_PREFIX)) == 0) {
		obs_data_set_string(settings, "protocol", "RTMPS");
		return;
	}

	obs_data_set_string(settings, "protocol", "RTMP");
}

static void copy_info_to_settings(json_t *service, obs_data_t *settings)
{
	const char *name = obs_data_get_string(settings, "service");

	fill_more_info_link(service, settings);
	fill_stream_key_link(service, settings);
	copy_string_from_json_if_available(service, settings, "multitrack_video_configuration_url");
	copy_string_from_json_if_available(service, settings, "multitrack_video_name");
	if (!obs_data_has_user_value(settings, "multitrack_video_name")) {
		obs_data_set_string(settings, "multitrack_video_name", "Multitrack Video");
	}

	const char *learn_more_link_url = get_string_val(service, "multitrack_video_learn_more_link");
	struct dstr learn_more_link = {0};
	if (learn_more_link_url) {
		dstr_init_copy(&learn_more_link, obs_module_text("MultitrackVideo.LearnMoreLink"));
		dstr_replace(&learn_more_link, "%1", learn_more_link_url);
	}

	struct dstr str;
	dstr_init_copy(&str, obs_module_text("MultitrackVideo.Disclaimer"));
	dstr_replace(&str, "%1", obs_data_get_string(settings, "multitrack_video_name"));
	dstr_replace(&str, "%2", name);
	if (learn_more_link.array) {
		dstr_cat(&str, learn_more_link.array);
	}
	obs_data_set_string(settings, "multitrack_video_disclaimer", str.array);
	dstr_free(&learn_more_link);
	dstr_free(&str);

	update_protocol(service, settings);
}

static inline json_t *find_service(json_t *root, const char *name, const char **p_new_name)
{
	size_t index;
	json_t *service;

	if (p_new_name)
		*p_new_name = NULL;

	json_array_foreach (root, index, service) {
		/* skip service with non-available protocol */
		if (!is_protocol_available(service))
			continue;

		const char *cur_name = get_string_val(service, "name");

		if (strcmp(name, cur_name) == 0)
			return service;

		/* check for alternate names */
		json_t *alt_names = json_object_get(service, "alt_names");
		size_t alt_name_idx;
		json_t *alt_name_obj;

		json_array_foreach (alt_names, alt_name_idx, alt_name_obj) {
			const char *alt_name = json_string_value(alt_name_obj);
			if (alt_name && strcmp(name, alt_name) == 0) {
				if (p_new_name)
					*p_new_name = cur_name;
				return service;
			}
		}
	}

	return NULL;
}

static bool service_selected(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	const char *name = obs_data_get_string(settings, "service");
	json_t *root = obs_properties_get_param(props);
	json_t *service;
	const char *new_name;

	if (!name || !*name)
		return false;

	service = find_service(root, name, &new_name);
	if (!service) {
		const char *server = obs_data_get_string(settings, "server");

		obs_property_list_insert_string(p, 0, name, name);
		obs_property_list_item_disable(p, 0, true);

		p = obs_properties_get(props, "server");
		obs_property_list_insert_string(p, 0, server, server);
		obs_property_list_item_disable(p, 0, true);
		return true;
	}
	if (new_name) {
		name = new_name;
		obs_data_set_string(settings, "service", name);
	}

	fill_servers(obs_properties_get(props, "server"), service, name);
	copy_info_to_settings(service, settings);

	return true;
}

static bool show_all_services_toggled(obs_properties_t *ppts, obs_property_t *p, obs_data_t *settings)
{
	const char *cur_service = obs_data_get_string(settings, "service");
	bool show_all = obs_data_get_bool(settings, "show_all");

	json_t *root = obs_properties_get_param(ppts);
	if (!root)
		return false;

	build_service_list(obs_properties_get(ppts, "service"), root, show_all, cur_service);

	UNUSED_PARAMETER(p);
	return true;
}

static obs_properties_t *rtmp_common_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *ppts = obs_properties_create();
	obs_property_t *p;
	json_t *root;

	root = open_services_file();
	if (root)
		obs_properties_set_param(ppts, root, properties_data_destroy);

	p = obs_properties_add_list(ppts, "service", obs_module_text("Service"), OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);

	obs_property_set_modified_callback(p, service_selected);

	p = obs_properties_add_bool(ppts, "show_all", obs_module_text("ShowAll"));

	obs_property_set_modified_callback(p, show_all_services_toggled);

	obs_properties_add_list(ppts, "server", obs_module_text("Server"), OBS_COMBO_TYPE_LIST,
				OBS_COMBO_FORMAT_STRING);

	obs_properties_add_text(ppts, "key", obs_module_text("StreamKey"), OBS_TEXT_PASSWORD);
	return ppts;
}

static int get_bitrate_matrix_max(json_t *array);

static void apply_video_encoder_settings(obs_data_t *settings, json_t *recommended)
{
	json_t *item = json_object_get(recommended, "keyint");
	if (json_is_integer(item)) {
		int keyint = (int)json_integer_value(item);
		obs_data_set_int(settings, "keyint_sec", keyint);
	}

	obs_data_set_string(settings, "rate_control", "CBR");

	item = json_object_get(recommended, "profile");
	obs_data_item_t *enc_item = obs_data_item_byname(settings, "profile");
	if (json_is_string(item) && obs_data_item_gettype(enc_item) == OBS_DATA_STRING) {
		const char *profile = json_string_value(item);
		obs_data_set_string(settings, "profile", profile);
	}

	obs_data_item_release(&enc_item);

	int max_bitrate = 0;
	item = json_object_get(recommended, "bitrate matrix");
	if (json_is_array(item)) {
		max_bitrate = get_bitrate_matrix_max(item);
	}

	item = json_object_get(recommended, "max video bitrate");
	if (!max_bitrate && json_is_integer(item)) {
		max_bitrate = (int)json_integer_value(item);
	}

	if (max_bitrate && obs_data_get_int(settings, "bitrate") > max_bitrate) {
		obs_data_set_int(settings, "bitrate", max_bitrate);
		obs_data_set_int(settings, "buffer_size", max_bitrate);
	}

	item = json_object_get(recommended, "bframes");
	if (json_is_integer(item)) {
		int bframes = (int)json_integer_value(item);
		obs_data_set_int(settings, "bf", bframes);
	}

	item = json_object_get(recommended, "x264opts");
	if (json_is_string(item)) {
		const char *x264_settings = json_string_value(item);
		const char *cur_settings = obs_data_get_string(settings, "x264opts");
		struct dstr opts;

		dstr_init_copy(&opts, cur_settings);
		if (!dstr_is_empty(&opts))
			dstr_cat(&opts, " ");
		dstr_cat(&opts, x264_settings);

		obs_data_set_string(settings, "x264opts", opts.array);
		dstr_free(&opts);
	}
}

static void apply_audio_encoder_settings(obs_data_t *settings, json_t *recommended)
{
	json_t *item = json_object_get(recommended, "max audio bitrate");
	if (json_is_integer(item)) {
		int max_bitrate = (int)json_integer_value(item);
		if (obs_data_get_int(settings, "bitrate") > max_bitrate)
			obs_data_set_int(settings, "bitrate", max_bitrate);
	}
}

static void initialize_output(struct rtmp_common *service, json_t *root, obs_data_t *video_settings,
			      obs_data_t *audio_settings)
{
	json_t *json_service = find_service(root, service->service, NULL);
	json_t *recommended;

	if (!json_service) {
		if (service->service && *service->service)
			blog(LOG_WARNING,
			     "rtmp-common.c: [initialize_output] "
			     "Could not find service '%s'",
			     service->service);
		return;
	}

	recommended = json_object_get(json_service, "recommended");
	if (!recommended)
		return;

	if (video_settings)
		apply_video_encoder_settings(video_settings, recommended);
	if (audio_settings)
		apply_audio_encoder_settings(audio_settings, recommended);
}

static void rtmp_common_apply_settings(void *data, obs_data_t *video_settings, obs_data_t *audio_settings)
{
	struct rtmp_common *service = data;
	json_t *root = open_services_file();

	if (root) {
		initialize_output(service, root, video_settings, audio_settings);
		json_decref(root);
	}
}

static const char *rtmp_common_url(void *data)
{
	struct rtmp_common *service = data;

	if (service->service && strcmp(service->service, "Twitch") == 0) {
		if (service->server && strcmp(service->server, "auto") == 0) {
			struct ingest twitch_ing;

			twitch_ingests_refresh(3);

			twitch_ingests_lock();
			twitch_ing = twitch_ingest(0);
			twitch_ingests_unlock();

			return twitch_ing.url;
		}
	}

	if (service->service && strcmp(service->service, "Amazon IVS") == 0) {
		if (service->server && strncmp(service->server, "auto", 4) == 0) {
			struct ingest amazon_ivs_ing;
			bool rtmp = strcmp(service->server, "auto-rtmp") == 0;

			amazon_ivs_ingests_refresh(3);

			amazon_ivs_ingests_lock();
			amazon_ivs_ing = amazon_ivs_ingest(0);
			amazon_ivs_ingests_unlock();

			return rtmp ? amazon_ivs_ing.url : amazon_ivs_ing.rtmps_url;
		}
	}

	if (service->service && strcmp(service->service, "Nimo TV") == 0) {
		if (service->server && strcmp(service->server, "auto") == 0) {
			return nimotv_get_ingest(service->key);
		}
	}

	if (service->service && strcmp(service->service, "SHOWROOM") == 0) {
		if (service->server && service->key) {
			struct showroom_ingest *ingest;
			ingest = showroom_get_ingest(service->server, service->key);
			return ingest->url;
		}
	}

	if (service->service && strcmp(service->service, "Dacast") == 0) {
		if (service->server && service->key) {
			dacast_ingests_load_data(service->server, service->key);

			struct dacast_ingest *ingest;
			ingest = dacast_ingest(service->key);
			return ingest->url;
		}
	}
	return service->server;
}

static const char *rtmp_common_key(void *data)
{
	struct rtmp_common *service = data;
	if (service->service && strcmp(service->service, "SHOWROOM") == 0) {
		if (service->server && service->key) {
			struct showroom_ingest *ingest;
			ingest = showroom_get_ingest(service->server, service->key);
			return ingest->key;
		}
	}

	if (service->service && strcmp(service->service, "Dacast") == 0) {
		if (service->key) {
			struct dacast_ingest *ingest;
			ingest = dacast_ingest(service->key);
			return ingest->streamkey;
		}
	}
	return service->key;
}

static void rtmp_common_get_supported_resolutions(void *data, struct obs_service_resolution **resolutions,
						  size_t *count)
{
	struct rtmp_common *service = data;

	if (service->supported_resolutions_count) {
		*count = service->supported_resolutions_count;
		*resolutions = bmemdup(service->supported_resolutions, *count * sizeof(struct obs_service_resolution));
	} else {
		*count = 0;
		*resolutions = NULL;
	}
}

static void rtmp_common_get_max_fps(void *data, int *fps)
{
	struct rtmp_common *service = data;
	*fps = service->max_fps;
}

static int get_bitrate_matrix_max(json_t *array)
{
	size_t index;
	json_t *item;

	struct obs_video_info ovi;
	if (!obs_get_video_info(&ovi))
		return 0;

	double cur_fps = (double)ovi.fps_num / (double)ovi.fps_den;

	json_array_foreach (array, index, item) {
		if (!json_is_object(item))
			continue;

		const char *res = get_string_val(item, "res");
		double fps = (double)get_int_val(item, "fps") + 0.0000001;
		int bitrate = get_int_val(item, "max bitrate");

		if (!res)
			continue;

		int cx, cy;
		int c = sscanf(res, "%dx%d", &cx, &cy);
		if (c != 2)
			continue;

		if ((int)ovi.output_width == cx && (int)ovi.output_height == cy && cur_fps <= fps)
			return bitrate;
	}

	return 0;
}

static void rtmp_common_get_max_bitrate(void *data, int *video_bitrate, int *audio_bitrate)
{
	struct rtmp_common *service = data;
	json_t *root = open_services_file();
	json_t *item;

	if (!root)
		return;

	json_t *json_service = find_service(root, service->service, NULL);
	if (!json_service) {
		goto fail;
	}

	json_t *recommended = json_object_get(json_service, "recommended");
	if (!recommended) {
		goto fail;
	}

	if (audio_bitrate) {
		item = json_object_get(recommended, "max audio bitrate");
		if (json_is_integer(item))
			*audio_bitrate = (int)json_integer_value(item);
	}

	if (video_bitrate) {
		int bitrate = 0;
		item = json_object_get(recommended, "bitrate matrix");
		if (json_is_array(item)) {
			bitrate = get_bitrate_matrix_max(item);
		}
		if (!bitrate) {
			item = json_object_get(recommended, "max video bitrate");
			if (json_is_integer(item))
				bitrate = (int)json_integer_value(item);
		}

		*video_bitrate = bitrate;
	}

fail:
	json_decref(root);
}

static const char **rtmp_common_get_supported_video_codecs(void *data)
{
	struct rtmp_common *service = data;

	if (service->video_codecs)
		return (const char **)service->video_codecs;

	struct dstr codecs = {0};
	json_t *root = open_services_file();
	if (!root)
		return NULL;

	json_t *json_service = find_service(root, service->service, NULL);
	if (!json_service) {
		goto fail;
	}

	json_t *json_video_codecs = json_object_get(json_service, "supported video codecs");
	if (!json_is_array(json_video_codecs)) {
		goto fail;
	}

	size_t index;
	json_t *item;

	json_array_foreach (json_video_codecs, index, item) {
		char codec[16];

		snprintf(codec, sizeof(codec), "%s", json_string_value(item));
		if (codecs.len)
			dstr_cat(&codecs, ";");
		dstr_cat(&codecs, codec);
	}

	service->video_codecs = strlist_split(codecs.array, ';', false);
	dstr_free(&codecs);

fail:
	json_decref(root);
	return (const char **)service->video_codecs;
}

static const char **rtmp_common_get_supported_audio_codecs(void *data)
{
	struct rtmp_common *service = data;

	if (service->audio_codecs)
		return (const char **)service->audio_codecs;

	struct dstr codecs = {0};
	json_t *root = open_services_file();
	if (!root)
		return NULL;

	json_t *json_service = find_service(root, service->service, NULL);
	if (!json_service) {
		goto fail;
	}

	json_t *json_audio_codecs = json_object_get(json_service, "supported audio codecs");
	if (!json_is_array(json_audio_codecs)) {
		goto fail;
	}

	size_t index;
	json_t *item;

	json_array_foreach (json_audio_codecs, index, item) {
		char codec[16];

		snprintf(codec, sizeof(codec), "%s", json_string_value(item));
		if (codecs.len)
			dstr_cat(&codecs, ";");
		dstr_cat(&codecs, codec);
	}

	service->audio_codecs = strlist_split(codecs.array, ';', false);
	dstr_free(&codecs);

fail:
	json_decref(root);
	return (const char **)service->audio_codecs;
}

static const char *rtmp_common_username(void *data)
{
	struct rtmp_common *service = data;
	if (service->service && strcmp(service->service, "Dacast") == 0) {
		if (service->key) {
			struct dacast_ingest *ingest;
			ingest = dacast_ingest(service->key);
			return ingest->username;
		}
	}
	return NULL;
}

static const char *rtmp_common_password(void *data)
{
	struct rtmp_common *service = data;
	if (service->service && strcmp(service->service, "Dacast") == 0) {
		if (service->key) {
			struct dacast_ingest *ingest;
			ingest = dacast_ingest(service->key);
			return ingest->password;
		}
	}
	return NULL;
}

static const char *rtmp_common_get_protocol(void *data)
{
	struct rtmp_common *service = data;

	return service->protocol ? service->protocol : "RTMP";
}

static const char *rtmp_common_get_connect_info(void *data, uint32_t type)
{
	switch ((enum obs_service_connect_info)type) {
	case OBS_SERVICE_CONNECT_INFO_SERVER_URL:
		return rtmp_common_url(data);
	case OBS_SERVICE_CONNECT_INFO_STREAM_ID:
		return rtmp_common_key(data);
	case OBS_SERVICE_CONNECT_INFO_USERNAME:
		return rtmp_common_username(data);
	case OBS_SERVICE_CONNECT_INFO_PASSWORD:
		return rtmp_common_password(data);
	case OBS_SERVICE_CONNECT_INFO_ENCRYPT_PASSPHRASE: {
		const char *protocol = rtmp_common_get_protocol(data);

		if ((strcmp(protocol, "SRT") == 0))
			return rtmp_common_password(data);
		else if ((strcmp(protocol, "RIST") == 0))
			return rtmp_common_key(data);

		break;
	}
	case OBS_SERVICE_CONNECT_INFO_BEARER_TOKEN:
		return NULL;
	}

	return NULL;
}

static bool rtmp_common_can_try_to_connect(void *data)
{
	struct rtmp_common *service = data;
	const char *key = rtmp_common_key(data);

	if (service->service && strcmp(service->service, "Dacast") == 0)
		return (key != NULL && key[0] != '\0');

	const char *url = rtmp_common_url(data);

	return (url != NULL && url[0] != '\0') && (key != NULL && key[0] != '\0');
}

struct obs_service_info rtmp_common_service = {
	.id = "rtmp_common",
	.get_name = rtmp_common_getname,
	.create = rtmp_common_create,
	.destroy = rtmp_common_destroy,
	.update = rtmp_common_update,
	.get_properties = rtmp_common_properties,
	.get_protocol = rtmp_common_get_protocol,
	.get_url = rtmp_common_url,
	.get_key = rtmp_common_key,
	.get_username = rtmp_common_username,
	.get_password = rtmp_common_password,
	.get_connect_info = rtmp_common_get_connect_info,
	.apply_encoder_settings = rtmp_common_apply_settings,
	.get_supported_resolutions = rtmp_common_get_supported_resolutions,
	.get_max_fps = rtmp_common_get_max_fps,
	.get_max_bitrate = rtmp_common_get_max_bitrate,
	.get_supported_video_codecs = rtmp_common_get_supported_video_codecs,
	.get_supported_audio_codecs = rtmp_common_get_supported_audio_codecs,
	.can_try_to_connect = rtmp_common_can_try_to_connect,
};
