#include "service-ingest.h"
#include "twitch.h"

static struct service_ingests twitch = {
	.update_info = NULL,
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.ingests_refreshed = false,
	.ingests_refreshing = false,
	.ingests_loaded = false,
	.cur_ingests = {0},
	.cache_old_filename = "twitch_ingests.json",
	.cache_new_filename = "twitch_ingests.new.json"};

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

struct ingest twitch_ingest(size_t idx)
{
	return get_ingest(&twitch, idx);
}

void init_twitch_data(void)
{
	init_service_data(&twitch);
}

void twitch_ingests_refresh(int seconds)
{
	service_ingests_refresh(&twitch, seconds, "[twitch ingest update] ",
				"https://ingest.twitch.tv/ingests");
}

void load_twitch_data(void)
{
	struct ingest def = {.name = bstrdup("Default"),
			     .url = bstrdup("rtmp://live.twitch.tv/app")};
	load_service_data(&twitch, "twitch_ingests.json", &def);
}

void unload_twitch_data(void)
{
	unload_service_data(&twitch);
}
