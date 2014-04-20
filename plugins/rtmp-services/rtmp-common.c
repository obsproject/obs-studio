#include <util/platform.h>
#include <obs-module.h>
#include <jansson.h>

struct rtmp_common {
	char *service;
	char *server;
	char *key;
};

static const char *rtmp_common_getname(const char *locale)
{
	UNUSED_PARAMETER(locale);

	/* TODO: locale */
	return "Other Streaming Services";
}

static void rtmp_common_update(void *data, obs_data_t settings)
{
	struct rtmp_common *service = data;

	bfree(service->service);
	bfree(service->server);
	bfree(service->key);

	service->service = bstrdup(obs_data_getstring(settings, "service"));
	service->server  = bstrdup(obs_data_getstring(settings, "server"));
	service->key     = bstrdup(obs_data_getstring(settings, "key"));
}

static void rtmp_common_destroy(void *data)
{
	struct rtmp_common *service = data;

	bfree(service->service);
	bfree(service->server);
	bfree(service->key);
	bfree(service);
}

static void *rtmp_common_create(obs_data_t settings, obs_service_t service)
{
	struct rtmp_common *data = bzalloc(sizeof(struct rtmp_common));
	rtmp_common_update(data, settings);

	UNUSED_PARAMETER(service);
	return data;
}

static void add_service(obs_property_t list, const char *name,
		json_t *service)
{
	json_t *servers;

	if (!json_is_object(service)) {
		blog(LOG_WARNING, "rtmp-common.c: [add_service] service "
		                  "'%s' is not an object", name);
		return;
	}

	servers = json_object_get(service, "servers");
	if (!servers) {
		blog(LOG_WARNING, "rtmp-common.c: [add_service] service "
		                  "'%s' has no servers", name);
		return;
	}

	obs_property_list_add_string(list, name, name);
}

static void add_services(obs_property_t list, const char *file, json_t *root)
{
	json_t *service;
	const char *name;

	if (!json_is_object(root)) {
		blog(LOG_WARNING, "rtmp-common.c: [add_services] JSON file "
		                  "'%s' root is not an object", file);
		return;
	}

	json_object_foreach (root, name, service) {
		add_service(list, name, service);
	}
}

static json_t *build_service_list(obs_property_t list, const char *file)
{
	char         *file_data = os_quick_read_utf8_file(file);
	json_error_t error;
	json_t       *root;

	if (!file_data)
		return NULL;

	root = json_loads(file_data, JSON_REJECT_DUPLICATES, &error);
	bfree(file_data);

	if (!root) {
		blog(LOG_WARNING, "rtmp-common.c: [build_service_list] "
		                  "Error reading JSON file '%s' (%d): %s",
		                  file, error.line, error.text);
		return NULL;
	}

	add_services(list, file, root);
	return root;
}

static void properties_data_destroy(void *data)
{
	json_t *root = data;
	if (root)
		json_decref(root);
}

static void fill_servers(obs_property_t servers, json_t *service,
		const char *name)
{
	json_t     *server;
	const char *server_name;

	obs_property_list_clear(servers);

	if (!json_is_object(service)) {
		blog(LOG_WARNING, "rtmp-common.c: [fill_servers] "
		                  "Servers for service '%s' not a valid object",
		                  name);
		return;
	}

	json_object_foreach (service, server_name, server) {
		if (json_is_string(server)) {
			obs_property_list_add_string(servers, server_name,
					json_string_value(server));
		}
	}
}

static bool service_selected(obs_properties_t props, obs_property_t p,
		obs_data_t settings)
{
	const char *name = obs_data_getstring(settings, "service");
	json_t *root     = obs_properties_get_param(props);
	json_t *service  = json_object_get(root, name);

	fill_servers(obs_properties_get(props, "server"), service, name);

	UNUSED_PARAMETER(p);
	return true;
}

static obs_properties_t rtmp_common_properties(const char *locale)
{
	obs_properties_t ppts = obs_properties_create(locale);
	obs_property_t   list;
	char             *file;

	/* TODO: locale */

	list = obs_properties_add_list(ppts, "service", "Service",
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	file = obs_find_plugin_file("rtmp-services/services.json");
	if (file) {
		json_t *root = build_service_list(list, file);
		obs_properties_set_param(ppts, root, properties_data_destroy);
		obs_property_set_modified_callback(list, service_selected);
		bfree(file);
	}

	obs_properties_add_list(ppts, "server", "Server",
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	obs_properties_add_text(ppts, "key", "Stream Key", OBS_TEXT_PASSWORD);
	return ppts;
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
	.id         = "rtmp_common",
	.getname    = rtmp_common_getname,
	.create     = rtmp_common_create,
	.destroy    = rtmp_common_destroy,
	.update     = rtmp_common_update,
	.properties = rtmp_common_properties,
	.get_url    = rtmp_common_url,
	.get_key    = rtmp_common_key
};
