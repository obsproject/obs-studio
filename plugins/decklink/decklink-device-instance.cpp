#include "decklink-device-instance.hpp"

#include <util/platform.h>
#include <util/threading.h>

#include <sstream>

#define LOG(level, message, ...) blog(level, "%s: " message, \
		obs_source_get_name(this->decklink->GetSource()), ##__VA_ARGS__)

static inline enum video_format ConvertPixelFormat(BMDPixelFormat format)
{
	switch (format) {
	case bmdFormat8BitBGRA: return VIDEO_FORMAT_BGRX;

	default:
	case bmdFormat8BitYUV:;
	}

	return VIDEO_FORMAT_UYVY;
}

DeckLinkDeviceInstance::DeckLinkDeviceInstance(DeckLink *decklink_,
		DeckLinkDevice *device_) :
	currentFrame(), currentPacket(), decklink(decklink_), device(device_)
{
	currentPacket.samples_per_sec = 48000;
	currentPacket.speakers        = SPEAKERS_STEREO;
	currentPacket.format          = AUDIO_FORMAT_16BIT;
}

DeckLinkDeviceInstance::~DeckLinkDeviceInstance()
{
}

void DeckLinkDeviceInstance::HandleAudioPacket(
		IDeckLinkAudioInputPacket *audioPacket,
		const uint64_t timestamp)
{
	if (audioPacket == nullptr)
		return;

	void *bytes;
	if (audioPacket->GetBytes(&bytes) != S_OK) {
		LOG(LOG_WARNING, "Failed to get audio packet data");
		return;
	}

	currentPacket.data[0]   = (uint8_t *)bytes;
	currentPacket.frames    = (uint32_t)audioPacket->GetSampleFrameCount();
	currentPacket.timestamp = timestamp;

	obs_source_output_audio(decklink->GetSource(), &currentPacket);
}

void DeckLinkDeviceInstance::HandleVideoFrame(
		IDeckLinkVideoInputFrame *videoFrame, const uint64_t timestamp)
{
	if (videoFrame == nullptr)
		return;

	void *bytes;
	if (videoFrame->GetBytes(&bytes) != S_OK) {
		LOG(LOG_WARNING, "Failed to get video frame data");
		return;
	}

	currentFrame.data[0]     = (uint8_t *)bytes;
	currentFrame.linesize[0] = (uint32_t)videoFrame->GetRowBytes();
	currentFrame.width       = (uint32_t)videoFrame->GetWidth();
	currentFrame.height      = (uint32_t)videoFrame->GetHeight();
	currentFrame.timestamp   = timestamp;

	video_format_get_parameters(VIDEO_CS_601, VIDEO_RANGE_PARTIAL,
			currentFrame.color_matrix, currentFrame.color_range_min,
			currentFrame.color_range_max);

	obs_source_output_video(decklink->GetSource(), &currentFrame);
}

bool DeckLinkDeviceInstance::StartCapture(DeckLinkDeviceMode *mode_)
{
	if (mode != nullptr)
		return false;
	if (mode_ == nullptr)
		return false;

	LOG(LOG_INFO, "Starting capture...");

	if (!device->GetInput(&input))
		return false;

	pixelFormat = decklink->GetPixelFormat();
	currentFrame.format = ConvertPixelFormat(pixelFormat);

	input->SetCallback(this);

	const BMDDisplayMode displayMode = mode_->GetDisplayMode();

	const HRESULT videoResult = input->EnableVideoInput(displayMode,
			pixelFormat, bmdVideoInputFlagDefault);

	if (videoResult != S_OK) {
		LOG(LOG_ERROR, "Failed to enable video input");
		input->SetCallback(nullptr);
		return false;
	}

	const HRESULT audioResult = input->EnableAudioInput(
			bmdAudioSampleRate48kHz, bmdAudioSampleType16bitInteger,
			2);

	if (audioResult != S_OK)
		LOG(LOG_WARNING, "Failed to enable audio input; continuing...");

	if (input->StartStreams() != S_OK) {
		LOG(LOG_ERROR, "Failed to start streams");
		input->SetCallback(nullptr);
		input->DisableVideoInput();
		input->DisableAudioInput();
		return false;
	}

	mode = mode_;

	return true;
}

bool DeckLinkDeviceInstance::StopCapture(void)
{
	if (mode == nullptr || input == nullptr)
		return false;

	LOG(LOG_INFO, "Stopping capture of '%s'...",
			GetDevice()->GetDisplayName().c_str());

	input->StopStreams();
	input->SetCallback(nullptr);
	input->DisableVideoInput();
	input->DisableAudioInput();

	mode = nullptr;

	return true;
}

#define TIME_BASE 1000000000

HRESULT STDMETHODCALLTYPE DeckLinkDeviceInstance::VideoInputFrameArrived(
		IDeckLinkVideoInputFrame *videoFrame,
		IDeckLinkAudioInputPacket *audioPacket)
{
	BMDTimeValue videoTS = 0;
	BMDTimeValue videoDur = 0;
	BMDTimeValue audioTS = 0;

	if (videoFrame)
		videoFrame->GetStreamTime(&videoTS, &videoDur, TIME_BASE);
	if (audioPacket)
		audioPacket->GetPacketTime(&audioTS, TIME_BASE);

	if (videoFrame && videoTS >= 0)
		HandleVideoFrame(videoFrame, (uint64_t)videoTS);
	if (audioPacket && audioTS >= 0)
		HandleAudioPacket(audioPacket, (uint64_t)audioTS);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE DeckLinkDeviceInstance::VideoInputFormatChanged(
		BMDVideoInputFormatChangedEvents events,
		IDeckLinkDisplayMode *newMode,
		BMDDetectedVideoInputFormatFlags detectedSignalFlags)
{
	UNUSED_PARAMETER(events);
	UNUSED_PARAMETER(newMode);
	UNUSED_PARAMETER(detectedSignalFlags);

	// There is no implementation for automatic format detection, so this
	// method goes unused.

	return S_OK;
}

ULONG STDMETHODCALLTYPE DeckLinkDeviceInstance::AddRef(void)
{
	return os_atomic_inc_long(&refCount);
}

HRESULT STDMETHODCALLTYPE DeckLinkDeviceInstance::QueryInterface(REFIID iid,
		LPVOID *ppv)
{
	HRESULT result = E_NOINTERFACE;

	*ppv = nullptr;

	CFUUIDBytes unknown = CFUUIDGetUUIDBytes(IUnknownUUID);
	if (memcmp(&iid, &unknown, sizeof(REFIID)) == 0) {
		*ppv = this;
		AddRef();
		result = S_OK;
	} else if (memcmp(&iid, &IID_IDeckLinkNotificationCallback,
				sizeof(REFIID)) == 0) {
		*ppv = (IDeckLinkNotificationCallback *)this;
		AddRef();
		result = S_OK;
	}

	return result;
}

ULONG STDMETHODCALLTYPE DeckLinkDeviceInstance::Release(void)
{
	const long newRefCount = os_atomic_dec_long(&refCount);
	if (newRefCount == 0) {
		delete this;
		return 0;
	}

	return newRefCount;
}
