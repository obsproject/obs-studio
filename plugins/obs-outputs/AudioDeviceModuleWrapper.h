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

	static rtc::scoped_refptr<AudioDeviceModuleWrapper> Create();

	void onIncomingData(uint8_t *data, size_t samples_per_channel);

	virtual int64_t TimeUntilNextProcess();
	virtual void Process() {}

	// Main initialization and termination
	int32_t Init() override;
	int32_t Terminate() override;
	bool Initialized() const override;

	// Retrieve the currently utilized audio layer
	int32_t ActiveAudioLayer(AudioLayer *audioLayer) const override;

	// Full-duplex transportation of PCM audio
	int32_t RegisterAudioCallback(AudioTransport *audioCallback) override;

	// Device enumeration
	int32_t PlayoutDeviceName(uint16_t index,
				  char name[kAdmMaxDeviceNameSize],
				  char guid[kAdmMaxGuidSize]) override;
	int32_t RecordingDeviceName(uint16_t index,
				    char name[kAdmMaxDeviceNameSize],
				    char guid[kAdmMaxGuidSize]) override;

	// Audio transport control
	bool Recording() const override;

	// Stereo support
	int32_t StereoRecordingIsAvailable(bool *available) const override;

private:
	bool initialized_;
	rtc::CriticalSection crit_;
	AudioTransport *audioTransport_;
	uint8_t pending_[640 * 2 * 2];
	size_t pendingLength_;
};

#endif
