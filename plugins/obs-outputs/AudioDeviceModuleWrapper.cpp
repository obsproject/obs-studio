#include "AudioDeviceModuleWrapper.h"

#include "obs.h"
#include "media-io/audio-io.h"

AudioDeviceModuleWrapper::AudioDeviceModuleWrapper() : _initialized(false)
{
	pendingLength = 0;
}

AudioDeviceModuleWrapper::~AudioDeviceModuleWrapper() {}

rtc::scoped_refptr<AudioDeviceModuleWrapper>
AudioDeviceModuleWrapper::CreateAudioDeviceModule()
{
	return rtc::scoped_refptr<AudioDeviceModuleWrapper>(
		new rtc::RefCountedObject<AudioDeviceModuleWrapper>());
}

int32_t AudioDeviceModuleWrapper::Init()
{
	if (_initialized)
		return 0;
	_initialized = true;
	return 0;
}

int32_t AudioDeviceModuleWrapper::Terminate()
{
	if (!_initialized)
		return 0;
	_initialized = false;
	return 0;
}

bool AudioDeviceModuleWrapper::Initialized() const
{
	return _initialized;
}

void AudioDeviceModuleWrapper::onIncomingData(uint8_t *data,
					      size_t samples_per_channel)
{
	_critSect.Enter();
	if (!audioTransport)
		return;
	_critSect.Leave();

	// Get audio
	audio_t *audio = obs_get_audio();
	// This info is set on the stream before starting capture
	size_t channels = 2;
	uint32_t sample_rate = 48000;
	size_t sample_size = 2;
	// Get chunk for 10ms
	size_t chunk = (sample_rate / 100);

	size_t i = 0;
	uint32_t level;

	// Check if we had pending chunks
	if (pendingLength) {
		// Copy missing chunks
		i = chunk - pendingLength;
		memcpy(pending + pendingLength * sample_size * channels, data,
		       i * sample_size * channels);
		// Add sent
		audioTransport->RecordedDataIsAvailable(pending, chunk,
							sample_size * channels,
							channels, sample_rate,
							0, 0, 0, false, level);
		// No pending chunks
		pendingLength = 0;
	}

	// Send all possible full chunks
	while (i + chunk < samples_per_channel) {
		// Send them
		audioTransport->RecordedDataIsAvailable(
			data + i * sample_size * channels, chunk,
			sample_size * channels, channels, sample_rate, 0, 0, 0,
			false, level);
		i += chunk;
	}

	// If there are missing chunks
	if (i != samples_per_channel) {
		// Calculate pending chunks
		pendingLength = samples_per_channel - i;
		// Copy chunks to pending buffer
		memcpy(pending, data + i * sample_size * channels,
		       pendingLength * sample_size * channels);
	}
}
