#pragma once

#include "service-ingest.h"

extern void twitch_ingests_lock(void);
extern void twitch_ingests_unlock(void);
extern size_t twitch_ingest_count(void);
extern struct ingest twitch_ingest(size_t idx);
