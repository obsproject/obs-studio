#pragma once

#include "service-ingest.h"

extern void amazon_ivs_ingests_lock(void);
extern void amazon_ivs_ingests_unlock(void);
extern size_t amazon_ivs_ingest_count(void);
extern struct ingest amazon_ivs_ingest(size_t idx);
