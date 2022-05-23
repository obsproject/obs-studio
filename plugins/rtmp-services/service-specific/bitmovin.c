#include <curl/curl.h>
#include <jansson.h>
#include <obs.h>
#include <stdlib.h>
#include <string.h>
#include <util/darray.h>
#include <util/base.h>
#include <util/dstr.h>
#include <util/platform.h>
#include <util/threading.h>

#include "bitmovin.h"
#include "bitmovin-constants.h"

#ifndef SEC_TO_NSEC
#define SEC_TO_NSEC 1000000000ULL
#endif

static char *bitmovin_base_url = "https://api.bitmovin.com/v1/encoding";
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static u_int cache_timeout_in_seconds = 10;

struct bitmovin_ingest_info {
	struct dstr encoding_id;
	struct dstr ingest_url;
	struct dstr stream_key;
	struct dstr encoding_name;
};

uint64_t last_time;

static DARRAY(struct bitmovin_ingest_info) running_live_encodings;

void free_live_encodings(void)
{
	last_time = 0;
	for (size_t i = 0; i < running_live_encodings.num; i++) {
		dstr_free(&running_live_encodings.array[i].encoding_id);
		dstr_free(&running_live_encodings.array[i].ingest_url);
		dstr_free(&running_live_encodings.array[i].stream_key);
		dstr_free(&running_live_encodings.array[i].encoding_name);
	}

	da_free(running_live_encodings);
}

void reset_live_encodings(void)
{
	free_live_encodings();
	da_init(running_live_encodings);
}

static struct bitmovin_ingest_info *find_live_encoding(const char *encoding_id)
{
	struct bitmovin_ingest_info *ret = NULL;
	for (size_t i = 0; i < running_live_encodings.num; i++) {
		struct bitmovin_ingest_info *info =
			&running_live_encodings.array[i];
		if (strcmp(info->encoding_id.array, encoding_id) == 0) {
			ret = info;
			break;
		}
	}

	return ret;
}

static size_t write_cb(void *data, size_t size, size_t nmemb, void *ptr)
{
	struct dstr *json = ptr;
	size_t realsize = size * nmemb;
	dstr_ncat(json, data, realsize);
	return realsize;
}

static json_t *bitmovin_json_get_request(char *request_url,
					 struct curl_slist *header)
{
	CURL *curl;
	CURLcode res;

	curl = curl_easy_init();
	if (!curl) {
		return NULL;
	}
	struct dstr response = {0};

	header = curl_slist_append(header, "accept: application/json");
	header = curl_slist_append(header, "User-Agent: obs-studio");

	curl_easy_setopt(curl, CURLOPT_URL, request_url);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

	res = curl_easy_perform(curl);

	json_error_t error;
	json_t *json_response = json_loads(response.array, 0, &error);

	if (res != CURLE_OK) {
		blog(LOG_WARNING, "bitmovin_json_get_request: %s\n",
		     curl_easy_strerror(res));
		curl_easy_cleanup(curl);
		dstr_free(&response);
		return NULL;
	}
	long response_code;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
	if (response_code != 200) {
		blog(LOG_WARNING, "bitmovin_json_get_request: status code: %ld",
		     response_code);
		curl_easy_cleanup(curl);
		dstr_free(&response);
		return NULL;
	}

	curl_easy_cleanup(curl);
	dstr_free(&response);
	blog(LOG_DEBUG, "bitmovin_json_get_request: %s\n",
	     json_dumps(json_response, 0));
	return json_response;
}

void bitmovin_get_stream_details(const char *key,
				 struct bitmovin_ingest_info *info)
{
	struct curl_slist *header = NULL;
	struct dstr api_header = {0};
	const char *key_label = "X-Api-Key: ";
	dstr_copy(&api_header, key_label);
	dstr_cat(&api_header, key);

	header = curl_slist_append(header, api_header.array);
	struct dstr url = {0};
	const char *encodings_url = "/encodings/";
	dstr_copy(&url, bitmovin_base_url);
	dstr_cat(&url, encodings_url);

	dstr_cat(&url, info->encoding_id.array);

	const char *live_url = "/live";
	dstr_cat(&url, live_url);

	struct json_t *response = bitmovin_json_get_request(url.array, header);

	curl_slist_free_all(header);
	dstr_free(&api_header);
	dstr_free(&url);
	if (response == NULL) {
		return;
	}
	const char *status =
		json_string_value(json_object_get(response, "status"));
	if (strcmp(status, "SUCCESS") != 0) {
		blog(LOG_INFO, "bitmovin_get_stream_details: STATUS: %s",
		     status);
		return;
	}

	struct json_t *data = json_object_get(response, "data");
	struct json_t *result = json_object_get(data, "result");
	const char *encoderIp =
		json_string_value(json_object_get(result, "encoderIp"));
	const char *application =
		json_string_value(json_object_get(result, "application"));
	const char *stream_key =
		json_string_value(json_object_get(result, "streamKey"));

	dstr_copy(&(info->ingest_url), "rtmp://");
	dstr_cat(&(info->ingest_url), encoderIp);
	dstr_cat(&(info->ingest_url), "/");
	dstr_cat(&(info->ingest_url), application);
	dstr_copy(&(info->stream_key), stream_key);
}

void bitmovin_retrieve_running_live_streams(const char *key)
{
	if (key == NULL) {
		return;
	}
	if (running_live_encodings.num > 0) {
		// just like the showroom plugin did it:
		// this function is called a bunch of times for the same data,
		// so in order to prevent multiple unnecessary queries in a
		// short period of time, return the same data for 10 seconds
		uint64_t ts_sec = os_gettime_ns() / SEC_TO_NSEC;
		if (ts_sec - last_time < cache_timeout_in_seconds) {
			return;
		}
	}

	struct curl_slist *header = NULL;
	struct dstr api_header = {0};
	const char *key_label = "X-Api-Key: ";
	dstr_copy(&api_header, key_label);
	dstr_cat(&api_header, key);

	header = curl_slist_append(header, api_header.array);
	struct dstr url = {0};
	const char *encodings_url_query =
		"/encodings?sort=createdAt%3Adesc&type=LIVE&status=RUNNING";
	dstr_copy(&url, bitmovin_base_url);
	dstr_cat(&url, encodings_url_query);

	struct json_t *response = bitmovin_json_get_request(url.array, header);

	curl_slist_free_all(header);
	dstr_free(&api_header);
	dstr_free(&url);
	if (response == NULL) {
		return;
	}
	const char *status =
		json_string_value(json_object_get(response, "status"));
	if (strcmp(status, "SUCCESS") != 0) {
		blog(LOG_WARNING,
		     "bitmovin_retrieve_running_live_streams: STATUS: %s",
		     status);
		return;
	}
	struct json_t *data = json_object_get(response, "data");
	struct json_t *result = json_object_get(data, "result");
	struct json_t *items = json_object_get(result, "items");

	reset_live_encodings();

	size_t index;
	json_t *value;
	json_array_foreach (items, index, value) {
		const char *encoding_id =
			json_string_value(json_object_get(value, "id"));
		const char *encoding_name =
			json_string_value(json_object_get(value, "name"));
		if (encoding_id != NULL) {
			blog(LOG_DEBUG,
			     "bitmovin_retrieve_running_live_streams: encoding_id=%s",
			     encoding_id);
			struct bitmovin_ingest_info *info =
				find_live_encoding(encoding_id);
			if (!info) {
				info = da_push_back_new(running_live_encodings);
			}
			dstr_copy(&(info->encoding_id), encoding_id);
			if (encoding_name != NULL) {
				dstr_copy(&(info->encoding_name),
					  encoding_name);
			}

			bitmovin_get_stream_details(key, info);
			blog(LOG_DEBUG,
			     "bitmovin_retrieve_running_live_streams: ingest_url=%s streamkey=%s",
			     info->ingest_url.array, info->stream_key.array);
		}
	}
	last_time = os_gettime_ns() / SEC_TO_NSEC;
}

void init_bitmovin_data(void)
{
	pthread_mutex_lock(&mutex);
	da_init(running_live_encodings);
	pthread_mutex_unlock(&mutex);
}

void unload_bitmovin_data(void)
{
	pthread_mutex_lock(&mutex);
	free_live_encodings();
	pthread_mutex_unlock(&mutex);
}

void bitmovin_update(const char *key)
{
	if (strlen(key) != 36) {
		return;
	}
	pthread_mutex_lock(&mutex);
	bitmovin_retrieve_running_live_streams(key);
	pthread_mutex_unlock(&mutex);
}

const char *bitmovin_get_ingest(const char *key, const char *encoding_id)
{
	pthread_mutex_lock(&mutex);
	bitmovin_retrieve_running_live_streams(key);
	if (running_live_encodings.num <= 0) {
		pthread_mutex_unlock(&mutex);
		return NULL;
	}
	struct bitmovin_ingest_info *info = find_live_encoding(encoding_id);
	char *url = NULL;
	if (info != NULL) {
		url = info->ingest_url.array;
	}
	pthread_mutex_unlock(&mutex);
	blog(LOG_DEBUG, "bitmovin_get_ingest: %s", url);
	return url;
}

const char *bitmovin_get_stream_key(void)
{
	char *result = NULL;
	pthread_mutex_lock(&mutex);
	if (running_live_encodings.num > 0) {
		struct bitmovin_ingest_info *info =
			&running_live_encodings.array[0];
		result = info->stream_key.array;
	}
	pthread_mutex_unlock(&mutex);
	blog(LOG_INFO, "bitmovin_get_stream_key: %s", result);
	return result;
}

void bitmovin_get_obs_properties(obs_properties_t *props)
{
	obs_property_t *list = obs_properties_add_list(
		props, BITMOVIN_RUNNING_LIVE_STREAMS_LIST_PROPERTY_NAME,
		"Running live streams", OBS_COMBO_TYPE_LIST,
		OBS_COMBO_FORMAT_STRING);

	for (size_t i = 0; i < running_live_encodings.num; i++) {
		struct bitmovin_ingest_info *info =
			&running_live_encodings.array[i];
		struct dstr name = {0};
		dstr_copy(&name, info->encoding_name.array);
		dstr_cat(&name, " (");
		dstr_cat(&name, info->encoding_id.array);
		dstr_cat(&name, ")");
		obs_property_list_insert_string(list, i, name.array,
						info->encoding_id.array);
		dstr_free(&name);
	}
}
