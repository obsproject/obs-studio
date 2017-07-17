#include <util/platform.h>
#include <util/dstr.h>
#include <obs-module.h>
#include <jansson.h>

#include "rtmp-format-ver.h"

struct rtmp_common {
	char *service;
	char *server;
	char *key;

	char *output;
};

static const char *rtmp_common_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("StreamingServices");
}

static json_t *open_services_file(void);
static inline json_t *find_service(json_t *root, const char *name);
static inline const char *get_string_val(json_t *service, const char *key);

static void rtmp_common_update(void *data, obs_data_t *settings)
{
	struct rtmp_common *service = data;

	bfree(service->service);
	bfree(service->server);
	bfree(service->output);
	bfree(service->key);

	service->service = bstrdup(obs_data_get_string(settings, "service"));
	service->server  = bstrdup(obs_data_get_string(settings, "server"));
	service->key     = bstrdup(obs_data_get_string(settings, "key"));
	service->output  = NULL;

	json_t *root = open_services_file();
	if (root) {
		json_t *serv = find_service(root, service->service);
		if (serv) {
			json_t *rec = json_object_get(serv, "recommended");
			if (rec && json_is_object(rec)) {
				const char *out = get_string_val(rec, "output");
				if (out)
					service->output = bstrdup(out);
			}
		}
	}
	json_decref(root);

	if (!service->output)
		service->output = bstrdup("rtmp_output");
}

static void rtmp_common_destroy(void *data)
{
	struct rtmp_common *service = data;

	bfree(service->service);
	bfree(service->server);
	bfree(service->output);
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

static void add_service(obs_property_t *list, json_t *service, bool show_all,
		const char *cur_service)
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
		blog(LOG_WARNING, "rtmp-common.c: [add_service] service "
		                  "'%s' has no servers", name);
		return;
	}

	obs_property_list_add_string(list, name, name);
}

static void add_services(obs_property_t *list, json_t *root, bool show_all,
		const char *cur_service)
{
	json_t *service;
	size_t index;

	if (!json_is_array(root)) {
		blog(LOG_WARNING, "rtmp-common.c: [add_services] JSON file "
		                  "root is not an array");
		return;
	}

	json_array_foreach (root, index, service) {
		add_service(list, service, show_all, cur_service);
	}

	service = find_service(root, cur_service);
	if (!service && cur_service && *cur_service) {
		obs_property_list_insert_string(list, 0, cur_service,
				cur_service);
		obs_property_list_item_disable(list, 0, true);
	}
}

static json_t *open_json_file(const char *file)
{
	char         *file_data = os_quick_read_utf8_file(file);
	json_error_t error;
	json_t       *root;
	json_t       *list;
	int          format_ver;

	if (!file_data)
		return NULL;

	root = json_loads(file_data, JSON_REJECT_DUPLICATES, &error);
	bfree(file_data);

	if (!root) {
		blog(LOG_WARNING, "rtmp-common.c: [open_json_file] "
		                  "Error reading JSON file (%d): %s",
		                  error.line, error.text);
		return NULL;
	}

	format_ver = get_int_val(root, "format_version");

	if (format_ver != RTMP_SERVICES_FORMAT_VERSION) {
		blog(LOG_DEBUG, "rtmp-common.c: [open_json_file] "
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

static void build_service_list(obs_property_t *list, json_t *root,
		bool show_all, const char *cur_service)
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

static void fill_servers(obs_property_t *servers_prop, json_t *service,
		const char *name)
{
	json_t *servers, *server;
	size_t index;

	obs_property_list_clear(servers_prop);

	servers = json_object_get(service, "servers");

	if (!json_is_array(servers)) {
		blog(LOG_WARNING, "rtmp-common.c: [fill_servers] "
		                  "Servers for service '%s' not a valid object",
		                  name);
		return;
	}

	json_array_foreach (servers, index, server) {
		const char *server_name = get_string_val(server, "name");
		const char *url         = get_string_val(server, "url");

		if (!server_name || !url)
			continue;

		obs_property_list_add_string(servers_prop, server_name, url);
	}
}

static inline json_t *find_service(json_t *root, const char *name)
{
	size_t index;
	json_t *service;

	json_array_foreach (root, index, service) {
		const char *cur_name = get_string_val(service, "name");

		if (strcmp(name, cur_name) == 0)
			return service;
	}

	return NULL;
}

static bool service_selected(obs_properties_t *props, obs_property_t *p,
		obs_data_t *settings)
{
	const char *name = obs_data_get_string(settings, "service");
	json_t *root     = obs_properties_get_param(props);
	json_t *service;

	if (!name || !*name)
		return false;

	service = find_service(root, name);
	if (!service) {
		const char *server = obs_data_get_string(settings, "server");

		obs_property_list_insert_string(p, 0, name, name);
		obs_property_list_item_disable(p, 0, true);

		p = obs_properties_get(props, "server");
		obs_property_list_insert_string(p, 0, server, server);
		obs_property_list_item_disable(p, 0, true);
		return true;
	}

	fill_servers(obs_properties_get(props, "server"), service, name);

	return true;
}

static bool show_all_services_toggled(obs_properties_t *ppts,
		obs_property_t *p, obs_data_t *settings)
{
	const char *cur_service = obs_data_get_string(settings, "service");
	bool show_all = obs_data_get_bool(settings, "show_all");

	json_t *root = obs_properties_get_param(ppts);
	if (!root)
		return false;

	build_service_list(obs_properties_get(ppts, "service"), root, show_all,
			cur_service);

	UNUSED_PARAMETER(p);
	return true;
}

static obs_properties_t *rtmp_common_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *ppts = obs_properties_create();
	obs_property_t   *p;
	json_t           *root;

	root = open_services_file();
	if (root)
		obs_properties_set_param(ppts, root, properties_data_destroy);

	p = obs_properties_add_list(ppts, "service",
			obs_module_text("Service"),
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	obs_property_set_modified_callback(p, service_selected);

	p = obs_properties_add_bool(ppts, "show_all",
			obs_module_text("ShowAll"));

	obs_property_set_modified_callback(p, show_all_services_toggled);

	obs_properties_add_list(ppts, "server", obs_module_text("Server"),
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	obs_properties_add_text(ppts, "key", obs_module_text("StreamKey"),
			OBS_TEXT_PASSWORD);
	return ppts;
}

static void apply_video_encoder_settings(obs_data_t *settings,
		json_t *recommended)
{
	json_t *item = json_object_get(recommended, "keyint");
	if (item && json_is_integer(item)) {
		int keyint = (int)json_integer_value(item);
		obs_data_set_int(settings, "keyint_sec", keyint);
	}

	obs_data_set_string(settings, "rate_control", "CBR");

	item = json_object_get(recommended, "profile");
	if (item && json_is_string(item)) {
		const char *profile = json_string_value(item);
		obs_data_set_string(settings, "profile", profile);
	}

	item = json_object_get(recommended, "max video bitrate");
	if (item && json_is_integer(item)) {
		int max_bitrate = (int)json_integer_value(item);
		if (obs_data_get_int(settings, "bitrate") > max_bitrate) {
			obs_data_set_int(settings, "bitrate", max_bitrate);
			obs_data_set_int(settings, "buffer_size", max_bitrate);
		}
	}

	item = json_object_get(recommended, "bframes");
	if (item && json_is_integer(item))
		obs_data_set_int(settings, "bf", 0);

	item = json_object_get(recommended, "x264opts");
	if (item && json_is_string(item)) {
		const char *x264_settings = json_string_value(item);
		const char *cur_settings =
			obs_data_get_string(settings, "x264opts");
		struct dstr opts;

		dstr_init_copy(&opts, cur_settings);
		if (!dstr_is_empty(&opts))
			dstr_cat(&opts, " ");
		dstr_cat(&opts, x264_settings);

		obs_data_set_string(settings, "x264opts", opts.array);
		dstr_free(&opts);
	}
}

static void apply_audio_encoder_settings(obs_data_t *settings,
		json_t *recommended)
{
	json_t *item = json_object_get(recommended, "max audio bitrate");
	if (item && json_is_integer(item)) {
		int max_bitrate = (int)json_integer_value(item);
		if (obs_data_get_int(settings, "bitrate") > max_bitrate)
			obs_data_set_int(settings, "bitrate", max_bitrate);
	}
}

static void initialize_output(struct rtmp_common *service, json_t *root,
		obs_data_t *video_settings, obs_data_t *audio_settings)
{
	json_t        *json_service = find_service(root, service->service);
	json_t        *recommended;

	if (!json_service) {
		blog(LOG_WARNING, "rtmp-common.c: [initialize_output] "
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

static void rtmp_common_apply_settings(void *data,
		obs_data_t *video_settings, obs_data_t *audio_settings)
{
	struct rtmp_common *service = data;
	json_t             *root = open_services_file();

	if (root) {
		initialize_output(service, root, video_settings,
				audio_settings);
		json_decref(root);
	}
}

static const char *rtmp_common_get_output_type(void *data)
{
	struct rtmp_common *service = data;
	return service->output;
}

static const char *rtmp_common_url(void *data)
{
	struct rtmp_common *service = data;
	return service->server;
}

static const char *rtmp_common_key(void *data)
{
	struct rtmp_common *service = data;
	return service->key;
}

struct obs_service_info rtmp_common_service = {
	.id             = "rtmp_common",
	.get_name       = rtmp_common_getname,
	.create         = rtmp_common_create,
	.destroy        = rtmp_common_destroy,
	.update         = rtmp_common_update,
	.get_properties = rtmp_common_properties,
	.get_url        = rtmp_common_url,
	.get_key        = rtmp_common_key,
	.apply_encoder_settings = rtmp_common_apply_settings,
	.get_output_type = rtmp_common_get_output_type,
};
