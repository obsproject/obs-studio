#include "service-ingest.h"
#include "amazon-ivs.h"

static struct service_ingests amazon_ivs = {
	.update_info = NULL,
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.ingests_refreshed = false,
	.ingests_refreshing = false,
	.ingests_loaded = false,
	.cur_ingests = {0},
	.cache_old_filename = "amazon_ivs_ingests.json",
	.cache_new_filename = "amazon_ivs_ingests.new.json"};

void init_amazon_ivs_data(void)
{
	init_service_data(&amazon_ivs);
}

void load_amazon_ivs_data(void)
{
	struct ingest def = {
		.name = bstrdup("Default"),
		.url = bstrdup(
			"rtmps://ingest.global-contribute.live-video.net:443/app/")};
	load_service_data(&amazon_ivs, "amazon_ivs_ingests.json", &def);
}

void unload_amazon_ivs_data(void)
{
	unload_service_data(&amazon_ivs);
}

void amazon_ivs_ingests_refresh(int seconds)
{
	service_ingests_refresh(
		&amazon_ivs, seconds, "[amazon ivs ingest update] ",
		"https://ingest.contribute.live-video.net/ingests");
}

void amazon_ivs_ingests_lock(void)
{
	pthread_mutex_lock(&amazon_ivs.mutex);
}

void amazon_ivs_ingests_unlock(void)
{
	pthread_mutex_unlock(&amazon_ivs.mutex);
}

size_t amazon_ivs_ingest_count(void)
{
	return amazon_ivs.cur_ingests.num;
}

struct ingest amazon_ivs_ingest(size_t idx)
{
	return get_ingest(&amazon_ivs, idx);
}
