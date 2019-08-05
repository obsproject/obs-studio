#include "AudioDeviceModuleWrapper.h"

#include "obs.h"
#include "media-io/audio-io.h"

AudioDeviceModuleWrapper::AudioDeviceModuleWrapper()
	: initialized_(false), pendingLength_(0)
{
}

AudioDeviceModuleWrapper::~AudioDeviceModuleWrapper()
{
	Terminate();
}

rtc::scoped_refptr<AudioDeviceModuleWrapper> AudioDeviceModuleWrapper::Create()
{
	rtc::scoped_refptr<AudioDeviceModuleWrapper> capture_module(
		new rtc::RefCountedObject<AudioDeviceModuleWrapper>());
	capture_module->Init();
	return capture_module;
}

void AudioDeviceModuleWrapper::onIncomingData(uint8_t *data,
					      size_t samples_per_channel)
{
	crit_.Enter();
	if (!audioTransport_)
		return;
	crit_.Leave();

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
	if (pendingLength_) {
		// Copy missing chunks
		i = chunk - pendingLength_;
		memcpy(pending_ + pendingLength_ * sample_size * channels, data,
		       i * sample_size * channels);
		// Add sent
		audioTransport_->RecordedDataIsAvailable(pending_, chunk,
							 sample_size * channels,
							 channels, sample_rate,
							 0, 0, 0, false, level);
		// No pending chunks
		pendingLength_ = 0;
	}

	// Send all possible full chunks
	while (i + chunk < samples_per_channel) {
		// Send them
		audioTransport_->RecordedDataIsAvailable(
			data + i * sample_size * channels, chunk,
			sample_size * channels, channels, sample_rate, 0, 0, 0,
			false, level);
		i += chunk;
	}

	// If there are missing chunks
	if (i != samples_per_channel) {
		// Calculate pending chunks
		pendingLength_ = samples_per_channel - i;
		// Copy chunks to pending buffer
		memcpy(pending_, data + i * sample_size * channels,
		       pendingLength_ * sample_size * channels);
	}
}

int64_t AudioDeviceModuleWrapper::TimeUntilNextProcess()
{
	return 1000;
}

int32_t AudioDeviceModuleWrapper::Init()
{
	if (initialized_)
		return 0;
	initialized_ = true;
	return 0;
}

int32_t AudioDeviceModuleWrapper::Terminate()
{
	if (!initialized_)
		return 0;
	initialized_ = false;
	return 0;
}

bool AudioDeviceModuleWrapper::Initialized() const
{
	return initialized_;
}

int32_t AudioDeviceModuleWrapper::ActiveAudioLayer(AudioLayer *audioLayer) const
{
	*audioLayer = AudioLayer::kDummyAudio;
	return 0;
}

int32_t
AudioDeviceModuleWrapper::RegisterAudioCallback(AudioTransport *audioCallback)
{
	this->audioTransport_ = audioCallback;
	return 0;
}

int32_t
AudioDeviceModuleWrapper::PlayoutDeviceName(uint16_t index,
					    char name[kAdmMaxDeviceNameSize],
					    char guid[kAdmMaxGuidSize])
{
	sprintf(name, "rtmp_stream");
	sprintf(name, "obs");
	sprintf(guid, "guid");
	return 0;
}

int32_t
AudioDeviceModuleWrapper::RecordingDeviceName(uint16_t index,
					      char name[kAdmMaxDeviceNameSize],
					      char guid[kAdmMaxGuidSize])
{
	sprintf(name, "rtmp_stream");
	sprintf(name, "obs");
	sprintf(guid, "guid");
	return 0;
}

bool AudioDeviceModuleWrapper::Recording() const
{
	return true;
}

int32_t
AudioDeviceModuleWrapper::StereoRecordingIsAvailable(bool *available) const
{
	*available = true;
	return 0;
}
