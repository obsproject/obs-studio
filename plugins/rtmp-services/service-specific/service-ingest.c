#include <util/platform.h>
#include <obs-module.h>
#include <util/dstr.h>
#include <jansson.h>
#include "service-ingest.h"

extern const char *get_module_name(void);

void init_service_data(struct service_ingests *si)
{
	da_init(si->cur_ingests);
	pthread_mutex_init(&si->mutex, NULL);
}

static void free_ingests(struct service_ingests *si)
{
	for (size_t i = 0; i < si->cur_ingests.num; i++) {
		struct ingest *ingest = si->cur_ingests.array + i;
		bfree(ingest->name);
		bfree(ingest->url);
		bfree(ingest->rtmps_url);
	}

	da_free(si->cur_ingests);
}

static bool load_ingests(struct service_ingests *si, const char *json,
			 bool write_file)
{
	json_t *root;
	json_t *ingests;
	bool success = false;
	char *cache_old;
	char *cache_new;
	size_t count;

	root = json_loads(json, 0, NULL);
	if (!root)
		goto finish;

	ingests = json_object_get(root, "ingests");
	if (!ingests)
		goto finish;

	count = json_array_size(ingests);
	if (count <= 1 && si->cur_ingests.num)
		goto finish;

	free_ingests(si);

	for (size_t i = 0; i < count; i++) {
		json_t *item = json_array_get(ingests, i);
		json_t *item_name = json_object_get(item, "name");
		json_t *item_url = json_object_get(item, "url_template");
		json_t *item_rtmps_url =
			json_object_get(item, "url_template_secure");
		struct ingest ingest = {0};
		struct dstr url = {0};
		struct dstr rtmps_url = {0};

		if (!item_name || !item_url)
			continue;

		const char *url_str = json_string_value(item_url);
		const char *rtmps_url_str = json_string_value(item_rtmps_url);
		const char *name_str = json_string_value(item_name);

		/* At the moment they currently mis-spell "deprecated",
		 * but that may change in the future, so blacklist both */
		if (strstr(name_str, "deprecated") != NULL ||
		    strstr(name_str, "depracated") != NULL)
			continue;

		dstr_copy(&url, url_str);
		dstr_replace(&url, "/{stream_key}", "");

		dstr_copy(&rtmps_url, rtmps_url_str);
		dstr_replace(&rtmps_url, "/{stream_key}", "");

		ingest.name = bstrdup(name_str);
		ingest.url = url.array;
		ingest.rtmps_url = rtmps_url.array;

		da_push_back(si->cur_ingests, &ingest);
	}

	if (!si->cur_ingests.num)
		goto finish;

	success = true;

	if (!write_file)
		goto finish;

	cache_old = obs_module_config_path(si->cache_old_filename);
	cache_new = obs_module_config_path(si->cache_new_filename);

	os_quick_write_utf8_file(cache_new, json, strlen(json), false);
	os_safe_replace(cache_old, cache_new, NULL);

	bfree(cache_old);
	bfree(cache_new);

finish:
	if (root)
		json_decref(root);
	return success;
}

static bool ingest_update(void *param, struct file_download_data *data)
{
	struct service_ingests *service = param;
	bool success;

	pthread_mutex_lock(&service->mutex);
	success = load_ingests(service, (const char *)data->buffer.array, true);
	pthread_mutex_unlock(&service->mutex);

	if (success) {
		os_atomic_set_bool(&service->ingests_refreshed, true);
		os_atomic_set_bool(&service->ingests_loaded, true);
	}

	return true;
}

void service_ingests_refresh(struct service_ingests *si, int seconds,
			     const char *log_prefix, const char *file_url)
{
	if (os_atomic_load_bool(&si->ingests_refreshed))
		return;

	if (!os_atomic_load_bool(&si->ingests_refreshing)) {
		os_atomic_set_bool(&si->ingests_refreshing, true);

		si->update_info =
			update_info_create_single(log_prefix, get_module_name(),
						  file_url, ingest_update, si);
	}

	/* wait five seconds max when loading ingests for the first time */
	if (!os_atomic_load_bool(&si->ingests_loaded)) {
		for (int i = 0; i < seconds * 100; i++) {
			if (os_atomic_load_bool(&si->ingests_refreshed)) {
				break;
			}
			os_sleep_ms(10);
		}
	}
}

void load_service_data(struct service_ingests *si, const char *cache_filename,
		       struct ingest *def)
{
	char *service_cache = obs_module_config_path(cache_filename);

	pthread_mutex_lock(&si->mutex);
	da_push_back(si->cur_ingests, def);
	pthread_mutex_unlock(&si->mutex);

	if (os_file_exists(service_cache)) {
		char *data = os_quick_read_utf8_file(service_cache);
		bool success;

		pthread_mutex_lock(&si->mutex);
		success = load_ingests(si, data, false);
		pthread_mutex_unlock(&si->mutex);

		if (success) {
			os_atomic_set_bool(&si->ingests_loaded, true);
		}

		bfree(data);
	}

	bfree(service_cache);
}

void unload_service_data(struct service_ingests *si)
{
	update_info_destroy(si->update_info);
	free_ingests(si);
	pthread_mutex_destroy(&si->mutex);
}

struct ingest get_ingest(struct service_ingests *si, size_t idx)
{
	struct ingest ingest;

	if (si->cur_ingests.num <= idx) {
		ingest.name = NULL;
		ingest.url = NULL;
		ingest.rtmps_url = NULL;
	} else {
		ingest = *(struct ingest *)(si->cur_ingests.array + idx);
	}

	return ingest;
}
