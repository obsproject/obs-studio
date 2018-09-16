#include "SpoutPlugin.hpp"
#include <obs-module.h>
#include <util/platform.h>

SpoutPlugin::SpoutPlugin() {
	receiver = new SpoutReceiver();
	senderName = nullptr;
}

std::vector<std::string> SpoutPlugin::GetSenderNames() {
	std::vector<std::string> senders;
	int index, nSenders;
	char SenderName[64];
	int MaxSize = 64;

	nSenders = receiver->GetSenderCount();
	if (nSenders > 0) {
		for (index = 0; index<nSenders; index++) {
			receiver->GetSenderName(index, SenderName, MaxSize);
			senders.push_back(std::string(SenderName));
		}
	}

	return senders;
}

int SpoutPlugin::GetHeight() {
	return height;
}

int SpoutPlugin::GetWidth() {
	return width;
}

void SpoutPlugin::SetActiveSender(const char* senderName) {
	if (senderName == nullptr) {
		return;
	}

	this->senderName = senderName;
}

void SpoutPlugin::Tick() {
	if (this->senderName == nullptr) {
		return;
	}

	if (!initialized) {
		unsigned int width, height;
		char name[256];
		if (receiver->CreateReceiver(name, width, height, true)) {
			initialized = true;
			return;
		}
	}

	else {

		uint64_t   ts = os_gettime_ns();
		unsigned int width, height;
		unsigned char * pixels = NULL;

		char * name = strdup(this->senderName);
		bool success = receiver->ReceiveImage(name, width, height, pixels, GL_BGRA_EXT);

		if (!success) {
			return;
		}

		struct obs_source_frame frame = {};
		frame.format = VIDEO_FORMAT_BGRA;
		frame.width = width;
		frame.height = height;
		frame.data[0] = pixels;
		frame.linesize[0] = width * 4;
		frame.timestamp = ts;

		obs_source_output_video(source, &frame);
	}
}