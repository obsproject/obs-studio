#pragma once

struct showroom_ingest {
	char *url;
	char *key;
};
typedef struct showroom_ingest showroom_ingest;
extern const showroom_ingest showroom_get_ingest(const char *server,
						 const char *accessKey);
