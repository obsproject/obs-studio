#pragma once

/* service-ingest.h is common between the Amazon IVS service
 * and the Twitch streaming service.
 */
#include <file-updater/file-updater.h>
#include <util/threading.h>

struct ingest {
	char *name;
	char *url;
	char *rtmps_url;
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

void init_service_data(struct service_ingests *si);
void service_ingests_refresh(struct service_ingests *si, int seconds,
			     const char *log_prefix, const char *file_url);
void load_service_data(struct service_ingests *si, const char *cache_filename,
		       struct ingest *def);
void unload_service_data(struct service_ingests *si);
struct ingest get_ingest(struct service_ingests *si, size_t idx);
