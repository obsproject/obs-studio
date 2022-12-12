#pragma once

class OBSHttpApi {

public:
	bool HttpApiStart();
	void HttpApiExit();
	char* GetLocalIpAddr();
};
