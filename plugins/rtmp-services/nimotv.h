#pragma once

struct nimotv_ingest {
	const char *name;
	const char *url;
};

extern void nimotv_ingests_lock(void);
extern void nimotv_ingests_unlock(void);
extern size_t nimotv_ingest_count(void);
extern struct nimotv_ingest nimotv_ingest(size_t idx);
extern const char *get_recommended_ingest();
