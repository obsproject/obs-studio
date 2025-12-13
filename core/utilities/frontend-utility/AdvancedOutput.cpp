#include "AdvancedOutput.hpp"

#include <utility/audio-encoders.hpp>
#include <utility/StartMultiTrackVideoStreamingGuard.hpp>
#include <widgets/OBSBasic.hpp>

#include <qt-wrappers.hpp>

using namespace std;

static OBSData GetDataFromJsonFile(const char *jsonFile)
{
	const OBSBasic *basic = OBSBasic::Get();

	const OBSProfile &currentProfile = basic->GetCurrentProfile();

	const std::filesystem::path jsonFilePath = currentProfile.path / std::filesystem::u8path(jsonFile);

	OBSDataAutoRelease data = nullptr;

	if (!jsonFilePath.empty()) {
		BPtr<char> jsonData = os_quick_read_utf8_file(jsonFilePath.u8string().c_str());

		if (!!jsonData) {
			data = obs_data_create_from_json(jsonData);
		}
	}

	if (!data) {
		data = obs_data_create();
	}

	return data.Get();
}

static void ApplyEncoderDefaults(OBSData &settings, const obs_encoder_t *encoder)
{
	OBSData dataRet = obs_encoder_get_defaults(encoder);
	obs_data_release(dataRet);

	if (!!settings)
		obs_data_apply(dataRet, settings);
	settings = std::move(dataRet);
}

#define ADV_ARCHIVE_NAME "adv_archive_audio"

#ifdef __APPLE__
static void translate_macvth264_encoder(const char *&encoder)
{
	if (strcmp(encoder, "vt_h264_hw") == 0) {
		encoder = "com.apple.videotoolbox.videoencoder.h264.gva";
	} else if (strcmp(encoder, "vt_h264_sw") == 0) {
		encoder = "com.apple.videotoolbox.videoencoder.h264";
	}
}
#endif

AdvancedOutput::AdvancedOutput(OBSBasic *main_) : BasicOutputHandler(main_)
{
	const char *recType = config_get_string(main->Config(), "AdvOut", "RecType");
	const char *streamEncoder = config_get_string(main->Config(), "AdvOut", "Encoder");
	const char *streamAudioEncoder = config_get_string(main->Config(), "AdvOut", "AudioEncoder");
	const char *recordEncoder = config_get_string(main->Config(), "AdvOut", "RecEncoder");
	const char *recAudioEncoder = config_get_string(main->Config(), "AdvOut", "RecAudioEncoder");
	const char *recFormat = config_get_string(main->Config(), "AdvOut", "RecFormat2");
#ifdef __APPLE__
	translate_macvth264_encoder(streamEncoder);
	translate_macvth264_encoder(recordEncoder);
#endif

	ffmpegOutput = astrcmpi(recType, "FFmpeg") == 0;
	ffmpegRecording = ffmpegOutput && config_get_bool(main->Config(), "AdvOut", "FFOutputToFile");
	useStreamEncoder = astrcmpi(recordEncoder, "none") == 0;
	useStreamAudioEncoder = astrcmpi(recAudioEncoder, "none") == 0;

	OBSData streamEncSettings = GetDataFromJsonFile("streamEncoder.json");
	OBSData recordEncSettings = GetDataFromJsonFile("recordEncoder.json");

	if (ffmpegOutput) {
		fileOutput = obs_output_create("ffmpeg_output", "adv_ffmpeg_output", nullptr, nullptr);
		if (!fileOutput)
			throw "Failed to create recording FFmpeg output "
			      "(advanced output)";
	} else {
		bool useReplayBuffer = config_get_bool(main->Config(), "AdvOut", "RecRB");
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

		const char *mux = "ffmpeg_muxer";
		if (strcmp(recFormat, "hybrid_mp4") == 0)
			mux = "mp4_output";
		else if (strcmp(recFormat, "hybrid_mov") == 0)
			mux = "mov_output";

		fileOutput = obs_output_create(mux, "adv_file_output", nullptr, nullptr);
		if (!fileOutput)
			throw "Failed to create recording output "
			      "(advanced output)";

		if (!useStreamEncoder) {
			videoRecording = obs_video_encoder_create(recordEncoder, "advanced_video_recording",
								  recordEncSettings, nullptr);
			if (!videoRecording)
				throw "Failed to create recording video "
				      "encoder (advanced output)";
			obs_encoder_release(videoRecording);
		}
	}

	videoStreaming = obs_video_encoder_create(streamEncoder, "advanced_video_stream", streamEncSettings, nullptr);
	if (!videoStreaming)
		throw "Failed to create streaming video encoder "
		      "(advanced output)";
	obs_encoder_release(videoStreaming);

	const char *rate_control =
		obs_data_get_string(useStreamEncoder ? streamEncSettings : recordEncSettings, "rate_control");
	if (!rate_control)
		rate_control = "";
	usesBitrate = astrcmpi(rate_control, "CBR") == 0 || astrcmpi(rate_control, "VBR") == 0 ||
		      astrcmpi(rate_control, "ABR") == 0;

	for (int i = 0; i < MAX_AUDIO_MIXES; i++) {
		char name[19];
		snprintf(name, sizeof(name), "adv_record_audio_%d", i);

		recordTrack[i] = obs_audio_encoder_create(useStreamAudioEncoder ? streamAudioEncoder : recAudioEncoder,
							  name, nullptr, i, nullptr);

		if (!recordTrack[i]) {
			throw "Failed to create audio encoder "
			      "(advanced output)";
		}

		obs_encoder_release(recordTrack[i]);

		snprintf(name, sizeof(name), "adv_stream_audio_%d", i);
		streamTrack[i] = obs_audio_encoder_create(streamAudioEncoder, name, nullptr, i, nullptr);

		if (!streamTrack[i]) {
			throw "Failed to create streaming audio encoders "
			      "(advanced output)";
		}

		obs_encoder_release(streamTrack[i]);
	}

	std::string id;
	int streamTrackIndex = config_get_int(main->Config(), "AdvOut", "TrackIndex") - 1;
	streamAudioEnc =
		obs_audio_encoder_create(streamAudioEncoder, "adv_stream_audio", nullptr, streamTrackIndex, nullptr);
	if (!streamAudioEnc)
		throw "Failed to create streaming audio encoder "
		      "(advanced output)";
	obs_encoder_release(streamAudioEnc);

	id = "";
	int vodTrack = config_get_int(main->Config(), "AdvOut", "VodTrackIndex") - 1;
	streamArchiveEnc = obs_audio_encoder_create(streamAudioEncoder, ADV_ARCHIVE_NAME, nullptr, vodTrack, nullptr);
	if (!streamArchiveEnc)
		throw "Failed to create archive audio encoder "
		      "(advanced output)";
	obs_encoder_release(streamArchiveEnc);

	startRecording.Connect(obs_output_get_signal_handler(fileOutput), "start", OBSStartRecording, this);
	stopRecording.Connect(obs_output_get_signal_handler(fileOutput), "stop", OBSStopRecording, this);
	recordStopping.Connect(obs_output_get_signal_handler(fileOutput), "stopping", OBSRecordStopping, this);
	recordFileChanged.Connect(obs_output_get_signal_handler(fileOutput), "file_changed", OBSRecordFileChanged,
				  this);
}

void AdvancedOutput::UpdateStreamSettings()
{
	bool applyServiceSettings = config_get_bool(main->Config(), "AdvOut", "ApplyServiceSettings");
	bool enforceBitrate = !config_get_bool(main->Config(), "Stream1", "IgnoreRecommended");
	bool dynBitrate = config_get_bool(main->Config(), "Output", "DynamicBitrate");
	const char *streamEncoder = config_get_string(main->Config(), "AdvOut", "Encoder");

	OBSData settings = GetDataFromJsonFile("streamEncoder.json");
	ApplyEncoderDefaults(settings, videoStreaming);

	if (applyServiceSettings) {
		int bitrate = (int)obs_data_get_int(settings, "bitrate");
		int keyint_sec = (int)obs_data_get_int(settings, "keyint_sec");
		obs_service_apply_encoder_settings(main->GetService(), settings, nullptr);
		if (!enforceBitrate) {
			int maxVideoBitrate;
			int maxAudioBitrate;
			obs_service_get_max_bitrate(main->GetService(), &maxVideoBitrate, &maxAudioBitrate);

			std::string videoBitRateLogString = maxVideoBitrate > 0 ? std::to_string(maxVideoBitrate)
										: "None";
			std::string audioBitRateLogString = maxAudioBitrate > 0 ? std::to_string(maxAudioBitrate)
										: "None";

			blog(LOG_INFO,
			     "User is ignoring service bitrate limits.\n"
			     "Service Recommendations:\n"
			     "\tvideo bitrate: %s\n"
			     "\taudio bitrate: %s",
			     videoBitRateLogString.c_str(), audioBitRateLogString.c_str());

			obs_data_set_int(settings, "bitrate", bitrate);
		}

		int enforced_keyint_sec = (int)obs_data_get_int(settings, "keyint_sec");
		if (keyint_sec != 0 && keyint_sec < enforced_keyint_sec)
			obs_data_set_int(settings, "keyint_sec", keyint_sec);
	} else {
		blog(LOG_WARNING, "User is ignoring service settings.");
	}

	if (dynBitrate && strstr(streamEncoder, "nvenc") != nullptr)
		obs_data_set_bool(settings, "lookahead", false);

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

	obs_encoder_update(videoStreaming, settings);
}

inline void AdvancedOutput::UpdateRecordingSettings()
{
	OBSData settings = GetDataFromJsonFile("recordEncoder.json");
	obs_encoder_update(videoRecording, settings);
}

void AdvancedOutput::Update()
{
	UpdateStreamSettings();
	if (!useStreamEncoder && !ffmpegOutput)
		UpdateRecordingSettings();
	UpdateAudioSettings();
}

inline bool AdvancedOutput::allowsMultiTrack()
{
	const char *protocol = nullptr;
	obs_service_t *service_obj = main->GetService();
	protocol = obs_service_get_protocol(service_obj);
	if (!protocol)
		return false;
	return astrcmpi_n(protocol, SRT_PROTOCOL, strlen(SRT_PROTOCOL)) == 0 ||
	       astrcmpi_n(protocol, RIST_PROTOCOL, strlen(RIST_PROTOCOL)) == 0;
}

inline void AdvancedOutput::SetupStreaming()
{
	const char *rescaleRes = config_get_string(main->Config(), "AdvOut", "RescaleRes");
	int rescaleFilter = config_get_int(main->Config(), "AdvOut", "RescaleFilter");
	int multiTrackAudioMixes = config_get_int(main->Config(), "AdvOut", "StreamMultiTrackAudioMixes");
	unsigned int cx = 0;
	unsigned int cy = 0;
	int idx = 0;
	bool is_multitrack_output = allowsMultiTrack();

	if (rescaleFilter != OBS_SCALE_DISABLE && rescaleRes && *rescaleRes) {
		if (sscanf(rescaleRes, "%ux%u", &cx, &cy) != 2) {
			cx = 0;
			cy = 0;
		}
	}

	if (!is_multitrack_output) {
		obs_output_set_audio_encoder(streamOutput, streamAudioEnc, 0);
	} else {
		for (int i = 0; i < MAX_AUDIO_MIXES; i++) {
			if ((multiTrackAudioMixes & (1 << i)) != 0) {
				obs_output_set_audio_encoder(streamOutput, streamTrack[i], idx);
				idx++;
			}
		}
	}

	obs_encoder_set_scaled_size(videoStreaming, cx, cy);
	obs_encoder_set_gpu_scale_type(videoStreaming, (obs_scale_type)rescaleFilter);

	const char *id = obs_service_get_id(main->GetService());
	if (strcmp(id, "rtmp_custom") == 0) {
		OBSDataAutoRelease settings = obs_data_create();
		obs_service_apply_encoder_settings(main->GetService(), settings, nullptr);
		obs_encoder_update(videoStreaming, settings);
	}
}

inline void AdvancedOutput::SetupRecording()
{
	const char *path = config_get_string(main->Config(), "AdvOut", "RecFilePath");
	const char *mux = config_get_string(main->Config(), "AdvOut", "RecMuxerCustom");
	const char *rescaleRes = config_get_string(main->Config(), "AdvOut", "RecRescaleRes");
	int rescaleFilter = config_get_int(main->Config(), "AdvOut", "RecRescaleFilter");
	int tracks;

	const char *recFormat = config_get_string(main->Config(), "AdvOut", "RecFormat2");

	bool is_fragmented = strncmp(recFormat, "fragmented", 10) == 0;
	bool flv = strcmp(recFormat, "flv") == 0;

	if (flv)
		tracks = config_get_int(main->Config(), "AdvOut", "FLVTrack");
	else
		tracks = config_get_int(main->Config(), "AdvOut", "RecTracks");

	OBSDataAutoRelease settings = obs_data_create();
	unsigned int cx = 0;
	unsigned int cy = 0;
	int idx = 0;

	/* Hack to allow recordings without any audio tracks selected. It is no
	 * longer possible to select such a configuration in settings, but legacy
	 * configurations might still have this configured and we don't want to
	 * just break them. */
	if (tracks == 0)
		tracks = config_get_int(main->Config(), "AdvOut", "TrackIndex");

	if (useStreamEncoder) {
		obs_output_set_video_encoder(fileOutput, videoStreaming);
		if (replayBuffer)
			obs_output_set_video_encoder(replayBuffer, videoStreaming);
	} else {
		if (rescaleFilter != OBS_SCALE_DISABLE && rescaleRes && *rescaleRes) {
			if (sscanf(rescaleRes, "%ux%u", &cx, &cy) != 2) {
				cx = 0;
				cy = 0;
			}
		}

		obs_encoder_set_scaled_size(videoRecording, cx, cy);
		obs_encoder_set_gpu_scale_type(videoRecording, (obs_scale_type)rescaleFilter);
		obs_output_set_video_encoder(fileOutput, videoRecording);
		if (replayBuffer)
			obs_output_set_video_encoder(replayBuffer, videoRecording);
	}

	if (!flv) {
		for (int i = 0; i < MAX_AUDIO_MIXES; i++) {
			if ((tracks & (1 << i)) != 0) {
				obs_output_set_audio_encoder(fileOutput, recordTrack[i], idx);
				if (replayBuffer)
					obs_output_set_audio_encoder(replayBuffer, recordTrack[i], idx);
				idx++;
			}
		}
	} else if (flv && tracks != 0) {
		obs_output_set_audio_encoder(fileOutput, recordTrack[tracks - 1], idx);

		if (replayBuffer)
			obs_output_set_audio_encoder(replayBuffer, recordTrack[tracks - 1], idx);
	}

	// Use fragmented MOV/MP4 if user has not already specified custom movflags
	if (is_fragmented && (!mux || strstr(mux, "movflags") == NULL)) {
		string mux_frag = "movflags=frag_keyframe+empty_moov+delay_moov";
		if (mux) {
			mux_frag += " ";
			mux_frag += mux;
		}
		obs_data_set_string(settings, "muxer_settings", mux_frag.c_str());
	} else {
		if (is_fragmented)
			blog(LOG_WARNING, "User enabled fragmented recording, "
					  "but custom muxer settings contained movflags.");
		obs_data_set_string(settings, "muxer_settings", mux);
	}

	obs_data_set_string(settings, "path", path);
	obs_output_update(fileOutput, settings);
	if (replayBuffer)
		obs_output_update(replayBuffer, settings);
}

inline void AdvancedOutput::SetupFFmpeg()
{
	const char *url = config_get_string(main->Config(), "AdvOut", "FFURL");
	int vBitrate = config_get_int(main->Config(), "AdvOut", "FFVBitrate");
	int gopSize = config_get_int(main->Config(), "AdvOut", "FFVGOPSize");
	bool rescale = config_get_bool(main->Config(), "AdvOut", "FFRescale");
	const char *rescaleRes = config_get_string(main->Config(), "AdvOut", "FFRescaleRes");
	const char *formatName = config_get_string(main->Config(), "AdvOut", "FFFormat");
	const char *mimeType = config_get_string(main->Config(), "AdvOut", "FFFormatMimeType");
	const char *muxCustom = config_get_string(main->Config(), "AdvOut", "FFMCustom");
	const char *vEncoder = config_get_string(main->Config(), "AdvOut", "FFVEncoder");
	int vEncoderId = config_get_int(main->Config(), "AdvOut", "FFVEncoderId");
	const char *vEncCustom = config_get_string(main->Config(), "AdvOut", "FFVCustom");
	int aBitrate = config_get_int(main->Config(), "AdvOut", "FFABitrate");
	int aMixes = config_get_int(main->Config(), "AdvOut", "FFAudioMixes");
	const char *aEncoder = config_get_string(main->Config(), "AdvOut", "FFAEncoder");
	int aEncoderId = config_get_int(main->Config(), "AdvOut", "FFAEncoderId");
	const char *aEncCustom = config_get_string(main->Config(), "AdvOut", "FFACustom");

	OBSDataArrayAutoRelease audio_names = obs_data_array_create();

	for (size_t i = 0; i < MAX_AUDIO_MIXES; i++) {
		string cfg_name = "Track";
		cfg_name += to_string((int)i + 1);
		cfg_name += "Name";

		const char *audioName = config_get_string(main->Config(), "AdvOut", cfg_name.c_str());

		OBSDataAutoRelease item = obs_data_create();
		obs_data_set_string(item, "name", audioName);
		obs_data_array_push_back(audio_names, item);
	}

	OBSDataAutoRelease settings = obs_data_create();

	obs_data_set_array(settings, "audio_names", audio_names);
	obs_data_set_string(settings, "url", url);
	obs_data_set_string(settings, "format_name", formatName);
	obs_data_set_string(settings, "format_mime_type", mimeType);
	obs_data_set_string(settings, "muxer_settings", muxCustom);
	obs_data_set_int(settings, "gop_size", gopSize);
	obs_data_set_int(settings, "video_bitrate", vBitrate);
	obs_data_set_string(settings, "video_encoder", vEncoder);
	obs_data_set_int(settings, "video_encoder_id", vEncoderId);
	obs_data_set_string(settings, "video_settings", vEncCustom);
	obs_data_set_int(settings, "audio_bitrate", aBitrate);
	obs_data_set_string(settings, "audio_encoder", aEncoder);
	obs_data_set_int(settings, "audio_encoder_id", aEncoderId);
	obs_data_set_string(settings, "audio_settings", aEncCustom);

	if (rescale && rescaleRes && *rescaleRes) {
		int width;
		int height;
		int val = sscanf(rescaleRes, "%dx%d", &width, &height);

		if (val == 2 && width && height) {
			obs_data_set_int(settings, "scale_width", width);
			obs_data_set_int(settings, "scale_height", height);
		}
	}

	obs_output_set_mixers(fileOutput, aMixes);
	obs_output_set_media(fileOutput, obs_get_video(), obs_get_audio());
	obs_output_update(fileOutput, settings);
}

static inline void SetEncoderName(obs_encoder_t *encoder, const char *name, const char *defaultName)
{
	obs_encoder_set_name(encoder, (name && *name) ? name : defaultName);
}

inline void AdvancedOutput::UpdateAudioSettings()
{
	bool applyServiceSettings = config_get_bool(main->Config(), "AdvOut", "ApplyServiceSettings");
	bool enforceBitrate = !config_get_bool(main->Config(), "Stream1", "IgnoreRecommended");
	int streamTrackIndex = config_get_int(main->Config(), "AdvOut", "TrackIndex");
	int vodTrackIndex = config_get_int(main->Config(), "AdvOut", "VodTrackIndex");
	const char *audioEncoder = config_get_string(main->Config(), "AdvOut", "AudioEncoder");
	const char *recAudioEncoder = config_get_string(main->Config(), "AdvOut", "RecAudioEncoder");

	bool is_multitrack_output = allowsMultiTrack();

	OBSDataAutoRelease settings[MAX_AUDIO_MIXES];

	for (size_t i = 0; i < MAX_AUDIO_MIXES; i++) {
		string cfg_name = "Track";
		cfg_name += to_string((int)i + 1);
		cfg_name += "Name";
		const char *name = config_get_string(main->Config(), "AdvOut", cfg_name.c_str());

		string def_name = "Track";
		def_name += to_string((int)i + 1);
		SetEncoderName(recordTrack[i], name, def_name.c_str());
		SetEncoderName(streamTrack[i], name, def_name.c_str());
	}

	for (size_t i = 0; i < MAX_AUDIO_MIXES; i++) {
		int track = (int)(i + 1);
		settings[i] = obs_data_create();
		obs_data_set_int(settings[i], "bitrate", GetAudioBitrate(i, recAudioEncoder));

		obs_encoder_update(recordTrack[i], settings[i]);

		obs_data_set_int(settings[i], "bitrate", GetAudioBitrate(i, audioEncoder));

		if (!is_multitrack_output) {
			if (track == streamTrackIndex || track == vodTrackIndex) {
				if (applyServiceSettings) {
					int bitrate = (int)obs_data_get_int(settings[i], "bitrate");
					obs_service_apply_encoder_settings(main->GetService(), nullptr, settings[i]);

					if (!enforceBitrate)
						obs_data_set_int(settings[i], "bitrate", bitrate);
				}
			}

			if (track == streamTrackIndex)
				obs_encoder_update(streamAudioEnc, settings[i]);
			if (track == vodTrackIndex)
				obs_encoder_update(streamArchiveEnc, settings[i]);
		} else {
			obs_encoder_update(streamTrack[i], settings[i]);
		}
	}
}

void AdvancedOutput::SetupOutputs()
{
	obs_encoder_set_video(videoStreaming, obs_get_video());
	if (videoRecording)
		obs_encoder_set_video(videoRecording, obs_get_video());
	for (size_t i = 0; i < MAX_AUDIO_MIXES; i++) {
		obs_encoder_set_audio(streamTrack[i], obs_get_audio());
		obs_encoder_set_audio(recordTrack[i], obs_get_audio());
	}
	obs_encoder_set_audio(streamAudioEnc, obs_get_audio());
	obs_encoder_set_audio(streamArchiveEnc, obs_get_audio());

	SetupStreaming();

	if (ffmpegOutput)
		SetupFFmpeg();
	else
		SetupRecording();
}

int AdvancedOutput::GetAudioBitrate(size_t i, const char *id) const
{
	static const char *names[] = {
		"Track1Bitrate", "Track2Bitrate", "Track3Bitrate", "Track4Bitrate", "Track5Bitrate", "Track6Bitrate",
	};
	int bitrate = (int)config_get_uint(main->Config(), "AdvOut", names[i]);
	return FindClosestAvailableAudioBitrate(id, bitrate);
}

inline std::optional<size_t> AdvancedOutput::VodTrackMixerIdx(obs_service_t *service)
{
	int streamTrackIndex = config_get_int(main->Config(), "AdvOut", "TrackIndex");
	bool vodTrackEnabled = config_get_bool(main->Config(), "AdvOut", "VodTrackEnabled");
	int vodTrackIndex = config_get_int(main->Config(), "AdvOut", "VodTrackIndex");
	bool enableForCustomServer = config_get_bool(App()->GetUserConfig(), "General", "EnableCustomServerVodTrack");

	const char *id = obs_service_get_id(service);
	if (strcmp(id, "rtmp_custom") == 0) {
		vodTrackEnabled = enableForCustomServer ? vodTrackEnabled : false;
	} else {
		OBSDataAutoRelease settings = obs_service_get_settings(service);
		const char *service = obs_data_get_string(settings, "service");
		if (!ServiceSupportsVodTrack(service))
			vodTrackEnabled = false;
	}

	if (vodTrackEnabled && streamTrackIndex != vodTrackIndex)
		return {vodTrackIndex - 1};
	return std::nullopt;
}

inline void AdvancedOutput::SetupVodTrack(obs_service_t *service)
{
	if (VodTrackMixerIdx(service).has_value())
		obs_output_set_audio_encoder(streamOutput, streamArchiveEnc, 1);
	else
		clear_archive_encoder(streamOutput, ADV_ARCHIVE_NAME);
}

std::shared_future<void> AdvancedOutput::SetupStreaming(obs_service_t *service,
							SetupStreamingContinuation_t continuation)
{
	int multiTrackAudioMixes = config_get_int(main->Config(), "AdvOut", "StreamMultiTrackAudioMixes");

	bool is_multitrack_output = allowsMultiTrack();

	if (!useStreamEncoder || (!ffmpegOutput && !obs_output_active(fileOutput))) {
		UpdateStreamSettings();
	}

	UpdateAudioSettings();

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

	const char *audio_encoder_id = config_get_string(main->Config(), "AdvOut", "AudioEncoder");
	int streamTrackIndex = config_get_int(main->Config(), "AdvOut", "TrackIndex") - 1;

	auto handle_multitrack_video_result = [=](std::optional<bool> multitrackVideoResult) {
		if (multitrackVideoResult.has_value())
			return multitrackVideoResult.value();

		/* XXX: this is messy and disgusting and should be refactored */
		if (outputType != type) {
			streamDelayStarting.Disconnect();
			streamStopping.Disconnect();
			startStreaming.Disconnect();
			stopStreaming.Disconnect();

			streamOutput = obs_output_create(type, "adv_stream", nullptr, nullptr);
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
		obs_output_set_audio_encoder(streamOutput, streamAudioEnc, 0);

		if (!is_multitrack_output) {
			obs_output_set_audio_encoder(streamOutput, streamAudioEnc, 0);
		} else {
			int idx = 0;
			for (int i = 0; i < MAX_AUDIO_MIXES; i++) {
				if ((multiTrackAudioMixes & (1 << i)) != 0) {
					obs_output_set_audio_encoder(streamOutput, streamTrack[i], idx);
					idx++;
				}
			}
		}

		return true;
	};

	return SetupMultitrackVideo(service, audio_encoder_id, static_cast<size_t>(streamTrackIndex),
				    VodTrackMixerIdx(service), [=](std::optional<bool> res) {
					    continuation(handle_multitrack_video_result(res));
				    });
}

bool AdvancedOutput::StartStreaming(obs_service_t *service)
{
	obs_output_set_service(streamOutput, service);

	bool reconnect = config_get_bool(main->Config(), "Output", "Reconnect");
	int retryDelay = config_get_int(main->Config(), "Output", "RetryDelay");
	int maxRetries = config_get_int(main->Config(), "Output", "MaxRetries");
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

	bool is_rtmp = false;
	obs_service_t *service_obj = main->GetService();
	const char *protocol = obs_service_get_protocol(service_obj);
	if (protocol) {
		if (astrcmpi_n(protocol, RTMP_PROTOCOL, strlen(RTMP_PROTOCOL)) == 0)
			is_rtmp = true;
	}

	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "bind_ip", bindIP);
	obs_data_set_string(settings, "ip_family", ipFamily);
#ifdef _WIN32
	obs_data_set_bool(settings, "new_socket_loop_enabled", enableNewSocketLoop);
	obs_data_set_bool(settings, "low_latency_mode_enabled", enableLowLatencyMode);
#endif
	obs_data_set_bool(settings, "dyn_bitrate", enableDynBitrate);

	auto streamOutput = StreamingOutput(); // shadowing is sort of bad, but also convenient

	obs_output_update(streamOutput, settings);

	if (!reconnect)
		maxRetries = 0;

	obs_output_set_delay(streamOutput, useDelay ? delaySec : 0, preserveDelay ? OBS_OUTPUT_DELAY_PRESERVE : 0);

	obs_output_set_reconnect_settings(streamOutput, maxRetries, retryDelay);
	if (is_rtmp) {
		SetupVodTrack(service);
	}
	if (obs_output_start(streamOutput)) {
		if (multitrackVideo && multitrackVideoActive)
			multitrackVideo->StartedStreaming();
		return true;
	}

	if (multitrackVideo && multitrackVideoActive)
		multitrackVideoActive = false;

	const char *error = obs_output_get_last_error(streamOutput);
	bool hasLastError = error && *error;
	if (hasLastError)
		lastError = error;
	else
		lastError = string();

	const char *type = obs_output_get_id(streamOutput);
	blog(LOG_WARNING, "Stream output type '%s' failed to start!%s%s", type, hasLastError ? "  Last Error: " : "",
	     hasLastError ? error : "");
	return false;
}

bool AdvancedOutput::StartRecording()
{
	const char *path;
	const char *recFormat;
	const char *filenameFormat;
	bool noSpace = false;
	bool overwriteIfExists = false;
	bool splitFile;
	const char *splitFileType;
	int splitFileTime;
	int splitFileSize;

	if (!useStreamEncoder) {
		if (!ffmpegOutput) {
			UpdateRecordingSettings();
		}
	} else if (!obs_output_active(StreamingOutput())) {
		UpdateStreamSettings();
	}

	UpdateAudioSettings();

	if (!Active())
		SetupOutputs();

	if (!ffmpegOutput || ffmpegRecording) {
		path = config_get_string(main->Config(), "AdvOut", ffmpegRecording ? "FFFilePath" : "RecFilePath");
		recFormat = config_get_string(main->Config(), "AdvOut", ffmpegRecording ? "FFExtension" : "RecFormat2");
		filenameFormat = config_get_string(main->Config(), "Output", "FilenameFormatting");
		overwriteIfExists = config_get_bool(main->Config(), "Output", "OverwriteIfExists");
		noSpace = config_get_bool(main->Config(), "AdvOut",
					  ffmpegRecording ? "FFFileNameWithoutSpace" : "RecFileNameWithoutSpace");
		splitFile = config_get_bool(main->Config(), "AdvOut", "RecSplitFile");

		string strPath = GetRecordingFilename(path, recFormat, noSpace, overwriteIfExists, filenameFormat,
						      ffmpegRecording);

		OBSDataAutoRelease settings = obs_data_create();
		obs_data_set_string(settings, ffmpegRecording ? "url" : "path", strPath.c_str());

		if (splitFile) {
			splitFileType = config_get_string(main->Config(), "AdvOut", "RecSplitFileType");
			splitFileTime = (astrcmpi(splitFileType, "Time") == 0)
						? config_get_int(main->Config(), "AdvOut", "RecSplitFileTime")
						: 0;
			splitFileSize = (astrcmpi(splitFileType, "Size") == 0)
						? config_get_int(main->Config(), "AdvOut", "RecSplitFileSize")
						: 0;
			string ext = GetFormatExt(recFormat);
			obs_data_set_string(settings, "directory", path);
			obs_data_set_string(settings, "format", filenameFormat);
			obs_data_set_string(settings, "extension", ext.c_str());
			obs_data_set_bool(settings, "allow_spaces", !noSpace);
			obs_data_set_bool(settings, "allow_overwrite", overwriteIfExists);
			obs_data_set_bool(settings, "split_file", true);
			obs_data_set_int(settings, "max_time_sec", splitFileTime * 60);
			obs_data_set_int(settings, "max_size_mb", splitFileSize);
		}

		obs_output_update(fileOutput, settings);
	}

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

bool AdvancedOutput::StartReplayBuffer()
{
	const char *path;
	const char *recFormat;
	const char *filenameFormat;
	bool noSpace = false;
	bool overwriteIfExists = false;
	const char *rbPrefix;
	const char *rbSuffix;
	int rbTime;
	int rbSize;

	if (!useStreamEncoder) {
		if (!ffmpegOutput)
			UpdateRecordingSettings();
	} else if (!obs_output_active(StreamingOutput())) {
		UpdateStreamSettings();
	}

	UpdateAudioSettings();

	if (!Active())
		SetupOutputs();

	if (!ffmpegOutput || ffmpegRecording) {
		path = config_get_string(main->Config(), "AdvOut", ffmpegRecording ? "FFFilePath" : "RecFilePath");
		recFormat = config_get_string(main->Config(), "AdvOut", ffmpegRecording ? "FFExtension" : "RecFormat2");
		filenameFormat = config_get_string(main->Config(), "Output", "FilenameFormatting");
		overwriteIfExists = config_get_bool(main->Config(), "Output", "OverwriteIfExists");
		noSpace = config_get_bool(main->Config(), "AdvOut",
					  ffmpegRecording ? "FFFileNameWithoutSpace" : "RecFileNameWithoutSpace");
		rbPrefix = config_get_string(main->Config(), "SimpleOutput", "RecRBPrefix");
		rbSuffix = config_get_string(main->Config(), "SimpleOutput", "RecRBSuffix");
		rbTime = config_get_int(main->Config(), "AdvOut", "RecRBTime");
		rbSize = config_get_int(main->Config(), "AdvOut", "RecRBSize");

		string f = GetFormatString(filenameFormat, rbPrefix, rbSuffix);
		string ext = GetFormatExt(recFormat);

		OBSDataAutoRelease settings = obs_data_create();

		obs_data_set_string(settings, "directory", path);
		obs_data_set_string(settings, "format", f.c_str());
		obs_data_set_string(settings, "extension", ext.c_str());
		obs_data_set_bool(settings, "allow_spaces", !noSpace);
		obs_data_set_int(settings, "max_time_sec", rbTime);
		obs_data_set_int(settings, "max_size_mb", usesBitrate ? 0 : rbSize);

		obs_output_update(replayBuffer, settings);
	}

	if (!obs_output_start(replayBuffer)) {
		QString error_reason;
		const char *error = obs_output_get_last_error(replayBuffer);
		if (error)
			error_reason = QT_UTF8(error);
		else
			error_reason = QTStr("Output.StartFailedGeneric");
		QMessageBox::critical(main, QTStr("Output.StartReplayFailed"), error_reason);
		return false;
	}

	return true;
}

void AdvancedOutput::StopStreaming(bool force)
{
	auto output = StreamingOutput();
	if (force && output)
		obs_output_force_stop(output);
	else if (multitrackVideo && multitrackVideoActive)
		multitrackVideo->StopStreaming();
	else
		obs_output_stop(output);
}

void AdvancedOutput::StopRecording(bool force)
{
	if (force)
		obs_output_force_stop(fileOutput);
	else
		obs_output_stop(fileOutput);
}

void AdvancedOutput::StopReplayBuffer(bool force)
{
	if (force)
		obs_output_force_stop(replayBuffer);
	else
		obs_output_stop(replayBuffer);
}

bool AdvancedOutput::StreamingActive() const
{
	return obs_output_active(StreamingOutput());
}

bool AdvancedOutput::RecordingActive() const
{
	return obs_output_active(fileOutput);
}

bool AdvancedOutput::ReplayBufferActive() const
{
	return obs_output_active(replayBuffer);
}
