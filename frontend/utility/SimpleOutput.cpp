#include "SimpleOutput.hpp"

#include <utility/audio-encoders.hpp>
#include <utility/StartMultiTrackVideoStreamingGuard.hpp>
#include <widgets/OBSBasic.hpp>

#include <qt-wrappers.hpp>

using namespace std;

static bool CreateSimpleAACEncoder(OBSEncoder &res, int bitrate, const char *name, size_t idx)
{
	const char *id_ = GetSimpleAACEncoderForBitrate(bitrate);
	if (!id_) {
		res = nullptr;
		return false;
	}

	res = obs_audio_encoder_create(id_, name, nullptr, idx, nullptr);

	if (res) {
		obs_encoder_release(res);
		return true;
	}

	return false;
}

static bool CreateSimpleOpusEncoder(OBSEncoder &res, int bitrate, const char *name, size_t idx)
{
	const char *id_ = GetSimpleOpusEncoderForBitrate(bitrate);
	if (!id_) {
		res = nullptr;
		return false;
	}

	res = obs_audio_encoder_create(id_, name, nullptr, idx, nullptr);

	if (res) {
		obs_encoder_release(res);
		return true;
	}

	return false;
}

extern bool EncoderAvailable(const char *encoder);

void SimpleOutput::LoadRecordingPreset_Lossless()
{
	fileOutput = obs_output_create("ffmpeg_output", "simple_ffmpeg_output", nullptr, nullptr);
	if (!fileOutput)
		throw "Failed to create recording FFmpeg output "
		      "(simple output)";

	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "format_name", "avi");
	obs_data_set_string(settings, "video_encoder", "utvideo");
	obs_data_set_string(settings, "audio_encoder", "pcm_s16le");

	obs_output_update(fileOutput, settings);
}

void SimpleOutput::LoadRecordingPreset_Lossy(const char *encoderId)
{
	videoRecording = obs_video_encoder_create(encoderId, "simple_video_recording", nullptr, nullptr);
	if (!videoRecording)
		throw "Failed to create video recording encoder (simple output)";
	obs_encoder_release(videoRecording);
}

void SimpleOutput::LoadStreamingPreset_Lossy(const char *encoderId)
{
	videoStreaming = obs_video_encoder_create(encoderId, "simple_video_stream", nullptr, nullptr);
	if (!videoStreaming)
		throw "Failed to create video streaming encoder (simple output)";
	obs_encoder_release(videoStreaming);
}

/* mistakes have been made to lead us to this. */
const char *get_simple_output_encoder(const char *encoder)
{
	if (strcmp(encoder, SIMPLE_ENCODER_X264) == 0) {
		return "obs_x264";
	} else if (strcmp(encoder, SIMPLE_ENCODER_X264_LOWCPU) == 0) {
		return "obs_x264";
	} else if (strcmp(encoder, SIMPLE_ENCODER_QSV) == 0) {
		return "obs_qsv11_v2";
	} else if (strcmp(encoder, SIMPLE_ENCODER_QSV_AV1) == 0) {
		return "obs_qsv11_av1";
	} else if (strcmp(encoder, SIMPLE_ENCODER_AMD) == 0) {
		return "h264_texture_amf";
#ifdef ENABLE_HEVC
	} else if (strcmp(encoder, SIMPLE_ENCODER_AMD_HEVC) == 0) {
		return "h265_texture_amf";
#endif
	} else if (strcmp(encoder, SIMPLE_ENCODER_AMD_AV1) == 0) {
		return "av1_texture_amf";
	} else if (strcmp(encoder, SIMPLE_ENCODER_NVENC) == 0) {
		return EncoderAvailable("obs_nvenc_h264_tex") ? "obs_nvenc_h264_tex" : "ffmpeg_nvenc";
#ifdef ENABLE_HEVC
	} else if (strcmp(encoder, SIMPLE_ENCODER_NVENC_HEVC) == 0) {
		return EncoderAvailable("obs_nvenc_hevc_tex") ? "obs_nvenc_hevc_tex" : "ffmpeg_hevc_nvenc";
#endif
	} else if (strcmp(encoder, SIMPLE_ENCODER_NVENC_AV1) == 0) {
		return "obs_nvenc_av1_tex";
	} else if (strcmp(encoder, SIMPLE_ENCODER_APPLE_H264) == 0) {
		return "com.apple.videotoolbox.videoencoder.ave.avc";
#ifdef ENABLE_HEVC
	} else if (strcmp(encoder, SIMPLE_ENCODER_APPLE_HEVC) == 0) {
		return "com.apple.videotoolbox.videoencoder.ave.hevc";
#endif
	}

	return "obs_x264";
}

void SimpleOutput::LoadRecordingPreset()
{
	const char *quality = config_get_string(main->Config(), "SimpleOutput", "RecQuality");
	const char *encoder = config_get_string(main->Config(), "SimpleOutput", "RecEncoder");
	const char *audio_encoder = config_get_string(main->Config(), "SimpleOutput", "RecAudioEncoder");

	videoEncoder = encoder;
	videoQuality = quality;
	ffmpegOutput = false;

	if (strcmp(quality, "Stream") == 0) {
		videoRecording = videoStreaming;
		audioRecording = audioStreaming;
		usingRecordingPreset = false;
		return;

	} else if (strcmp(quality, "Lossless") == 0) {
		LoadRecordingPreset_Lossless();
		usingRecordingPreset = true;
		ffmpegOutput = true;
		return;

	} else {
		lowCPUx264 = false;

		if (strcmp(encoder, SIMPLE_ENCODER_X264_LOWCPU) == 0)
			lowCPUx264 = true;
		LoadRecordingPreset_Lossy(get_simple_output_encoder(encoder));
		usingRecordingPreset = true;

		bool success = false;

		if (strcmp(audio_encoder, "opus") == 0)
			success = CreateSimpleOpusEncoder(audioRecording, 192, "simple_opus_recording", 0);
		else
			success = CreateSimpleAACEncoder(audioRecording, 192, "simple_aac_recording", 0);

		if (!success)
			throw "Failed to create audio recording encoder "
			      "(simple output)";
		for (int i = 0; i < MAX_AUDIO_MIXES; i++) {
			char name[23];
			if (strcmp(audio_encoder, "opus") == 0) {
				snprintf(name, sizeof name, "simple_opus_recording%d", i);
				success = CreateSimpleOpusEncoder(audioTrack[i], GetAudioBitrate(), name, i);
			} else {
				snprintf(name, sizeof name, "simple_aac_recording%d", i);
				success = CreateSimpleAACEncoder(audioTrack[i], GetAudioBitrate(), name, i);
			}
			if (!success)
				throw "Failed to create multi-track audio recording encoder "
				      "(simple output)";
		}
	}
}

#define SIMPLE_ARCHIVE_NAME "simple_archive_audio"

SimpleOutput::SimpleOutput(OBSBasic *main_) : BasicOutputHandler(main_)
{
	const char *encoder = config_get_string(main->Config(), "SimpleOutput", "StreamEncoder");
	const char *audio_encoder = config_get_string(main->Config(), "SimpleOutput", "StreamAudioEncoder");

	LoadStreamingPreset_Lossy(get_simple_output_encoder(encoder));

	bool success = false;

	if (strcmp(audio_encoder, "opus") == 0)
		success = CreateSimpleOpusEncoder(audioStreaming, GetAudioBitrate(), "simple_opus", 0);
	else
		success = CreateSimpleAACEncoder(audioStreaming, GetAudioBitrate(), "simple_aac", 0);

	if (!success)
		throw "Failed to create audio streaming encoder (simple output)";

	if (strcmp(audio_encoder, "opus") == 0)
		success = CreateSimpleOpusEncoder(audioArchive, GetAudioBitrate(), SIMPLE_ARCHIVE_NAME, 1);
	else
		success = CreateSimpleAACEncoder(audioArchive, GetAudioBitrate(), SIMPLE_ARCHIVE_NAME, 1);

	if (!success)
		throw "Failed to create audio archive encoder (simple output)";

	// Vertical Stream Video Encoder
	// TODO: Add UI in OBSBasicSettings Simple Output for Service_V_Stream, Server_V_Stream, Key_V_Stream
	// For now, encoder settings are assumed to be in "Output" section with _V_Stream suffix
	const char *encoder_v = config_get_string(main->Config(), "Output", "StreamEncoder_V_Stream");
	if (encoder_v && *encoder_v) {
		videoStreaming_v = obs_video_encoder_create(get_simple_output_encoder(encoder_v), "simple_video_stream_v", nullptr, nullptr);
		if (!videoStreaming_v) {
			blog(LOG_WARNING, "Failed to create vertical video streaming encoder (simple output): %s", encoder_v);
			// Not throwing an error, as vertical stream might be optional or not fully configured.
		} else {
			obs_encoder_release(videoStreaming_v);
		}
	} else {
		blog(LOG_INFO, "Vertical stream encoder not configured in Simple Output mode.");
	}

	LoadRecordingPreset();

	if (!ffmpegOutput) {
		bool useReplayBuffer = config_get_bool(main->Config(), "SimpleOutput", "RecRB");
		const char *recFormat = config_get_string(main->Config(), "SimpleOutput", "RecFormat2");

		if (useReplayBuffer) {
			OBSDataAutoRelease hotkey;
			const char *str = config_get_string(main->Config(), "Hotkeys", "ReplayBuffer");
			if (str)
				hotkey = obs_data_create_from_json(str);
			else
				hotkey = nullptr;

			replayBuffer = obs_output_create("replay_buffer", Str("ReplayBuffer"), nullptr, hotkey);

			if (!replayBuffer)
				throw "Failed to create replay buffer output "
				      "(simple output)";

			signal_handler_t *signal = obs_output_get_signal_handler(replayBuffer);

			startReplayBuffer.Connect(signal, "start", OBSStartReplayBuffer, this);
			stopReplayBuffer.Connect(signal, "stop", OBSStopReplayBuffer, this);
			replayBufferStopping.Connect(signal, "stopping", OBSReplayBufferStopping, this);
			replayBufferSaved.Connect(signal, "saved", OBSReplayBufferSaved, this);
		}

		bool use_native = strcmp(recFormat, "hybrid_mp4") == 0;
		fileOutput = obs_output_create(use_native ? "mp4_output" : "ffmpeg_muxer", "simple_file_output",
					       nullptr, nullptr);
		if (!fileOutput)
			throw "Failed to create recording output "
			      "(simple output)";
	}

	startRecording.Connect(obs_output_get_signal_handler(fileOutput), "start", OBSStartRecording, this);
	stopRecording.Connect(obs_output_get_signal_handler(fileOutput), "stop", OBSStopRecording, this);
	recordStopping.Connect(obs_output_get_signal_handler(fileOutput), "stopping", OBSRecordStopping, this);
}

int SimpleOutput::GetAudioBitrate() const
{
	const char *audio_encoder = config_get_string(main->Config(), "SimpleOutput", "StreamAudioEncoder");
	int bitrate = (int)config_get_uint(main->Config(), "SimpleOutput", "ABitrate");

	if (strcmp(audio_encoder, "opus") == 0)
		return FindClosestAvailableSimpleOpusBitrate(bitrate);

	return FindClosestAvailableSimpleAACBitrate(bitrate);
}

void SimpleOutput::Update()
{
	OBSDataAutoRelease videoSettings = obs_data_create();
	OBSDataAutoRelease audioSettings = obs_data_create();

	int videoBitrate = config_get_uint(main->Config(), "SimpleOutput", "VBitrate");
	int audioBitrate = GetAudioBitrate();
	bool advanced = config_get_bool(main->Config(), "SimpleOutput", "UseAdvanced");
	bool enforceBitrate = !config_get_bool(main->Config(), "Stream1", "IgnoreRecommended");
	const char *custom = config_get_string(main->Config(), "SimpleOutput", "x264Settings");
	const char *encoder = config_get_string(main->Config(), "SimpleOutput", "StreamEncoder");
	const char *encoder_id = obs_encoder_get_id(videoStreaming);
	const char *presetType;
	const char *preset;

	if (strcmp(encoder, SIMPLE_ENCODER_QSV) == 0) {
		presetType = "QSVPreset";

	} else if (strcmp(encoder, SIMPLE_ENCODER_QSV_AV1) == 0) {
		presetType = "QSVPreset";

	} else if (strcmp(encoder, SIMPLE_ENCODER_AMD) == 0) {
		presetType = "AMDPreset";

#ifdef ENABLE_HEVC
	} else if (strcmp(encoder, SIMPLE_ENCODER_AMD_HEVC) == 0) {
		presetType = "AMDPreset";
#endif

	} else if (strcmp(encoder, SIMPLE_ENCODER_NVENC) == 0) {
		presetType = "NVENCPreset2";

#ifdef ENABLE_HEVC
	} else if (strcmp(encoder, SIMPLE_ENCODER_NVENC_HEVC) == 0) {
		presetType = "NVENCPreset2";
#endif

	} else if (strcmp(encoder, SIMPLE_ENCODER_AMD_AV1) == 0) {
		presetType = "AMDAV1Preset";

	} else if (strcmp(encoder, SIMPLE_ENCODER_NVENC_AV1) == 0) {
		presetType = "NVENCPreset2";

	} else {
		presetType = "Preset";
	}

	preset = config_get_string(main->Config(), "SimpleOutput", presetType);

	/* Only use preset2 for legacy/FFmpeg NVENC Encoder. */
	if (strncmp(encoder_id, "ffmpeg_", 7) == 0 && strcmp(presetType, "NVENCPreset2") == 0) {
		obs_data_set_string(videoSettings, "preset2", preset);
	} else {
		obs_data_set_string(videoSettings, "preset", preset);
	}

	obs_data_set_string(videoSettings, "rate_control", "CBR");
	obs_data_set_int(videoSettings, "bitrate", videoBitrate);

	if (advanced)
		obs_data_set_string(videoSettings, "x264opts", custom);

	obs_data_set_string(audioSettings, "rate_control", "CBR");
	obs_data_set_int(audioSettings, "bitrate", audioBitrate);

	obs_service_apply_encoder_settings(main->GetService(), videoSettings, audioSettings);

	if (!enforceBitrate) {
		int maxVideoBitrate;
		int maxAudioBitrate;
		obs_service_get_max_bitrate(main->GetService(), &maxVideoBitrate, &maxAudioBitrate);

		std::string videoBitrateLogString = maxVideoBitrate > 0 ? std::to_string(maxVideoBitrate) : "None";
		std::string audioBitrateLogString = maxAudioBitrate > 0 ? std::to_string(maxAudioBitrate) : "None";

		blog(LOG_INFO,
		     "User is ignoring service bitrate limits.\n"
		     "Service Recommendations:\n"
		     "\tvideo bitrate: %s\n"
		     "\taudio bitrate: %s",
		     videoBitrateLogString.c_str(), audioBitrateLogString.c_str());

		obs_data_set_int(videoSettings, "bitrate", videoBitrate);
		obs_data_set_int(audioSettings, "bitrate", audioBitrate);
	}

	video_t *video = obs_get_video();
	enum video_format format = video_output_get_format(video);

	switch (format) {
	case VIDEO_FORMAT_I420:
	case VIDEO_FORMAT_NV12:
	case VIDEO_FORMAT_I010:
	case VIDEO_FORMAT_P010:
		break;
	default:
		obs_encoder_set_preferred_video_format(videoStreaming, VIDEO_FORMAT_NV12);
	}

	obs_encoder_update(videoStreaming, videoSettings);
	obs_encoder_update(audioStreaming, audioSettings);
	obs_encoder_update(audioArchive, audioSettings);

	// Update Vertical Video Stream Encoder if it exists
	if (videoStreaming_v) {
		OBSDataAutoRelease videoSettings_v = obs_data_create();
		int videoBitrate_v = config_get_uint(main->Config(), "Output", "VBitrate_V_Stream");
		bool advanced_v = config_get_bool(main->Config(), "Output", "UseAdvanced_V_Stream");
		const char *custom_v = config_get_string(main->Config(), "Output", "StreamCustom_V_Stream");
		const char *encoder_v_cfg = config_get_string(main->Config(), "Output", "StreamEncoder_V_Stream");
		const char *encoder_id_v = obs_encoder_get_id(videoStreaming_v); // Use actual current encoder ID
		const char *presetType_v;
		const char *preset_v;

		if (strcmp(encoder_v_cfg, SIMPLE_ENCODER_QSV) == 0 || strcmp(encoder_v_cfg, SIMPLE_ENCODER_QSV_AV1) == 0) {
			presetType_v = "QSVPreset_V_Stream"; // Assuming specific config keys for vertical presets
		} else if (strcmp(encoder_v_cfg, SIMPLE_ENCODER_AMD) == 0 || strcmp(encoder_v_cfg, SIMPLE_ENCODER_AMD_HEVC) == 0 || strcmp(encoder_v_cfg, SIMPLE_ENCODER_AMD_AV1) == 0) {
			presetType_v = "AMDPreset_V_Stream";
		} else if (strcmp(encoder_v_cfg, SIMPLE_ENCODER_NVENC) == 0 || strcmp(encoder_v_cfg, SIMPLE_ENCODER_NVENC_HEVC) == 0 || strcmp(encoder_v_cfg, SIMPLE_ENCODER_NVENC_AV1) == 0) {
			presetType_v = "NVENCPreset2_V_Stream";
		} else {
			presetType_v = "Preset_V_Stream";
		}
		preset_v = config_get_string(main->Config(), "Output", presetType_v);
		// Fallback if specific vertical preset key doesn't exist
		if (!preset_v || !*preset_v) preset_v = config_get_string(main->Config(), "Output", config_get_string(main->Config(), "Output", "Preset_V_Stream"));


		if (strncmp(encoder_id_v, "ffmpeg_", 7) == 0 && (strcmp(presetType_v, "NVENCPreset2_V_Stream") == 0 || strcmp(presetType_v, "NVENCPreset2") == 0)) {
			obs_data_set_string(videoSettings_v, "preset2", preset_v);
		} else {
			obs_data_set_string(videoSettings_v, "preset", preset_v);
		}

		const char* tune_v = config_get_string(main->Config(), "Output", "Tune_V_Stream");
		if (tune_v && *tune_v && strcmp(tune_v, Str("None")) != 0) {
			obs_data_set_string(videoSettings_v, "tune", tune_v);
		}

		obs_data_set_string(videoSettings_v, "rate_control", "CBR");
		obs_data_set_int(videoSettings_v, "bitrate", videoBitrate_v);

		if (advanced_v) {
			obs_data_set_string(videoSettings_v, "x264opts", custom_v);
		}

		// Apply service settings for vertical stream - this might need a separate vertical service object
		// For now, assuming same service or that service settings are generic enough
		// obs_service_apply_encoder_settings(main->GetService(), videoSettings_v, audioSettings); // audioSettings is shared

		// TODO: Handle preferred video format for vertical encoder if different
		// enum video_format format_v = App()->GetVerticalVideoInfo()->output_format; (conceptual)
		// obs_encoder_set_preferred_video_format(videoStreaming_v, format_v);

		obs_encoder_update(videoStreaming_v, videoSettings_v);
		blog(LOG_INFO, "Updated vertical simple stream encoder settings.");
	}
}

void SimpleOutput::UpdateRecordingAudioSettings()
{
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_int(settings, "bitrate", 192);
	obs_data_set_string(settings, "rate_control", "CBR");

	int tracks = config_get_int(main->Config(), "SimpleOutput", "RecTracks");
	const char *recFormat = config_get_string(main->Config(), "SimpleOutput", "RecFormat2");
	const char *quality = config_get_string(main->Config(), "SimpleOutput", "RecQuality");
	bool flv = strcmp(recFormat, "flv") == 0;

	if (flv || strcmp(quality, "Stream") == 0) {
		obs_encoder_update(audioRecording, settings);
	} else {
		for (int i = 0; i < MAX_AUDIO_MIXES; i++) {
			if ((tracks & (1 << i)) != 0) {
				obs_encoder_update(audioTrack[i], settings);
			}
		}
	}
}

#define CROSS_DIST_CUTOFF 2000.0

int SimpleOutput::CalcCRF(int crf)
{
	int cx = config_get_uint(main->Config(), "Video", "OutputCX");
	int cy = config_get_uint(main->Config(), "Video", "OutputCY");
	double fCX = double(cx);
	double fCY = double(cy);

	if (lowCPUx264)
		crf -= 2;

	double crossDist = sqrt(fCX * fCX + fCY * fCY);
	double crfResReduction = fmin(CROSS_DIST_CUTOFF, crossDist) / CROSS_DIST_CUTOFF;
	crfResReduction = (1.0 - crfResReduction) * 10.0;

	return crf - int(crfResReduction);
}

void SimpleOutput::UpdateRecordingSettings_x264_crf(int crf)
{
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_int(settings, "crf", crf);
	obs_data_set_bool(settings, "use_bufsize", true);
	obs_data_set_string(settings, "rate_control", "CRF");
	obs_data_set_string(settings, "profile", "high");
	obs_data_set_string(settings, "preset", lowCPUx264 ? "ultrafast" : "veryfast");

	obs_encoder_update(videoRecording, settings);
}

static bool icq_available(obs_encoder_t *encoder)
{
	obs_properties_t *props = obs_encoder_properties(encoder);
	obs_property_t *p = obs_properties_get(props, "rate_control");
	bool icq_found = false;

	size_t num = obs_property_list_item_count(p);
	for (size_t i = 0; i < num; i++) {
		const char *val = obs_property_list_item_string(p, i);
		if (strcmp(val, "ICQ") == 0) {
			icq_found = true;
			break;
		}
	}

	obs_properties_destroy(props);
	return icq_found;
}

void SimpleOutput::UpdateRecordingSettings_qsv11(int crf, bool av1)
{
	bool icq = icq_available(videoRecording);

	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "profile", "high");

	if (icq && !av1) {
		obs_data_set_string(settings, "rate_control", "ICQ");
		obs_data_set_int(settings, "icq_quality", crf);
	} else {
		obs_data_set_string(settings, "rate_control", "CQP");
		obs_data_set_int(settings, "cqp", crf);
	}

	obs_encoder_update(videoRecording, settings);
}

void SimpleOutput::UpdateRecordingSettings_nvenc(int cqp)
{
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "rate_control", "CQP");
	obs_data_set_string(settings, "profile", "high");
	obs_data_set_int(settings, "cqp", cqp);

	obs_encoder_update(videoRecording, settings);
}

void SimpleOutput::UpdateRecordingSettings_nvenc_hevc_av1(int cqp)
{
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "rate_control", "CQP");
	obs_data_set_string(settings, "profile", "main");
	obs_data_set_int(settings, "cqp", cqp);

	obs_encoder_update(videoRecording, settings);
}

void SimpleOutput::UpdateRecordingSettings_apple(int quality)
{
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "rate_control", "CRF");
	obs_data_set_string(settings, "profile", "high");
	obs_data_set_int(settings, "quality", quality);

	obs_encoder_update(videoRecording, settings);
}

#ifdef ENABLE_HEVC
void SimpleOutput::UpdateRecordingSettings_apple_hevc(int quality)
{
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "rate_control", "CRF");
	obs_data_set_string(settings, "profile", "main");
	obs_data_set_int(settings, "quality", quality);

	obs_encoder_update(videoRecording, settings);
}
#endif

void SimpleOutput::UpdateRecordingSettings_amd_cqp(int cqp)
{
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "rate_control", "CQP");
	obs_data_set_string(settings, "profile", "high");
	obs_data_set_string(settings, "preset", "quality");
	obs_data_set_int(settings, "cqp", cqp);
	obs_encoder_update(videoRecording, settings);
}

void SimpleOutput::UpdateRecordingSettings()
{
	bool ultra_hq = (videoQuality == "HQ");
	int crf = CalcCRF(ultra_hq ? 16 : 23);

	if (astrcmp_n(videoEncoder.c_str(), "x264", 4) == 0) {
		UpdateRecordingSettings_x264_crf(crf);

	} else if (videoEncoder == SIMPLE_ENCODER_QSV) {
		UpdateRecordingSettings_qsv11(crf, false);

	} else if (videoEncoder == SIMPLE_ENCODER_QSV_AV1) {
		UpdateRecordingSettings_qsv11(crf, true);

	} else if (videoEncoder == SIMPLE_ENCODER_AMD) {
		UpdateRecordingSettings_amd_cqp(crf);

#ifdef ENABLE_HEVC
	} else if (videoEncoder == SIMPLE_ENCODER_AMD_HEVC) {
		UpdateRecordingSettings_amd_cqp(crf);
#endif

	} else if (videoEncoder == SIMPLE_ENCODER_AMD_AV1) {
		UpdateRecordingSettings_amd_cqp(crf);

	} else if (videoEncoder == SIMPLE_ENCODER_NVENC) {
		UpdateRecordingSettings_nvenc(crf);

#ifdef ENABLE_HEVC
	} else if (videoEncoder == SIMPLE_ENCODER_NVENC_HEVC) {
		UpdateRecordingSettings_nvenc_hevc_av1(crf);
#endif
	} else if (videoEncoder == SIMPLE_ENCODER_NVENC_AV1) {
		UpdateRecordingSettings_nvenc_hevc_av1(crf);

	} else if (videoEncoder == SIMPLE_ENCODER_APPLE_H264) {
		/* These are magic numbers. 0 - 100, more is better. */
		UpdateRecordingSettings_apple(ultra_hq ? 70 : 50);
#ifdef ENABLE_HEVC
	} else if (videoEncoder == SIMPLE_ENCODER_APPLE_HEVC) {
		UpdateRecordingSettings_apple_hevc(ultra_hq ? 70 : 50);
#endif
	}
	UpdateRecordingAudioSettings();
}

inline void SimpleOutput::SetupOutputs()
{
	SimpleOutput::Update();
	obs_encoder_set_video(videoStreaming, obs_get_video());
	obs_encoder_set_audio(audioStreaming, obs_get_audio());
	obs_encoder_set_audio(audioArchive, obs_get_audio());

	if (App()->IsDualOutputActive() && videoStreaming_v) {
		obs_encoder_set_video(videoStreaming_v, obs_get_video());
		// Audio for verticalStreamOutput will be set in OBSApp::SetupOutputs,
		// typically using audioStreaming or audioArchive.
		blog(LOG_INFO, "SimpleOutput::SetupOutputs - Configured video for vertical stream encoder.");
	}

	int tracks = config_get_int(main->Config(), "SimpleOutput", "RecTracks");
	const char *recFormat = config_get_string(main->Config(), "SimpleOutput", "RecFormat2");
	bool flv = strcmp(recFormat, "flv") == 0;
	const char *quality = config_get_string(main->Config(), "SimpleOutput", "RecQuality");

	if (usingRecordingPreset) {
		if (ffmpegOutput) {
			obs_output_set_media(fileOutput, obs_get_video(), obs_get_audio());
		} else {
			obs_encoder_set_video(videoRecording, obs_get_video());
			if (flv) {
				obs_encoder_set_audio(audioRecording, obs_get_audio());
			} else {
				for (int i = 0; i < MAX_AUDIO_MIXES; i++) {
					if ((tracks & (1 << i)) != 0) {
						obs_encoder_set_audio(audioTrack[i], obs_get_audio());
					}
				}
			}
		}
	} else {
		obs_encoder_set_audio(audioRecording, obs_get_audio());
	}
}

std::shared_future<void> SimpleOutput::SetupStreaming(obs_service_t *service, SetupStreamingContinuation_t continuation)
{
	if (!Active())
		SetupOutputs();

	Auth *auth = main->GetAuth();
	if (auth)
		auth->OnStreamConfig();

	/* --------------------- */

	const char *type = GetStreamOutputType(service);
	if (!type) {
		continuation(false);
		return StartMultitrackVideoStreamingGuard::MakeReadyFuture();
	}

	auto audio_bitrate = GetAudioBitrate();
	auto vod_track_mixer = IsVodTrackEnabled(service) ? std::optional{1} : std::nullopt;

	auto handle_multitrack_video_result = [=](std::optional<bool> multitrackVideoResult) {
		if (multitrackVideoResult.has_value())
			return multitrackVideoResult.value();

		/* XXX: this is messy and disgusting and should be refactored */
		if (outputType != type) {
			streamDelayStarting.Disconnect();
			streamStopping.Disconnect();
			startStreaming.Disconnect();
			stopStreaming.Disconnect();

			streamOutput = obs_output_create(type, "simple_stream", nullptr, nullptr);
			if (!streamOutput) {
				blog(LOG_WARNING,
				     "Creation of stream output type '%s' "
				     "failed!",
				     type);
				return false;
			}

			streamDelayStarting.Connect(obs_output_get_signal_handler(streamOutput), "starting",
						    OBSStreamStarting, this);
			streamStopping.Connect(obs_output_get_signal_handler(streamOutput), "stopping",
					       OBSStreamStopping, this);

			startStreaming.Connect(obs_output_get_signal_handler(streamOutput), "start", OBSStartStreaming,
					       this);
			stopStreaming.Connect(obs_output_get_signal_handler(streamOutput), "stop", OBSStopStreaming,
					      this);

			outputType = type;
		}

		obs_output_set_video_encoder(streamOutput, videoStreaming);
		obs_output_set_audio_encoder(streamOutput, audioStreaming, 0);
		obs_output_set_service(streamOutput, service);
		return true;
	};

	return SetupMultitrackVideo(service, GetSimpleAACEncoderForBitrate(audio_bitrate), 0, vod_track_mixer,
				    [=](std::optional<bool> res) {
					    continuation(handle_multitrack_video_result(res));
				    });
}

bool SimpleOutput::IsVodTrackEnabled(obs_service_t *service)
{
	bool advanced = config_get_bool(main->Config(), "SimpleOutput", "UseAdvanced");
	bool enable = config_get_bool(main->Config(), "SimpleOutput", "VodTrackEnabled");
	bool enableForCustomServer = config_get_bool(App()->GetUserConfig(), "General", "EnableCustomServerVodTrack");

	OBSDataAutoRelease settings = obs_service_get_settings(service);
	const char *name = obs_data_get_string(settings, "service");

	const char *id = obs_service_get_id(service);
	if (strcmp(id, "rtmp_custom") == 0)
		return enableForCustomServer ? enable : false;
	else
		return advanced && enable && ServiceSupportsVodTrack(name);
}

void SimpleOutput::SetupVodTrack(obs_service_t *service)
{
	if (IsVodTrackEnabled(service))
		obs_output_set_audio_encoder(streamOutput, audioArchive, 1);
	else
		clear_archive_encoder(streamOutput, SIMPLE_ARCHIVE_NAME);
}

// Replace existing SimpleOutput::StartStreaming
bool SimpleOutput::StartStreaming(obs_service_t *service)
{
	bool reconnect = config_get_bool(main->Config(), "Output", "Reconnect");
	int retryDelay = config_get_uint(main->Config(), "Output", "RetryDelay");
	int maxRetries = config_get_uint(main->Config(), "Output", "MaxRetries");
	bool useDelay = config_get_bool(main->Config(), "Output", "DelayEnable");
	int delaySec = config_get_int(main->Config(), "Output", "DelaySec");
	bool preserveDelay = config_get_bool(main->Config(), "Output", "DelayPreserve");
	const char *bindIP = config_get_string(main->Config(), "Output", "BindIP");
	const char *ipFamily = config_get_string(main->Config(), "Output", "IPFamily");
#ifdef _WIN32
	bool enableNewSocketLoop = config_get_bool(main->Config(), "Output", "NewSocketLoopEnable");
	bool enableLowLatencyMode = config_get_bool(main->Config(), "Output", "LowLatencyEnable");
#endif
	bool enableDynBitrate = config_get_bool(main->Config(), "Output", "DynamicBitrate");

	if (multitrackVideo && multitrackVideoActive &&
	    !multitrackVideo->HandleIncompatibleSettings(main, main->Config(), service, enableDynBitrate)) {
		multitrackVideoActive = false;
		return false;
	}

	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "bind_ip", bindIP);
	obs_data_set_string(settings, "ip_family", ipFamily);
#ifdef _WIN32
	obs_data_set_bool(settings, "new_socket_loop_enabled", enableNewSocketLoop);
	obs_data_set_bool(settings, "low_latency_mode_enabled", enableLowLatencyMode);
#endif
	obs_data_set_bool(settings, "dyn_bitrate", enableDynBitrate);

	// Horizontal Stream Output
	OBSOutputAutoRelease h_stream_output = StreamingOutput(); // This is effectively 'this->streamOutput' or from multitrack
	if (h_stream_output) {
		obs_output_update(h_stream_output, settings);
		if (!reconnect) maxRetries = 0;
		obs_output_set_delay(h_stream_output, useDelay ? delaySec : 0, preserveDelay ? OBS_OUTPUT_DELAY_PRESERVE : 0);
		obs_output_set_reconnect_settings(h_stream_output, maxRetries, retryDelay);
		if (!multitrackVideo || !multitrackVideoActive) {
			SetupVodTrack(service); // Setup VOD track for horizontal output if not multitrack
		}
	} else {
		blog(LOG_WARNING, "Horizontal stream output (SimpleOutput) is not available for configuration.");
	}

	bool horizontalStarted = false;
	if (h_stream_output && obs_output_start(h_stream_output)) {
		if (multitrackVideo && multitrackVideoActive)
			multitrackVideo->StartedStreaming();
		horizontalStarted = true;
		blog(LOG_INFO, "Horizontal stream started successfully (SimpleOutput).");
	} else {
		if (multitrackVideo && multitrackVideoActive)
			multitrackVideoActive = false;
		const char *error = h_stream_output ? obs_output_get_last_error(h_stream_output) : "Output not available";
		if (error && *error) lastError = error; else lastError.assign("Horizontal output failed to start or not available.");
		blog(LOG_WARNING, "Horizontal stream output failed to start! %s", lastError.c_str());
	}

	// Vertical Stream Output
	bool verticalAttempted = false;
	bool verticalSuccessfullyStarted = false;
	if (App()->IsDualOutputActive() && this->verticalStreamOutput && videoStreaming_v) {
		verticalAttempted = true;
		blog(LOG_INFO, "Attempting to start vertical stream (SimpleOutput)...");

		// Ensure the video encoder configured by SimpleOutput::Update() is set on the output
		obs_output_set_video_encoder(this->verticalStreamOutput, videoStreaming_v);
		// Audio encoder for vertical stream is handled by the service using settings from OBSApp::SetupOutputs,
		// or directly on the output if it's a muxing type not using a separate service audio encoder.
		// SimpleOutput doesn't manage a separate audio encoder instance for vertical to set here.

		// TODO: Apply vertical-specific delay, reconnect, and network settings if they are added to UI and config
		// OBSDataAutoRelease settings_v = obs_data_create();
		// obs_data_set_string(settings_v, "bind_ip", config_get_string(main->Config(), "Output", "BindIP_V_Stream"));
		// ... etc. ...
		// obs_output_update(this->verticalStreamOutput, settings_v);
		// obs_output_set_delay(this->verticalStreamOutput, ...);
		// obs_output_set_reconnect_settings(this->verticalStreamOutput, ...);
		// For now, vertical stream uses default output settings or those applied during its creation in OBSApp::SetupOutputs

		if (obs_output_start(this->verticalStreamOutput)) {
			blog(LOG_INFO, "Vertical stream started successfully (SimpleOutput).");
			verticalSuccessfullyStarted = true;
		} else {
			const char *error_v = obs_output_get_last_error(this->verticalStreamOutput);
			blog(LOG_WARNING, "Vertical stream output failed to start! %s", error_v ? error_v : "Unknown error");
			if (!horizontalStarted || (error_v && *error_v)) {
				if(!lastError.empty() && lastError.back() != ' ') lastError += "; ";
				lastError += "VerticalS: ";
				lastError += (error_v && *error_v) ? error_v : "Unknown error";
			}
			verticalSuccessfullyStarted = false;
		}
	} else if (App()->IsDualOutputActive()) {
		if (!this->verticalStreamOutput) blog(LOG_INFO, "Dual output active, but BasicOutputHandler::verticalStreamOutput is not set for SimpleOutput (cannot start vertical stream).");
		if (!videoStreaming_v) blog(LOG_INFO, "Dual output active, but SimpleOutput::videoStreaming_v (encoder) is not set (cannot start vertical stream).");
	}

	// Determine overall success
	if (horizontalStarted) return true; // If horizontal started, primary goal met.
	if (App()->IsDualOutputActive() && verticalAttempted) return verticalSuccessfullyStarted; // If dual and vertical was tried, its success matters.
	if (App()->IsDualOutputActive() && !verticalAttempted && !horizontalStarted) return false; // Dual active, H failed, V not attempted -> total fail.


	// If single output mode and horizontal failed.
	const char *type = h_stream_output ? obs_output_get_id(h_stream_output) : "N/A";
	blog(LOG_WARNING, "Stream output type '%s' (SimpleOutput) failed to start! Last Error: %s", type, lastError.c_str());
	return false;
}

// Replace existing SimpleOutput::StopStreaming
void SimpleOutput::StopStreaming(bool force)
{
	OBSOutputAutoRelease h_output = StreamingOutput(); // Horizontal output (could be multitrack)
	if (h_output) {
		blog(LOG_INFO, "Stopping horizontal stream (SimpleOutput)...");
		if (force) obs_output_force_stop(h_output);
		else obs_output_stop(h_output);
	}

	if (this->verticalStreamOutput && obs_output_active(this->verticalStreamOutput)) {
		blog(LOG_INFO, "Stopping vertical stream (SimpleOutput)...");
		if (force) obs_output_force_stop(this->verticalStreamOutput);
		else obs_output_stop(this->verticalStreamOutput);
	}

	// multitrackVideoActive is reset by the stop signal handler for the multitrack output
}

bool SimpleOutput::StartRecording()
{
	UpdateRecording();
	if (!ConfigureRecording(false))
		return false;
	if (!obs_output_start(fileOutput)) {
		QString error_reason;
		const char *error = obs_output_get_last_error(fileOutput);
		if (error)
			error_reason = QT_UTF8(error);
		else
			error_reason = QTStr("Output.StartFailedGeneric");
		QMessageBox::critical(main, QTStr("Output.StartRecordingFailed"), error_reason);
		return false;
	}

	return true;
}

bool SimpleOutput::StartReplayBuffer()
{
	UpdateRecording();
	if (!ConfigureRecording(true))
		return false;
	if (!obs_output_start(replayBuffer)) {
		QMessageBox::critical(main, QTStr("Output.StartReplayFailed"), QTStr("Output.StartFailedGeneric"));
		return false;
	}

	return true;
}


void SimpleOutput::StopRecording(bool force)
{
	if (force)
		obs_output_force_stop(fileOutput);
	else
		obs_output_stop(fileOutput);
}

void SimpleOutput::StopReplayBuffer(bool force)
{
	if (force)
		obs_output_force_stop(replayBuffer);
	else
		obs_output_stop(replayBuffer);
}

bool SimpleOutput::StreamingActive() const
{
	bool h_active = obs_output_active(StreamingOutput()); // Horizontal or multitrack
	bool v_active = VerticalStreamingActive();
	return h_active || v_active;
}

bool SimpleOutput::RecordingActive() const
{
	return obs_output_active(fileOutput);
}

bool SimpleOutput::ReplayBufferActive() const
{
	return obs_output_active(replayBuffer);
}

// Add these new functions or replace if stubs exist

bool SimpleOutput::StartVerticalStreaming(obs_service_t *service)
{
	if (!App()->IsDualOutputActive() || !this->verticalStreamOutput || !videoStreaming_v) {
		blog(LOG_WARNING, "Vertical stream cannot be started: Not in dual output mode or vertical output/encoder not configured (SimpleOutput).");
		return false;
	}

	blog(LOG_INFO, "Attempting to start ONLY vertical stream (SimpleOutput)...");

	// TODO: Apply vertical-specific service, delay, reconnect, and network settings
	// This would mirror parts of StartStreaming but target verticalStreamOutput and vertical-specific configs
	// For now, assumes verticalStreamOutput is pre-configured by OBSApp::SetupOutputs with its service
	// and uses default/shared network settings.

	// Example of setting service if it were managed here:
	// if (service) { // A specific service for vertical stream
	//     obs_output_set_service(this->verticalStreamOutput, service);
	// } else {
	//     blog(LOG_WARNING, "No service provided for StartVerticalStreaming.");
	//     return false;
	// }

	if (obs_output_start(this->verticalStreamOutput)) {
		blog(LOG_INFO, "Vertical stream started successfully via StartVerticalStreaming (SimpleOutput).");
		return true;
	} else {
		const char *error_v = obs_output_get_last_error(this->verticalStreamOutput);
		lastError = "VerticalS StartOnly: ";
		lastError += (error_v && *error_v) ? error_v : "Unknown error";
		blog(LOG_WARNING, "Vertical stream output (StartOnly) failed to start! %s", lastError.c_str());
		return false;
	}
}

void SimpleOutput::StopVerticalStreaming(bool force)
{
	if (this->verticalStreamOutput && obs_output_active(this->verticalStreamOutput)) {
		blog(LOG_INFO, "Stopping ONLY vertical stream (SimpleOutput)...");
		if (force) obs_output_force_stop(this->verticalStreamOutput);
		else obs_output_stop(this->verticalStreamOutput);
	} else {
		blog(LOG_INFO, "StopVerticalStreaming called but vertical stream not active or not configured (SimpleOutput).");
	}
}

bool SimpleOutput::VerticalStreamingActive() const
{
	return this->verticalStreamOutput && obs_output_active(this->verticalStreamOutput);
}
