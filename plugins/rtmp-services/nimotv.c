#include <util/threading.h>
#include <util/platform.h>
#include <obs-module.h>
#include <util/dstr.h>
#include <util/darray.h>
#include <jansson.h>
#include <curl/curl.h>
#include <time.h>
#include "nimotv.h"

typedef bool (*confirm_callback_t)(struct darray *da);

struct request_info {
	char error[CURL_ERROR_SIZE];
	DARRAY(uint8_t) body_data;
	char *user_agent;
	CURL *curl;
	char *url;
	confirm_callback_t callback;
	pthread_t thread;
	bool thread_created;
};

typedef struct request_info request_info_t;

static size_t http_write(uint8_t *ptr, size_t size, size_t nmemb,
			 struct request_info *info)
{
	size_t total = size * nmemb;
	if (total)
		da_push_back_array(info->body_data, ptr, total);

	return total;
}

static bool do_http_request(struct request_info *info, const char *url,
			    long *response_code)
{
	CURLcode code;
	uint8_t null_terminator = 0;

	da_resize(info->body_data, 0);
	curl_easy_setopt(info->curl, CURLOPT_URL, url);
	curl_easy_setopt(info->curl, CURLOPT_ERRORBUFFER, info->error);
	curl_easy_setopt(info->curl, CURLOPT_WRITEFUNCTION, http_write);
	curl_easy_setopt(info->curl, CURLOPT_WRITEDATA, info);
	curl_easy_setopt(info->curl, CURLOPT_FAILONERROR, true);
	curl_easy_setopt(info->curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(info->curl, CURLOPT_ACCEPT_ENCODING, "");

#if LIBCURL_VERSION_NUM >= 0x072400
	// A lot of servers don't yet support ALPN
	curl_easy_setopt(info->curl, CURLOPT_SSL_ENABLE_ALPN, 0);
#endif

	code = curl_easy_perform(info->curl);
	if (code != CURLE_OK) {
		blog(LOG_WARNING, "nimotv get ingest failed: %s", info->error);
		return false;
	}

	if (curl_easy_getinfo(info->curl, CURLINFO_RESPONSE_CODE,
			      response_code) != CURLE_OK)
		return false;

	if (*response_code >= 400) {
		blog(LOG_WARNING, "nimotv get ingest failed: HTTP/%ld",
		     *response_code);
		return false;
	}

	da_push_back(info->body_data, &null_terminator);

	return true;
}

static void *single_request_thread(void *data)
{
	struct request_info *info = data;
	long response_code;

	info->curl = curl_easy_init();
	if (!info->curl) {
		blog(LOG_WARNING,
		     "nimotv get ingest: could not initialize Curl");
		return NULL;
	}

	if (!do_http_request(info, info->url, &response_code))
		return NULL;
	if (!info->body_data.array || !info->body_data.array[0])
		return NULL;
	info->callback(&(info->body_data.da));
	return NULL;
}

static request_info_t *
request_info_create_single(const char *user_agent, const char *url,
			   confirm_callback_t confirm_callback)
{
	struct request_info *info;

	info = bzalloc(sizeof(*info));
	info->user_agent = bstrdup(user_agent);
	info->url = bstrdup(url);
	info->callback = confirm_callback;

	if (pthread_create(&info->thread, NULL, single_request_thread, info) ==
	    0)
		info->thread_created = true;

	return info;
}

static void request_info_destroy(struct request_info *info)
{
	if (!info)
		return;

	if (info->thread_created)
		pthread_join(info->thread, NULL);

	da_free(info->body_data);
	bfree(info->user_agent);
	bfree(info->url);
	if (info->curl)
		curl_easy_cleanup(info->curl);

	bfree(info);
}

static request_info_t *nimotv_request_info = NULL;
static pthread_mutex_t ingest_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t time_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool ingests_refreshed = false;
static bool ingests_refreshing = false;
static time_t last_time = -1;

struct ingest {
	char *name;
	char *url;
};

static DARRAY(struct ingest) cur_ingests;
static char *recommended_ingest = NULL;

static void free_ingests(void)
{
	for (size_t i = 0; i < cur_ingests.num; i++) {
		struct ingest *ingest = cur_ingests.array + i;
		bfree(ingest->name);
		bfree(ingest->url);
	}

	da_free(cur_ingests);
}

static void free_recommended_ingest(void)
{
	if (recommended_ingest) {
		bfree(recommended_ingest);
	}
}

static void update_recommended(json_t *recommended_ingests)
{
	json_t *recommended = json_array_get(recommended_ingests, 0);
	if (recommended) {
		json_t *item_url = json_object_get(recommended, "url");
		if (item_url) {
			free_recommended_ingest();
			const char *url = json_string_value(item_url);
			recommended_ingest = bstrdup(url);
		}
	}
}

static void load_ingests(const char *json, bool refresh)
{
	json_t *root;
	json_t *default_ingests;
	bool success = false;
	char *cache_old;
	char *cache_new;
	size_t count;

	root = json_loads(json, 0, NULL);
	if (!root)
		goto finish;

	if (refresh) {
		json_t *recommended_ingests = json_object_get(root, "ingests");
		if (recommended_ingests) {
			update_recommended(recommended_ingests);
		}
	}

	default_ingests = json_object_get(root, "default");
	if (!default_ingests)
		goto finish;

	count = json_array_size(default_ingests);
	if (count < 1 && cur_ingests.num)
		goto finish;

	free_ingests();

	for (size_t i = 0; i < count; i++) {
		json_t *item = json_array_get(default_ingests, i);
		json_t *item_name = json_object_get(item, "name");
		json_t *item_url = json_object_get(item, "url");
		struct ingest ingest = {0};

		if (!item_name || !item_url)
			continue;

		const char *url_str = json_string_value(item_url);
		const char *name_str = json_string_value(item_name);

		ingest.name = bstrdup(name_str);
		ingest.url = bstrdup(url_str);

		da_push_back(cur_ingests, &ingest);
	}

	if (!cur_ingests.num)
		goto finish;

	if (!refresh)
		goto finish;

	cache_old = obs_module_config_path("nimotv_ingests.json");
	cache_new = obs_module_config_path("nimotv_ingests.new.json");

	os_quick_write_utf8_file(cache_new, json, strlen(json), false);
	os_safe_replace(cache_old, cache_new, NULL);

	bfree(cache_old);
	bfree(cache_new);

finish:
	if (root)
		json_decref(root);
}

static bool nimotv_ingest_update(struct darray *data)
{
	DARRAY(uint8_t) buffer;
	buffer.da = *data;

	pthread_mutex_lock(&time_mutex);
	last_time = time(NULL);
	pthread_mutex_unlock(&time_mutex);

	pthread_mutex_lock(&ingest_mutex);
	load_ingests((const char *)buffer.array, true);
	pthread_mutex_unlock(&ingest_mutex);

	os_atomic_set_bool(&ingests_refreshed, true);
	os_atomic_set_bool(&ingests_refreshing, false);

	return true;
}

void nimotv_ingests_lock(void)
{
	pthread_mutex_lock(&ingest_mutex);
}

void nimotv_ingests_unlock(void)
{
	pthread_mutex_unlock(&ingest_mutex);
}

size_t nimotv_ingest_count(void)
{
	return cur_ingests.num;
}

struct nimotv_ingest nimotv_ingest(size_t idx)
{
	struct nimotv_ingest ingest;

	if (cur_ingests.num <= idx) {
		ingest.name = NULL;
		ingest.url = NULL;
	} else {
		ingest = *(struct nimotv_ingest *)(cur_ingests.array + idx);
	}

	return ingest;
}

const char *get_recommended_ingest()
{
	if (recommended_ingest) {
		return recommended_ingest;
	}
	struct nimotv_ingest ingest = nimotv_ingest(0);
	return ingest.url;
}

void init_nimotv_data(void)
{
	da_init(cur_ingests);
	pthread_mutex_init(&ingest_mutex, NULL);
	pthread_mutex_init(&time_mutex, NULL);
}

extern const char *get_module_name(void);

void nimotv_ingests_refresh(int seconds, const char *key, bool is_auto)
{
	if (os_atomic_load_bool(&ingests_refreshed)) {
		if (!is_auto)
			return;

		time_t now = time(NULL);
		pthread_mutex_lock(&time_mutex);
		double diff = difftime(now, last_time);
		pthread_mutex_unlock(&time_mutex);

		if (diff > 2) {
			os_atomic_set_bool(&ingests_refreshed, false);
		} else {
			return;
		}
	}

	if (!os_atomic_load_bool(&ingests_refreshing)) {
		os_atomic_set_bool(&ingests_refreshing, true);
		char *encoded_key = curl_easy_escape(NULL, key, 0);
		struct dstr uri;
		dstr_init(&uri);
		dstr_copy(&uri,
			  "https://globalcdnweb.nimo.tv/api/ingests/nimo?id=");
		dstr_ncat(&uri, encoded_key, strlen(encoded_key));
		curl_free(encoded_key);

		if (nimotv_request_info) {
			request_info_destroy(nimotv_request_info);
		}

		nimotv_request_info = request_info_create_single(
			get_module_name(), uri.array, nimotv_ingest_update);

		dstr_free(&uri);
	}

	if (!is_auto)
		return;

	for (int i = 0; i < seconds * 100; i++) {
		if (os_atomic_load_bool(&ingests_refreshed)) {
			break;
		}
		os_sleep_ms(10);
	}
}

void load_nimotv_data(void)
{
	char *nimotv_cache = obs_module_config_path("nimotv_ingests.json");

	if (os_file_exists(nimotv_cache)) {
		char *data = os_quick_read_utf8_file(nimotv_cache);

		pthread_mutex_lock(&ingest_mutex);
		load_ingests(data, false);
		pthread_mutex_unlock(&ingest_mutex);

		bfree(data);
	}

	bfree(nimotv_cache);
}

void unload_nimotv_data(void)
{
	request_info_destroy(nimotv_request_info);
	free_recommended_ingest();
	free_ingests();
	pthread_mutex_destroy(&ingest_mutex);
	pthread_mutex_destroy(&time_mutex);
}
