#include "Spout2/SpoutSDK/Source/spout.h"
#include <obs-module.h>

class SpoutPlugin {
	SpoutReceiver* receiver;
	int width;
	int height;
	const char* senderName;
	bool initialized = false;

public:
	SpoutPlugin();
	std::vector<std::string> GetSenderNames();
	int GetWidth();
	int GetHeight();
	void SetActiveSender(const char* senderName);
	void Tick();

	obs_source_t* source;
};