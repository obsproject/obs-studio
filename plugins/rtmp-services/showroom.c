#include <util/curl/curl-helper.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>
#include <util/dstr.h>
#include "util/base.h"
#include <obs-module.h>
#include <util/platform.h>
#include "showroom.h"
#include <util/threading.h>
struct showroom_mem_struct {
	char *memory;
	size_t size;
};
static size_t showroom_write_cb(void *contents, size_t size, size_t nmemb,
				void *userp)
{
	size_t realsize = size * nmemb;
	struct showroom_mem_struct *mem = (struct showroom_mem_struct *)userp;

	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory == NULL) {
		blog(LOG_WARNING, "showroom_write_cb: realloc returned NULL");
		return 0;
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}
showroom_ingest get_ingest_from_json(char *str)
{
	json_error_t error;
	json_t *root;
	showroom_ingest result;
	root = json_loads(str, JSON_REJECT_DUPLICATES, &error);
	if (!root) {
		return result;
	}
	result.url =
		json_string_value(json_object_get(root, "streaming_url_rtmp"));
	result.key = json_string_value(json_object_get(root, "streaming_key"));
	return result;
}
const showroom_ingest showroom_get_ingest(const char *server,
					  const char *accessKey)
{
	CURL *curl_handle;
	CURLcode res;
	struct showroom_mem_struct chunk;
	struct dstr uri;
	long response_code;
	curl_handle = curl_easy_init();

	chunk.memory = malloc(1);
	chunk.size = 0;
	dstr_init(&uri);
	dstr_copy(&uri, server);
	dstr_ncat(&uri, accessKey, strlen(accessKey));
	curl_easy_setopt(curl_handle, CURLOPT_URL, uri.array);
	curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, true);
	curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 2L);
	curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 30L);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, showroom_write_cb);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
	curl_obs_set_revoke_setting(curl_handle);

#if LIBCURL_VERSION_NUM >= 0x072400
	curl_easy_setopt(curl_handle, CURLOPT_SSL_ENABLE_ALPN, 0);
#endif

	res = curl_easy_perform(curl_handle);
	dstr_free(&uri);
	showroom_ingest ingest;
	ingest.url = "";
	ingest.key = "";
	if (res != CURLE_OK) {
		blog(LOG_WARNING,
		     "showroom_get_ingest: curl_easy_perform() failed: %s",
		     curl_easy_strerror(res));
		curl_easy_cleanup(curl_handle);
		free(chunk.memory);
		return ingest;
	}

	curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
	if (response_code != 200) {
		blog(LOG_WARNING,
		     "showroom_get_ingest: curl_easy_perform() returned code: %ld",
		     response_code);
		curl_easy_cleanup(curl_handle);
		free(chunk.memory);
		return ingest;
	}

	curl_easy_cleanup(curl_handle);

	if (chunk.size == 0) {
		blog(LOG_WARNING,
		     "showroom_get_ingest: curl_easy_perform() returned empty response");
		free(chunk.memory);
		return ingest;
	}
	char *response = strdup(chunk.memory);
	ingest = get_ingest_from_json(response);
	free(chunk.memory);
	return ingest;
}
