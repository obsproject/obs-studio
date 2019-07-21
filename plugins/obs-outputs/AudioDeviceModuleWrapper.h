#ifndef _AUDIO_DEVICE_MODULE_WRAPPER_H_
#define _AUDIO_DEVICE_MODULE_WRAPPER_H_

#include "api/scoped_refptr.h"
#include "modules/audio_device/include/audio_device_default.h"
#include "rtc_base/checks.h"
#include "rtc_base/critical_section.h"
#include "rtc_base/ref_counted_object.h"

#include <stdio.h>

using webrtc::AudioDeviceModule;
using webrtc::AudioTransport;
using webrtc::kAdmMaxDeviceNameSize;
using webrtc::kAdmMaxGuidSize;
using webrtc::kAdmMaxFileNameSize;
using webrtc::webrtc_impl::AudioDeviceModuleDefault;

class AudioDeviceModuleWrapper
	: public AudioDeviceModuleDefault<AudioDeviceModule> {
public:
	AudioDeviceModuleWrapper();
	~AudioDeviceModuleWrapper() override;

	rtc::scoped_refptr<AudioDeviceModuleWrapper> CreateAudioDeviceModule();

	// Main initialization and termination
	int32_t Init() override;
	int32_t Terminate() override;
	bool Initialized() const override;

	void onIncomingData(uint8_t *data, size_t samples_per_channel);

	virtual int64_t TimeUntilNextProcess() { return 1000; }
	virtual void Process() {}

	// Retrieve the currently utilized audio layer
	int32_t ActiveAudioLayer(AudioLayer *audioLayer) const override
	{
		*audioLayer = AudioLayer::kDummyAudio;
		return 0;
	}

	// Full-duplex transportation of PCM audio
	int32_t RegisterAudioCallback(AudioTransport *audioCallback) override
	{
		this->audioTransport = audioCallback;
		return 0;
	}

	// Device enumeration
	int32_t PlayoutDeviceName(uint16_t index,
				  char name[kAdmMaxDeviceNameSize],
				  char guid[kAdmMaxGuidSize]) override
	{
		sprintf(name, "rtmp_stream");
		sprintf(name, "obs");
		sprintf(guid, "guid");
		return 0;
	}

	int32_t RecordingDeviceName(uint16_t index,
				    char name[kAdmMaxDeviceNameSize],
				    char guid[kAdmMaxGuidSize]) override
	{
		sprintf(name, "rtmp_stream");
		sprintf(name, "obs");
		sprintf(guid, "guid");
		return 0;
	}

	// Audio transport control
	bool Recording() const override { return true; }

	// Stereo support
	int32_t StereoRecordingIsAvailable(bool *available) const override
	{
		*available = true;
		return 0;
	}

public:
	bool _initialized;
	rtc::CriticalSection _critSect;
	AudioTransport *audioTransport;
	uint8_t pending[640 * 2 * 2];
	size_t pendingLength;
};

#endif
