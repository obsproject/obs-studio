#include "decklink-device-instance.hpp"
#include "audio-repack.hpp"

#include <util/platform.h>
#include <util/threading.h>

#include <sstream>

#define LOG(level, message, ...) blog(level, "%s: " message, \
		obs_source_get_name(this->decklink->GetSource()), ##__VA_ARGS__)

#define ISSTEREO(flag) ((flag) == SPEAKERS_STEREO)

static inline enum video_format ConvertPixelFormat(BMDPixelFormat format)
{
	switch (format) {
	case bmdFormat8BitBGRA: return VIDEO_FORMAT_BGRX;

	default:
	case bmdFormat8BitYUV:;
	}

	return VIDEO_FORMAT_UYVY;
}

static inline int ConvertChannelFormat(speaker_layout format)
{
	switch (format) {
	case SPEAKERS_5POINT1:
	case SPEAKERS_5POINT1_SURROUND:
	case SPEAKERS_7POINT1:
		return 8;

	default:
	case SPEAKERS_STEREO:
		return 2;
	}
}

static inline audio_repack_mode_t ConvertRepackFormat(speaker_layout format)
{
	switch (format) {
	case SPEAKERS_5POINT1:
	case SPEAKERS_5POINT1_SURROUND:
		return repack_mode_8to6ch_swap23;

	case SPEAKERS_7POINT1:
		return repack_mode_8ch_swap23;

	default:
		assert(false && "No repack requested");
		return (audio_repack_mode_t)-1;
	}
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

	const uint32_t frameCount = (uint32_t)audioPacket->GetSampleFrameCount();
	currentPacket.frames      = frameCount;
	currentPacket.timestamp   = timestamp;

	if (!ISSTEREO(channelFormat)) {
		if (audioRepacker->repack((uint8_t *)bytes, frameCount) < 0) {
			LOG(LOG_ERROR, "Failed to convert audio packet data");
			return;
		}

		currentPacket.data[0]   = (*audioRepacker)->packet_buffer;
	} else {
		currentPacket.data[0]   = (uint8_t *)bytes;
	}

	nextAudioTS = timestamp +
		((uint64_t)frameCount * 1000000000ULL / 48000ULL) + 1;

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

	obs_source_output_video(decklink->GetSource(), &currentFrame);
}

void DeckLinkDeviceInstance::FinalizeStream()
{
	input->SetCallback(nullptr);
	input->DisableVideoInput();
	if (channelFormat != SPEAKERS_UNKNOWN)
		input->DisableAudioInput();

	if (audioRepacker != nullptr)
	{
		delete audioRepacker;
		audioRepacker = nullptr;
	}

	mode = nullptr;
}

//#define LOG_SETUP_VIDEO_FORMAT 1

void DeckLinkDeviceInstance::SetupVideoFormat(DeckLinkDeviceMode *mode_)
{
	if (mode_ == nullptr)
		return;

	currentFrame.format = ConvertPixelFormat(pixelFormat);

	colorSpace = decklink->GetColorSpace();
	if (colorSpace == VIDEO_CS_DEFAULT) {
		const BMDDisplayModeFlags flags = mode_->GetDisplayModeFlags();
		if (flags & bmdDisplayModeColorspaceRec709)
			activeColorSpace = VIDEO_CS_709;
		else if (flags & bmdDisplayModeColorspaceRec601)
			activeColorSpace = VIDEO_CS_601;
		else
			activeColorSpace = VIDEO_CS_DEFAULT;
	} else {
		activeColorSpace = colorSpace;
	}

	colorRange = decklink->GetColorRange();
	currentFrame.full_range = colorRange == VIDEO_RANGE_FULL;

	video_format_get_parameters(activeColorSpace, colorRange,
			currentFrame.color_matrix, currentFrame.color_range_min,
			currentFrame.color_range_max);

#ifdef LOG_SETUP_VIDEO_FORMAT
	LOG(LOG_INFO, "Setup video format: %s, %s, %s",
			pixelFormat == bmdFormat8BitYUV ? "YUV" : "RGB",
			activeColorSpace == VIDEO_CS_709 ? "BT.709" : "BT.601",
			colorRange == VIDEO_RANGE_FULL ? "full" : "limited");
#endif
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

	BMDVideoInputFlags flags;

	bool isauto = mode_->GetName() == "Auto";
	if (isauto) {
		displayMode = bmdModeNTSC;
		pixelFormat = bmdFormat8BitYUV;
		flags = bmdVideoInputEnableFormatDetection;
	} else {
		displayMode = mode_->GetDisplayMode();
		pixelFormat = decklink->GetPixelFormat();
		flags = bmdVideoInputFlagDefault;
	}

	const HRESULT videoResult = input->EnableVideoInput(displayMode,
			pixelFormat, flags);
	if (videoResult != S_OK) {
		LOG(LOG_ERROR, "Failed to enable video input");
		return false;
	}

	SetupVideoFormat(mode_);

	channelFormat = decklink->GetChannelFormat();
	currentPacket.speakers = channelFormat;

	if (channelFormat != SPEAKERS_UNKNOWN) {
		const int channel = ConvertChannelFormat(channelFormat);
		const HRESULT audioResult = input->EnableAudioInput(
				bmdAudioSampleRate48kHz, bmdAudioSampleType16bitInteger,
				channel);
		if (audioResult != S_OK)
			LOG(LOG_WARNING, "Failed to enable audio input; continuing...");

		if (!ISSTEREO(channelFormat)) {
			const audio_repack_mode_t repack_mode = ConvertRepackFormat(channelFormat);
			audioRepacker = new AudioRepacker(repack_mode);
		}
	}

	if (input->SetCallback(this) != S_OK) {
		LOG(LOG_ERROR, "Failed to set callback");
		FinalizeStream();
		return false;
	}

	if (input->StartStreams() != S_OK) {
		LOG(LOG_ERROR, "Failed to start streams");
		FinalizeStream();
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
	FinalizeStream();

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

	if (videoFrame) {
		videoFrame->GetStreamTime(&videoTS, &videoDur, TIME_BASE);
		lastVideoTS = (uint64_t)videoTS;
	}
	if (audioPacket) {
		BMDTimeValue newAudioTS = 0;
		int64_t diff;

		audioPacket->GetPacketTime(&newAudioTS, TIME_BASE);
		audioTS = newAudioTS + audioOffset;

		diff = (int64_t)audioTS - (int64_t)nextAudioTS;
		if (diff > 10000000LL) {
			audioOffset -= diff;
			audioTS = newAudioTS + audioOffset;

		} else if (diff < -1000000) {
			audioOffset = 0;
			audioTS = newAudioTS;
		}
	}

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
	input->PauseStreams();

	mode->SetMode(newMode);

	if (events & bmdVideoInputDisplayModeChanged) {
		displayMode = mode->GetDisplayMode();
	}

	if (events & bmdVideoInputColorspaceChanged) {
		switch (detectedSignalFlags) {
		case bmdDetectedVideoInputRGB444:
			pixelFormat = bmdFormat8BitBGRA;
			break;

		default:
		case bmdDetectedVideoInputYCbCr422:
			pixelFormat = bmdFormat8BitYUV;
			break;
		}
	}

	const HRESULT videoResult = input->EnableVideoInput(displayMode,
			pixelFormat, bmdVideoInputEnableFormatDetection);
	if (videoResult != S_OK) {
		LOG(LOG_ERROR, "Failed to enable video input");
		input->StopStreams();
		FinalizeStream();

		return E_FAIL;
	}

	SetupVideoFormat(mode);

	input->FlushStreams();
	input->StartStreams();

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
