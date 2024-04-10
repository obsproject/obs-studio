#include <file-updater/file-updater.h>
#include <util/threading.h>
#include <util/platform.h>
#include <obs-module.h>
#include <util/dstr.h>
#include <jansson.h>

#include "twitch.h"

struct ingest {
	char *name;
	char *url;
};

struct service_ingests {
	update_info_t *update_info;
	pthread_mutex_t mutex;
	bool ingests_refreshed;
	bool ingests_refreshing;
	bool ingests_loaded;
	DARRAY(struct ingest) cur_ingests;
	const char *cache_old_filename;
	const char *cache_new_filename;
};

#define INGESTS_INITIALIZER(cache_old_filename_, cache_new_filename_)    \
	{                                                                \
		.update_info = NULL, .mutex = PTHREAD_MUTEX_INITIALIZER, \
		.ingests_refreshed = false, .ingests_refreshing = false, \
		.ingests_loaded = false, .cur_ingests = {0},             \
		.cache_old_filename = cache_old_filename_,               \
		.cache_new_filename = cache_new_filename_                \
	}

static struct service_ingests twitch =
	INGESTS_INITIALIZER("twitch_ingests.json", "twitch_ingests.new.json");

#undef INGESTS_INITIALIZER

static void free_ingests(struct service_ingests *si)
{
	for (size_t i = 0; i < si->cur_ingests.num; i++) {
		struct ingest *ingest = si->cur_ingests.array + i;
		bfree(ingest->name);
		bfree(ingest->url);
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
		struct ingest ingest = {0};
		struct dstr url = {0};

		if (!item_name || !item_url)
			continue;

		const char *url_str = json_string_value(item_url);
		const char *name_str = json_string_value(item_name);

		/* At the moment they currently mis-spell "deprecated",
		 * but that may change in the future, so blacklist both */
		if (strstr(name_str, "deprecated") != NULL ||
		    strstr(name_str, "depracated") != NULL)
			continue;

		dstr_copy(&url, url_str);
		dstr_replace(&url, "/{stream_key}", "");

		ingest.name = bstrdup(name_str);
		ingest.url = url.array;

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

void twitch_ingests_lock(void)
{
	pthread_mutex_lock(&twitch.mutex);
}

void twitch_ingests_unlock(void)
{
	pthread_mutex_unlock(&twitch.mutex);
}

size_t twitch_ingest_count(void)
{
	return twitch.cur_ingests.num;
}

struct twitch_ingest get_ingest(struct service_ingests *si, size_t idx)
{
	struct twitch_ingest ingest;

	if (si->cur_ingests.num <= idx) {
		ingest.name = NULL;
		ingest.url = NULL;
	} else {
		ingest = *(struct twitch_ingest *)(si->cur_ingests.array + idx);
	}

	return ingest;
}

struct twitch_ingest twitch_ingest(size_t idx)
{
	return get_ingest(&twitch, idx);
}

void init_service_data(struct service_ingests *si)
{
	da_init(si->cur_ingests);
	pthread_mutex_init(&si->mutex, NULL);
}

void init_twitch_data(void)
{
	init_service_data(&twitch);
}

extern const char *get_module_name(void);

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

void twitch_ingests_refresh(int seconds)
{
	service_ingests_refresh(&twitch, seconds, "[twitch ingest update] ",
				"https://ingest.twitch.tv/ingests");
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

void load_twitch_data(void)
{
	struct ingest def = {.name = bstrdup("Default"),
			     .url = bstrdup("rtmp://live.twitch.tv/app")};
	load_service_data(&twitch, "twitch_ingests.json", &def);
}

void unload_service_data(struct service_ingests *si)
{
	update_info_destroy(si->update_info);
	free_ingests(si);
	pthread_mutex_destroy(&si->mutex);
}

void unload_twitch_data(void)
{
	unload_service_data(&twitch);
}
