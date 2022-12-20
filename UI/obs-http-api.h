#pragma once

#define MAX_STREAM_URL_LENGTH 1024
#define MAX_STREAM_KEY_LENGTH 1024

typedef struct {
	char streamUrl[MAX_STREAM_URL_LENGTH];
	char streamKey[MAX_STREAM_KEY_LENGTH];
} OBSHttpApiValue;

class OBSHttpApi {

public:
	bool HttpApiStart();
	void HttpApiExit();
	char* GetLocalIpAddr();
};

