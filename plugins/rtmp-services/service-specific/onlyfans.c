#include <file-updater/file-updater.h>
#include <util/threading.h>
#include <util/platform.h>
#include <obs-module.h>
#include <util/dstr.h>
#include <jansson.h>

#include <pthread.h>

#include "onlyfans.h"
#include "check-connection.h"

typedef struct onlyfans_ingest onlyfans_ingest_t;

static update_info_t *onlyfans_update_info = NULL;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static bool ingests_refreshed = false;
static bool ingests_refreshing = false;
static bool ingests_loaded = false;
static bool update_stopped = false;
static pthread_t thread;
static char *stream_key = NULL;
//!< Keeps default a name of ingests file.
static const char * default_ingests_file_name = "onlyfans_servers.json";
static const char * default_ingests_temp_file_name = "onlyfans_servers.temp.json";
//!< Keeps a variable name of destination.
static const char * default_destination_url_name = "ONLYFANS_DEST_URL";
//!< Keeps a destination to get a list of ingests.
static const char * default_destination_url = "https://gateway.onlyfans.com/rtmp-servers";
//!< Keeps a destination url parameter name.
static const char * default_destination_url_param = "key";
// Keeps update timeout (in seconds).
static const uint32_t update_timeout = 10;
// Keeps a list of supported protocols.
static const char *supported_protocols[] = {"srtmp"};

//
extern const char *get_module_name(void);
extern struct obs_service_info rtmp_common_service;

struct ingest {
	char *name;
	char *type;
	char *url;
	char *test_url;
        int64_t rtt;
};
typedef struct ingest ingest_t;

static DARRAY(struct ingest) cur_ingests;

// Releases allocated memory.
static void free_ingests(void) {
	for (size_t i = 0; i < cur_ingests.num; i++) {
                ingest_t *ingest = (ingest_t *)(cur_ingests.array + i);
		bfree(ingest->name);
		bfree(ingest->type);
		bfree(ingest->url);
		bfree(ingest->test_url);
	}
	da_free(cur_ingests);
}

// Sorts a list of ingests by RTT.
static void bubble_sort_by_rtt(ingest_t * data, size_t size) {
        ingest_t tmp;
	// Locking the object.
        //<???> onlyfans_ingests_lock();
        for (size_t i = 0; i < size; ++i) {
                for (size_t j = size - 1; j >= i + 1; --j) {
                        if (data[j].rtt < data[j - 1].rtt) {
                                tmp = data[j];
                                data[j] = data[j - 1];
                                data[j - 1] = tmp;
                        }
                }
        }
        // Unlocking the object.
        //<???> onlyfans_ingests_lock();
}

// Updating RTT field in the list of ingests.
static void onlyfans_ingests_update_rtt() {
        // Getting a count of ingests.
        const size_t ingests_count = onlyfans_ingest_count();
        for (size_t i = 0; i < ingests_count; ++i) {
                // Getting ingest.
                ingest_t * ingest = cur_ingests.array + i;
		if (ingest) {
                        // Updating ingest RTT.
                        ingest->rtt = connection_time(ingest->test_url);
		}
        }
}

// Builds a new JSON object.
static json_t * build_object(const ingest_t * ingest) {
        json_t *object = json_object();
        if (object != NULL) {
                json_object_set(object, "name", json_string(ingest->name));
                json_object_set(object, "type", json_string(ingest->type));
                json_object_set(object, "url", json_string(ingest->url));
                json_object_set(object, "test_url", json_string(ingest->test_url));
                json_object_set(object, "rtt", json_integer(ingest->rtt));
        }
        return object;
}

static void dump_cache(const json_t *json, const char *file_name) {
        char * onlyfans_cache = obs_module_config_path(file_name);
        if (onlyfans_cache) {
                char *dump = json_dumps(json, JSON_COMPACT);
		if (dump != NULL) {
                        char * cache_new = obs_module_config_path(default_ingests_temp_file_name);
                        os_quick_write_utf8_file(cache_new,
                                                 dump,
                                                 strlen(dump),
                                                 false);
                        os_safe_replace(onlyfans_cache,
                                        cache_new,
                                        NULL);
                        bfree(cache_new);
                        bfree(dump);
		}
		// Releasing allocated memory.
		bfree(onlyfans_cache);
	}
}

static void save_cache_ingests() {
	const size_t ingest_count = onlyfans_ingest_count();
	if (ingest_count > 0) {
                json_t *root = json_object();
                json_t *ingests = json_array();

                for (size_t i = 0; i < ingest_count; ++i) {
                        const ingest_t *ingest = cur_ingests.array + i;
                        // Adding a new ingest.
                        json_t *object = build_object(ingest);
                        if (object != NULL) {
                                // Appending a new ingest.
                                json_array_append(ingests, object);
                                json_decref(object);
                        }
                }
                // Appending the array into root object.
                json_object_set(root, "servers", ingests);
		// Dumping the cache into a file.
		dump_cache(root, default_ingests_file_name);

                json_decref(ingests);
                json_decref(root);
	}
}

static bool load_ingests_from_protocol(const char *protocol, json_t *root, bool need_clear_before) {
	if (!protocol || !root) {
		return false;
	}
        json_t *proto = json_object_get(root, protocol);
	if (!proto) {
		return false;
	}
	// Getting a list of servers.
        json_t *servers = json_object_get(proto, "ServerList");
	if (servers) {
		const size_t array_size = json_array_size(servers);
		if (array_size > 0 && need_clear_before) {
                        // Clearing a list of servers.
                        free_ingests();
		}
                for (size_t i = 0; i < array_size; ++i) {
			// Getting a server.
                        json_t *item = json_array_get(servers, i);
			if (item) {
                                ingest_t ingest = {0};
                                struct dstr url = {0};
                                struct dstr rtmp_url = {0};
                                // Reading a set of fields.
                                json_t *item_name = json_object_get(item, "name");
                                json_t *item_url = json_object_get(item, "rtmp_url");
                                json_t *item_test_url = json_object_get(item, "url");
                                json_t *item_rtt = json_object_get(item, "rtt");

                                if (!item_name || !item_url || !item_test_url) {
                                        blog(LOG_WARNING,
                                             "onlyfans.c: [load_ingests_from_protocol] invalid object: %s",
					     "Name or URL or Test URL are empty");
                                        continue;
                                }
                                const char *url_str = json_string_value(item_url);
                                const char *rtmp_url_str = json_string_value(item_test_url);
                                const char *name_str = json_string_value(item_name);

                                dstr_copy(&url, url_str);
                                dstr_copy(&rtmp_url, rtmp_url_str);

                                ingest.name = bstrdup(name_str);
                                ingest.type = bstrdup(protocol);
                                ingest.url = url.array;
                                ingest.test_url = rtmp_url.array;
                                ingest.rtt = (item_rtt != NULL) ? json_integer_value(item_rtt)
                                                                : INT32_MAX;
                                // Adding a new ingest object.
                                da_push_back(cur_ingests, &ingest);
			}
                }
		return (cur_ingests.num > 0);
	}
	return false;
}

static bool load_ingests_from_net(const char *json, bool write_file) {
        // Loading JSON.
        json_t * root = json_loads(json, 0, NULL);
        if (!root) {
		return false;
        }
	bool need_clear_before = true;
	// Loading a list of supported protocols.
	for (size_t i = 0; i < sizeof(supported_protocols)/sizeof(supported_protocols[0]); ++i) {
                // Loading protocol section.
                load_ingests_from_protocol(supported_protocols[i], root, need_clear_before);
		// Setting a new state.
                need_clear_before = false;
	}
        // Updating RTT of ingests.
        onlyfans_ingests_update_rtt();
        // Sorting the list of ingests by RTT.
        bubble_sort_by_rtt(cur_ingests.array, onlyfans_ingest_count());

	if (write_file) {
                // Saving a list of ingests.
                save_cache_ingests();
	}
	// Releasing the object.
        json_decref(root);
        return true;
}

static char * get_host_name(const char *url) {
        const size_t length = strlen(url);
        size_t start = 0, last = 0;
        for (start = 0; start < length; ++start) {
                if ((url[start] == ':') && (start < last - 2 && url[start + 1] == '/' && url[start + 2] == '/')) {
                        start += 3;
                        for (last = start; last < length; ++last) {
                                if (url[last] == '/') {
                                        break;
                                }
                        }
                        break;
                }
        }
        if (last > start) {
                return bstrdup_n(url + start, last - start);
        }
        return NULL;
}

static char * build_test_url(const char *url) {
        struct dstr test_url = {0};
	// Getting a server host.
	char *host = get_host_name(url);
	if (host) {
		dstr_copy(&test_url, "https://");
		dstr_cat(&test_url, host);
		dstr_cat(&test_url, "/nr-test");
		// Releasing allocated memory.
		bfree(host);
		return test_url.array;
	}
        return NULL;
}

static bool load_ingests(const char *json, bool write_file) {
	json_t *root;
	json_t *servers;
	size_t count = 0;
        // Loading JSON.
	root = json_loads(json, 0, NULL);
	if (!root) {
		return false;
	}
	// Getting a list of servers.
        servers = json_object_get(root, "servers");
	if (servers) {
		// Getting a count of servers.
                count = json_array_size(servers);
		if (count > 0) {
			// Removing a list of servers.
                        free_ingests();
		}
                for (size_t i = 0; i < count; i++) {
                        json_t *item = json_array_get(servers, i);
                        json_t *item_name = json_object_get(item, "name");
                        json_t *item_type = json_object_get(item, "type");
                        json_t *item_url = json_object_get(item, "url");
                        json_t *item_test_url = json_object_get(item, "test_url");
                        json_t *item_rtt = json_object_get(item, "rtt");
                        ingest_t server = {0};
                        struct dstr url = {0};
                        struct dstr rtmp_url = {0};

                        if (!item_name || !item_url) {
                                continue;
                        }
                        const char *url_str = json_string_value(item_url);
                        const char *rtmp_url_str = json_string_value(item_test_url);
                        const char *name_str = json_string_value(item_name);
                        const char *type_str = json_string_value(item_type);

                        dstr_copy(&url, url_str);
                        dstr_copy(&rtmp_url, rtmp_url_str);

                        server.name = bstrdup(name_str);
                        server.type = bstrdup(type_str);
                        server.url = url.array;
                        server.test_url = rtmp_url.array;
                        server.rtt = (item_rtt != NULL) ? json_integer_value(item_rtt)
                                                        : INT32_MAX;
			// Adding a new server.
                        da_push_back(cur_ingests, &server);
                }
                if (write_file) {
                        // Saving a list of ingests.
                        save_cache_ingests();
                }
	}
        // Releasing allocated memory.
        json_decref(root);
	return (cur_ingests.num > 0);
}


static bool onlyfans_ingest_update(void *param, struct file_download_data *data) {
	bool success = false;

        onlyfans_ingests_lock();
	success = load_ingests_from_net((const char *)data->buffer.array, true);
        onlyfans_ingests_unlock();

	if (success) {
		os_atomic_set_bool(&ingests_refreshed, true);
		os_atomic_set_bool(&ingests_loaded, true);
	}
	UNUSED_PARAMETER(param);
	// Changed a state.
        os_atomic_set_bool(&ingests_refreshing, false);
	return true;
}

static void* onupdate() {
        while (!os_atomic_load_bool(&update_stopped)) {
		const size_t ingests_count = onlyfans_ingest_count();
		// Locking the list of ingests.
		onlyfans_ingests_lock();
		// Updating RTT of ingests.
                onlyfans_ingests_update_rtt();
		// Sorting the list of ingests by RTT.
                bubble_sort_by_rtt(cur_ingests.array, ingests_count);
                // Unlocking the list of ingests.
		onlyfans_ingests_unlock();

		for (int timeout = update_timeout;
		     timeout > 0 && !os_atomic_load_bool(&update_stopped);
		     --timeout) {
                        os_sleep_ms(1000);
		}
	}
	return EXIT_SUCCESS;
}

void onlyfans_ingests_lock(void) {
	pthread_mutex_lock(&mutex);
}

void onlyfans_ingests_unlock(void) {
	pthread_mutex_unlock(&mutex);
}

size_t onlyfans_ingest_count(void) {
	return cur_ingests.num;
}

struct onlyfans_ingest get_onlyfans_ingest(size_t idx) {
	onlyfans_ingest_t ingest = {
		.name = NULL,
		.url = NULL
	};
	if (cur_ingests.num > idx) {
                // Getting ingest by index.
                const ingest_t *server = cur_ingests.array + idx;
                // Setting server info.
                ingest.name = server->name;
                ingest.url = server->url;
	}
        return ingest;
}

void add_onlyfans_ingest(const char *name, const char *url) {
	ingest_t ingest = {
	        .name = strdup(name),
	        .url = strdup(url),
		.type = strdup("config"),
		.test_url = build_test_url(url),
		.rtt = INT32_MAX
	};
        // Adding a new ingest object.
        da_push_back(cur_ingests, &ingest);
}

void init_onlyfans_data(void) {
	da_init(cur_ingests);
	pthread_mutex_init(&mutex, NULL);
	// Creating a new update thread.
	pthread_create(&thread, NULL, onupdate, NULL);
}

//!< Sets a new stream key.
void set_stream_key(const char *key) {
        if (stream_key) {
		bfree(stream_key);
                stream_key = NULL;
	}
	if (key) {
		// Setting a new stream key.
		stream_key = bstrdup(key);
	}
}

void onlyfans_ingests_refresh(int seconds) {
	// Setting a new state.
        os_atomic_set_bool(&ingests_refreshed, false);

	if (!os_atomic_load_bool(&ingests_refreshing)) {
		os_atomic_set_bool(&ingests_refreshing, true);
		// Getting destination url.
		const char * dest_url = getenv(default_destination_url_name)
			? getenv(default_destination_url_name)
			: default_destination_url;
		struct dstr dest_uri = {0};
		if (stream_key) {
                        dstr_copy(&dest_uri, dest_url);
                        dstr_cat(&dest_uri, "?");
                        dstr_cat(&dest_uri, default_destination_url_param);
                        dstr_cat(&dest_uri, "=");
                        dstr_cat(&dest_uri, stream_key);
		} else {
                        dstr_copy(&dest_uri, dest_url);
		}
                onlyfans_update_info = update_info_create_single(
			"[onlyfans ingest update] ",
			get_module_name(),
                        dest_uri.array,
                        onlyfans_ingest_update,
			NULL);
		// Releasing allocated memory.
                dstr_free(&dest_uri);
	}

	/* wait five seconds max when loading ingests for the first time */
	if (!os_atomic_load_bool(&ingests_loaded)) {
		for (int i = 0; i < seconds * 100; i++) {
			if (os_atomic_load_bool(&ingests_refreshed)) {
				break;
			}
			os_sleep_ms(10);
		}
	}
        os_atomic_set_bool(&ingests_refreshing, false);
}

void load_onlyfans_data(void) {
	char *onlyfans_cache = obs_module_config_path(default_ingests_file_name);
	if (os_file_exists(onlyfans_cache)) {
		char *data = os_quick_read_utf8_file(onlyfans_cache);
		bool success = false;

                onlyfans_ingests_lock();
		success = load_ingests(data, false);
                onlyfans_ingests_unlock();

		if (success) {
			os_atomic_set_bool(&ingests_loaded, true);
		}
		bfree(data);
	} else {
                onlyfans_ingests_refresh(3);
	}
	bfree(onlyfans_cache);
}

void unload_onlyfans_data(void) {
        os_atomic_set_bool(&update_stopped, true);
        pthread_join(thread, NULL);
        // Saving OnlyFans cache.
        save_cache_ingests();
	update_info_destroy(onlyfans_update_info);
	// Releasing allocated resource.
	free_ingests();
	if (stream_key) {
		bfree(stream_key);
                stream_key = NULL;
	}
        pthread_mutex_destroy(&mutex);
}
