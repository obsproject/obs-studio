#include <file-updater/file-updater.h>
#include <util/threading.h>
#include <util/platform.h>
#include <obs-module.h>
#include <util/dstr.h>
#include <jansson.h>

#include "twitch.h"

static update_info_t *twitch_update_info = NULL;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct ingest {
	char *name;
	char *url;
};

static DARRAY(struct ingest) cur_ingests;

static void free_ingests(void)
{
	for (size_t i = 0; i < cur_ingests.num; i++) {
		struct ingest *ingest = cur_ingests.array + i;
		bfree(ingest->name);
		bfree(ingest->url);
	}

	da_free(cur_ingests);
}

static void load_ingests(const char *json, bool write_file)
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
	if (count <= 1 && cur_ingests.num)
		goto finish;

	free_ingests();

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

		da_push_back(cur_ingests, &ingest);
	}

	if (!write_file || !cur_ingests.num)
		goto finish;

	cache_old = obs_module_config_path("twitch_ingests.json");
	cache_new = obs_module_config_path("twitch_ingests.new.json");

	os_quick_write_utf8_file(cache_new, json, strlen(json), false);
	os_safe_replace(cache_old, cache_new, NULL);

	bfree(cache_old);
	bfree(cache_new);

finish:
	if (root)
		json_decref(root);
}

static bool twitch_ingest_update(void *param, struct file_download_data *data)
{
	pthread_mutex_lock(&mutex);
	load_ingests(data->buffer.array, true);
	pthread_mutex_unlock(&mutex);

	UNUSED_PARAMETER(param);
	return true;
}

void twitch_ingests_lock(void)
{
	pthread_mutex_lock(&mutex);
}

void twitch_ingests_unlock(void)
{
	pthread_mutex_unlock(&mutex);
}

size_t twitch_ingest_count(void)
{
	return cur_ingests.num;
}

struct twitch_ingest twitch_ingest(size_t idx)
{
	struct twitch_ingest ingest;

	if (cur_ingests.num <= idx) {
		ingest.name = NULL;
		ingest.url = NULL;
	} else {
		ingest = *(struct twitch_ingest*)(cur_ingests.array + idx);
	}

	return ingest;
}

void init_twitch_data(void)
{
	da_init(cur_ingests);
	pthread_mutex_init(&mutex, NULL);
}

void load_twitch_data(const char *module_str)
{
	char *twitch_cache = obs_module_config_path("twitch_ingests.json");

	if (os_file_exists(twitch_cache)) {
		char *data = os_quick_read_utf8_file(twitch_cache);

		pthread_mutex_lock(&mutex);
		load_ingests(data, false);
		pthread_mutex_unlock(&mutex);

		bfree(data);
	}

	twitch_update_info = update_info_create_single(
			"[twitch ingest update] ",
			module_str,
			"https://ingest.twitch.tv/api/v2/ingests",
			twitch_ingest_update, NULL);

	bfree(twitch_cache);
}

void unload_twitch_data(void)
{
	update_info_destroy(twitch_update_info);
	free_ingests();
	pthread_mutex_destroy(&mutex);
}
