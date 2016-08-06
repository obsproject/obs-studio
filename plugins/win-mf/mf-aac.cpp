#include <obs-module.h>

#include <memory>

#include "mf-aac-encoder.hpp"

#include <VersionHelpers.h>

using namespace MFAAC;

static const char *MFAAC_GetName(void*)
{
	return obs_module_text("MFAACEnc");
}

static obs_properties_t *MFAAC_GetProperties(void *)
{
	obs_properties_t *props = obs_properties_create();

	obs_properties_add_int(props, "bitrate",
			obs_module_text("Bitrate"), 96, 192, 32);

	return props;
}

static void MFAAC_GetDefaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "bitrate", 128);
}

static void *MFAAC_Create(obs_data_t *settings, obs_encoder_t *encoder)
{
	UINT32 bitrate = (UINT32)obs_data_get_int(settings, "bitrate");
	if (!bitrate) {
		MF_LOG_ENCODER("AAC", encoder, LOG_ERROR,
			"Invalid bitrate specified");
		return NULL;
	}

	audio_t *audio = obs_encoder_audio(encoder);
	UINT32 channels = (UINT32)audio_output_get_channels(audio);
	UINT32 sampleRate = audio_output_get_sample_rate(audio);
	UINT32 bitsPerSample = 16;

	UINT32 recommendedSampleRate = FindBestSamplerateMatch(sampleRate);
	if (recommendedSampleRate != sampleRate) {
		MF_LOG_ENCODER("AAC", encoder, LOG_WARNING,
			"unsupported sample rate; "
			"resampling to best guess '%d' instead of '%d'",
			recommendedSampleRate, sampleRate);
		sampleRate = recommendedSampleRate;
	}

	UINT32 recommendedBitRate = FindBestBitrateMatch(bitrate);
	if (recommendedBitRate != bitrate) {
		MF_LOG_ENCODER("AAC", encoder, LOG_WARNING,
			"unsupported bitrate; "
			"resampling to best guess '%d' instead of '%d'",
			recommendedBitRate, bitrate);
		bitrate = recommendedBitRate;
	}

	std::unique_ptr<Encoder> enc(new Encoder(encoder,
			bitrate, channels, sampleRate, bitsPerSample));

	if (!enc->Initialize())
		return nullptr;

	return enc.release();
}

static void MFAAC_Destroy(void *data)
{
	Encoder *enc = static_cast<Encoder *>(data);
	delete enc;
}

static bool MFAAC_Encode(void *data, struct encoder_frame *frame,
		struct encoder_packet *packet, bool *received_packet)
{
	Encoder *enc = static_cast<Encoder *>(data);
	Status status;

	if (!enc->ProcessInput(frame->data[0], frame->linesize[0], frame->pts,
			&status))
		return false;

	// This shouldn't happen since we drain right after
	// we process input
	if (status == NOT_ACCEPTING)
		return false;

	UINT8 *outputData;
	UINT32 outputDataLength;
	UINT64 outputPts;

	if (!enc->ProcessOutput(&outputData, &outputDataLength, &outputPts,
			&status))
		return false;

	// Needs more input, not a failure case
	if (status == NEED_MORE_INPUT)
		return true;

	packet->pts = outputPts;
	packet->dts = outputPts;
	packet->data = outputData;
	packet->size = outputDataLength;
	packet->type = OBS_ENCODER_AUDIO;
	packet->timebase_num = 1;
	packet->timebase_den = enc->SampleRate();

	return *received_packet = true;
}

static bool MFAAC_GetExtraData(void *data, uint8_t **extra_data, size_t *size)
{
	Encoder *enc = static_cast<Encoder *>(data);

	UINT32 length;
	if (enc->ExtraData(extra_data, &length)) {
		*size = length;
		return true;
	}
	return false;
}

static void MFAAC_GetAudioInfo(void *, struct audio_convert_info *info)
{
	info->format = AUDIO_FORMAT_16BIT;
	info->samples_per_sec = FindBestSamplerateMatch(info->samples_per_sec);
}

static size_t MFAAC_GetFrameSize(void *)
{
	return Encoder::FrameSize;
}

extern "C" void RegisterMFAACEncoder()
{
	if (!IsWindows8OrGreater()) {
		MF_LOG(LOG_WARNING, "plugin is disabled for performance "
			"reasons on Windows versions prior to 8");
		return;
	}

	obs_encoder_info info = {};
	info.id                        = "mf_aac";
	info.type                      = OBS_ENCODER_AUDIO;
	info.codec                     = "AAC";
	info.get_name                  = MFAAC_GetName;
	info.create                    = MFAAC_Create;
	info.destroy                   = MFAAC_Destroy;
	info.encode                    = MFAAC_Encode;
	info.get_frame_size            = MFAAC_GetFrameSize;
	info.get_defaults              = MFAAC_GetDefaults;
	info.get_properties            = MFAAC_GetProperties;
	info.get_extra_data            = MFAAC_GetExtraData;
	info.get_audio_info            = MFAAC_GetAudioInfo;

	MF_LOG(LOG_DEBUG, "Adding Media Foundation AAC Encoder");

	obs_register_encoder(&info);

}
