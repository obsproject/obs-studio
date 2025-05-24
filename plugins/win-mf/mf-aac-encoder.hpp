#pragma once

#define WIN32_MEAN_AND_LEAN
#include <Windows.h>
#undef WIN32_MEAN_AND_LEAN

#include <mfapi.h>
#include <mfidl.h>

#include <stdint.h>
#include <vector>

#include <util/windows/ComPtr.hpp>

inline void MF_LOG(int level, const char *format, ...)
{
	va_list args;
	va_start(args, format);

	char formattedMessage[1024];
	vsnprintf(formattedMessage, sizeof(formattedMessage), format, args);
	va_end(args);

	blog(level, "[Media Foundation encoder]: %s", formattedMessage);
}

inline void MF_LOG_ENCODER(const char *format_name, const obs_encoder_t *encoder, int level, const char *format, ...)
{
	va_list args;
	va_start(args, format);

	char formattedMessage[1024];
	vsnprintf(formattedMessage, sizeof(formattedMessage), format, args);
	va_end(args);
	blog(level, "[Media Foundation %s: '%s']: %s", format_name,
	     obs_encoder_get_name(static_cast<const obs_encoder_t *>(encoder)), formattedMessage);
}

namespace MFAAC {

enum Status { FAILURE, SUCCESS, NOT_ACCEPTING, NEED_MORE_INPUT };

class Encoder {
public:
	Encoder(const obs_encoder_t *encoder, UINT32 bitrate, UINT32 channels, UINT32 sampleRate, UINT32 bitsPerSample)
		: encoder(encoder),
		  bitrate(bitrate),
		  channels(channels),
		  sampleRate(sampleRate),
		  bitsPerSample(bitsPerSample)
	{
	}

	Encoder &operator=(Encoder const &) = delete;

	bool Initialize();
	bool ProcessInput(UINT8 *data, UINT32 dataLength, UINT64 pts, MFAAC::Status *status);
	bool ProcessOutput(UINT8 **data, UINT32 *dataLength, UINT64 *pts, MFAAC::Status *status);
	bool ExtraData(UINT8 **extraData, UINT32 *extraDataLength);

	const obs_encoder_t *ObsEncoder() const { return encoder; }
	UINT32 Bitrate() { return bitrate; }
	UINT32 Channels() { return channels; }
	UINT32 SampleRate() { return sampleRate; }
	UINT32 BitsPerSample() { return bitsPerSample; }

	static const UINT32 FrameSize = 1024;

private:
	void InitializeExtraData();
	HRESULT CreateMediaTypes(ComPtr<IMFMediaType> &inputType, ComPtr<IMFMediaType> &outputType);
	HRESULT EnsureCapacity(ComPtr<IMFSample> &sample, DWORD length);
	HRESULT CreateEmptySample(ComPtr<IMFSample> &sample, ComPtr<IMFMediaBuffer> &buffer, DWORD length);

private:
	const obs_encoder_t *encoder;
	const UINT32 bitrate;
	const UINT32 channels;
	const UINT32 sampleRate;
	const UINT32 bitsPerSample;

	ComPtr<IMFTransform> transform;
	ComPtr<IMFSample> outputSample;
	std::vector<BYTE> packetBuffer;
	UINT8 extraData[5];
};

static const UINT32 FrameSize = 1024;

UINT32 FindBestBitrateMatch(UINT32 value);
UINT32 FindBestChannelsMatch(UINT32 value);
UINT32 FindBestBitsPerSampleMatch(UINT32 value);
UINT32 FindBestSamplerateMatch(UINT32 value);
bool BitrateValid(UINT32 value);
bool ChannelsValid(UINT32 value);
bool BitsPerSampleValid(UINT32 value);
bool SamplerateValid(UINT32 value);
} // namespace MFAAC