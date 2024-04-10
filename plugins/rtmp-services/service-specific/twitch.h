#pragma once

struct twitch_ingest {
	const char *name;
	const char *url;
};

extern void twitch_ingests_lock(void);
extern void twitch_ingests_unlock(void);
extern size_t twitch_ingest_count(void);
extern struct twitch_ingest twitch_ingest(size_t idx);

extern void amazon_ivs_ingests_lock(void);
extern void amazon_ivs_ingests_unlock(void);
extern size_t amazon_ivs_ingest_count(void);
extern struct twitch_ingest amazon_ivs_ingest(size_t idx);
