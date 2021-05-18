#pragma once

struct onlyfans_ingest {
	const char *name;
	const char *url;
};

extern void onlyfans_ingests_lock(void);
extern void onlyfans_ingests_unlock(void);
extern size_t onlyfans_ingest_count(void);
extern struct onlyfans_ingest get_onlyfans_ingest(size_t idx);
//!<
extern void add_onlyfans_ingest(const char *name, const char *url);
