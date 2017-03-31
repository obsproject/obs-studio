#pragma once

#include "platform.hpp"

#include <obs-module.h>

#include <map>
#include <vector>
#include <mutex>

class DeckLinkDeviceDiscovery;
class DeckLinkDeviceInstance;
class DeckLinkDevice;
class DeckLinkDeviceMode;

class DeckLink {
protected:
	ComPtr<DeckLinkDeviceInstance>        instance;
	DeckLinkDeviceDiscovery               *discovery;
	bool                                  isCapturing = false;
	obs_source_t                          *source;
	volatile long                         activateRefs = 0;
	std::recursive_mutex                  deviceMutex;
	BMDPixelFormat                        pixelFormat = bmdFormat8BitYUV;
	speaker_layout                        channelFormat = SPEAKERS_STEREO;

	void SaveSettings();
	static void DevicesChanged(void *param, DeckLinkDevice *device,
			bool added);

public:
	DeckLink(obs_source_t *source, DeckLinkDeviceDiscovery *discovery);
	virtual ~DeckLink(void);

	DeckLinkDevice *GetDevice() const;

	long long GetActiveModeId(void) const;
	obs_source_t *GetSource(void) const;

	inline BMDPixelFormat GetPixelFormat() const {return pixelFormat;}
	inline void SetPixelFormat(BMDPixelFormat format)
	{
		pixelFormat = format;
	}
	inline speaker_layout GetChannelFormat() const {return channelFormat;}
	inline void SetChannelFormat(speaker_layout format)
	{
		channelFormat = format;
	}

	bool Activate(DeckLinkDevice *device, long long modeId);
	void Deactivate();
};
