#include <string>
#include <algorithm>
#include <cinttypes>
#include <QMessageBox>
#include <QThreadPool>
#include <qt-wrappers.hpp>
#include "audio-encoders.hpp"
#include "multitrack-video-error.hpp"
#include "window-basic-main.hpp"
#include "window-basic-main-outputs.hpp"
#include "window-basic-vcam.hpp"

using namespace std;

extern bool EncoderAvailable(const char *encoder);

volatile bool streaming_active = false;
volatile bool recording_active = false;
volatile bool recording_paused = false;
volatile bool replaybuf_active = false;
volatile bool virtualcam_active = false;

#define RTMP_PROTOCOL "rtmp"
#define SRT_PROTOCOL "srt"
#define RIST_PROTOCOL "rist"

static void OBSStreamStarting(void *data, calldata_t *params)
{
	BasicOutputHandler *output = static_cast<BasicOutputHandler *>(data);
	obs_output_t *obj = (obs_output_t *)calldata_ptr(params, "output");

	int sec = (int)obs_output_get_active_delay(obj);
	if (sec == 0)
		return;

	output->delayActive = true;
	QMetaObject::invokeMethod(output->main, "StreamDelayStarting", Q_ARG(int, sec));
}

static void OBSStreamStopping(void *data, calldata_t *params)
{
	BasicOutputHandler *output = static_cast<BasicOutputHandler *>(data);
	obs_output_t *obj = (obs_output_t *)calldata_ptr(params, "output");

	int sec = (int)obs_output_get_active_delay(obj);
	if (sec == 0)
		QMetaObject::invokeMethod(output->main, "StreamStopping");
	else
		QMetaObject::invokeMethod(output->main, "StreamDelayStopping", Q_ARG(int, sec));
}

static void OBSStartStreaming(void *data, calldata_t * /* params */)
{
	BasicOutputHandler *output = static_cast<BasicOutputHandler *>(data);
	output->streamingActive = true;
	os_atomic_set_bool(&streaming_active, true);
	QMetaObject::invokeMethod(output->main, "StreamingStart");
}

static void OBSStopStreaming(void *data, calldata_t *params)
{
	BasicOutputHandler *output = static_cast<BasicOutputHandler *>(data);
	int code = (int)calldata_int(params, "code");
	const char *last_error = calldata_string(params, "last_error");

	QString arg_last_error = QString::fromUtf8(last_error);

	output->streamingActive = false;
	output->delayActive = false;
	output->multitrackVideoActive = false;
	os_atomic_set_bool(&streaming_active, false);
	QMetaObject::invokeMethod(output->main, "StreamingStop", Q_ARG(int, code), Q_ARG(QString, arg_last_error));
}

static void OBSStartRecording(void *data, calldata_t * /* params */)
{
	BasicOutputHandler *output = static_cast<BasicOutputHandler *>(data);

	output->recordingActive = true;
	os_atomic_set_bool(&recording_active, true);
	QMetaObject::invokeMethod(output->main, "RecordingStart");
}

static void OBSStopRecording(void *data, calldata_t *params)
{
	BasicOutputHandler *output = static_cast<BasicOutputHandler *>(data);
	int code = (int)calldata_int(params, "code");
	const char *last_error = calldata_string(params, "last_error");

	QString arg_last_error = QString::fromUtf8(last_error);

	output->recordingActive = false;
	os_atomic_set_bool(&recording_active, false);
	os_atomic_set_bool(&recording_paused, false);
	QMetaObject::invokeMethod(output->main, "RecordingStop", Q_ARG(int, code), Q_ARG(QString, arg_last_error));
}

static void OBSRecordStopping(void *data, calldata_t * /* params */)
{
	BasicOutputHandler *output = static_cast<BasicOutputHandler *>(data);
	QMetaObject::invokeMethod(output->main, "RecordStopping");
}

static void OBSRecordFileChanged(void *data, calldata_t *params)
{
	BasicOutputHandler *output = static_cast<BasicOutputHandler *>(data);
	const char *next_file = calldata_string(params, "next_file");

	QString arg_last_file = QString::fromUtf8(output->lastRecordingPath.c_str());

	QMetaObject::invokeMethod(output->main, "RecordingFileChanged", Q_ARG(QString, arg_last_file));

	output->lastRecordingPath = next_file;
}

static void OBSStartReplayBuffer(void *data, calldata_t * /* params */)
{
	BasicOutputHandler *output = static_cast<BasicOutputHandler *>(data);

	output->replayBufferActive = true;
	os_atomic_set_bool(&replaybuf_active, true);
	QMetaObject::invokeMethod(output->main, "ReplayBufferStart");
}

static void OBSStopReplayBuffer(void *data, calldata_t *params)
{
	BasicOutputHandler *output = static_cast<BasicOutputHandler *>(data);
	int code = (int)calldata_int(params, "code");

	output->replayBufferActive = false;
	os_atomic_set_bool(&replaybuf_active, false);
	QMetaObject::invokeMethod(output->main, "ReplayBufferStop", Q_ARG(int, code));
}

static void OBSReplayBufferStopping(void *data, calldata_t * /* params */)
{
	BasicOutputHandler *output = static_cast<BasicOutputHandler *>(data);
	QMetaObject::invokeMethod(output->main, "ReplayBufferStopping");
}

static void OBSReplayBufferSaved(void *data, calldata_t * /* params */)
{
	BasicOutputHandler *output = static_cast<BasicOutputHandler *>(data);
	QMetaObject::invokeMethod(output->main, "ReplayBufferSaved", Qt::QueuedConnection);
}

static void OBSStartVirtualCam(void *data, calldata_t * /* params */)
{
	BasicOutputHandler *output = static_cast<BasicOutputHandler *>(data);

	output->virtualCamActive = true;
	os_atomic_set_bool(&virtualcam_active, true);
	QMetaObject::invokeMethod(output->main, "OnVirtualCamStart");
}

static void OBSStopVirtualCam(void *data, calldata_t *params)
{
	BasicOutputHandler *output = static_cast<BasicOutputHandler *>(data);
	int code = (int)calldata_int(params, "code");

	output->virtualCamActive = false;
	os_atomic_set_bool(&virtualcam_active, false);
	QMetaObject::invokeMethod(output->main, "OnVirtualCamStop", Q_ARG(int, code));
}

static void OBSDeactivateVirtualCam(void *data, calldata_t * /* params */)
{
	BasicOutputHandler *output = static_cast<BasicOutputHandler *>(data);
	output->DestroyVirtualCamView();
}

/* ------------------------------------------------------------------------ */

struct StartMultitrackVideoStreamingGuard {
	StartMultitrackVideoStreamingGuard() { future = guard.get_future().share(); };
	~StartMultitrackVideoStreamingGuard() { guard.set_value(); }

	std::shared_future<void> GetFuture() const { return future; }

	static std::shared_future<void> MakeReadyFuture()
	{
		StartMultitrackVideoStreamingGuard guard;
		return guard.GetFuture();
	}

private:
	std::promise<void> guard;
	std::shared_future<void> future;
};

/* ------------------------------------------------------------------------ */

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

static inline bool can_use_output(const char *prot, const char *output, const char *prot_test1,
				  const char *prot_test2 = nullptr)
{
	return (strcmp(prot, prot_test1) == 0 || (prot_test2 && strcmp(prot, prot_test2) == 0)) &&
	       (obs_get_output_flags(output) & OBS_OUTPUT_SERVICE) != 0;
}

static bool return_first_id(void *data, const char *id)
{
	const char **output = (const char **)data;

	*output = id;
	return false;
}

static const char *GetStreamOutputType(const obs_service_t *service)
{
	const char *protocol = obs_service_get_protocol(service);
	const char *output = nullptr;

	if (!protocol) {
		blog(LOG_WARNING, "The service '%s' has no protocol set", obs_service_get_id(service));
		return nullptr;
	}

	if (!obs_is_output_protocol_registered(protocol)) {
		blog(LOG_WARNING, "The protocol '%s' is not registered", protocol);
		return nullptr;
	}

	/* Check if the service has a preferred output type */
	output = obs_service_get_preferred_output_type(service);
	if (output) {
		if ((obs_get_output_flags(output) & OBS_OUTPUT_SERVICE) != 0)
			return output;

		blog(LOG_WARNING, "The output '%s' is not registered, fallback to another one", output);
	}

	/* Otherwise, prefer first-party output types */
	if (can_use_output(protocol, "rtmp_output", "RTMP", "RTMPS")) {
		return "rtmp_output";
	} else if (can_use_output(protocol, "ffmpeg_hls_muxer", "HLS")) {
		return "ffmpeg_hls_muxer";
	} else if (can_use_output(protocol, "ffmpeg_mpegts_muxer", "SRT", "RIST")) {
		return "ffmpeg_mpegts_muxer";
	}

	/* If third-party protocol, use the first enumerated type */
	obs_enum_output_types_with_protocol(protocol, &output, return_first_id);
	if (output)
		return output;

	blog(LOG_WARNING, "No output compatible with the service '%s' is registered", obs_service_get_id(service));

	return nullptr;
}

/* ------------------------------------------------------------------------ */

inline BasicOutputHandler::BasicOutputHandler(OBSBasic *main_) : main(main_)
{
	if (main->vcamEnabled) {
		virtualCam = obs_output_create(VIRTUAL_CAM_ID, "virtualcam_output", nullptr, nullptr);

		signal_handler_t *signal = obs_output_get_signal_handler(virtualCam);
		startVirtualCam.Connect(signal, "start", OBSStartVirtualCam, this);
		stopVirtualCam.Connect(signal, "stop", OBSStopVirtualCam, this);
		deactivateVirtualCam.Connect(signal, "deactivate", OBSDeactivateVirtualCam, this);
	}

	auto multitrack_enabled = config_get_bool(main->Config(), "Stream1", "EnableMultitrackVideo");
	if (!config_has_user_value(main->Config(), "Stream1", "EnableMultitrackVideo")) {
		auto service = main_->GetService();
		OBSDataAutoRelease settings = obs_service_get_settings(service);
		multitrack_enabled = obs_data_has_user_value(settings, "multitrack_video_configuration_url");
	}
	if (multitrack_enabled)
		multitrackVideo = make_unique<MultitrackVideoOutput>();
}

extern void log_vcam_changed(const VCamConfig &config, bool starting);

bool BasicOutputHandler::StartVirtualCam()
{
	if (!main->vcamEnabled)
		return false;

	bool typeIsProgram = main->vcamConfig.type == VCamOutputType::ProgramView;

	if (!virtualCamView && !typeIsProgram)
		virtualCamView = obs_view_create();

	UpdateVirtualCamOutputSource();

	if (!virtualCamVideo) {
		virtualCamVideo = typeIsProgram ? obs_get_video() : obs_view_add(virtualCamView);

		if (!virtualCamVideo)
			return false;
	}

	obs_output_set_media(virtualCam, virtualCamVideo, obs_get_audio());
	if (!Active())
		SetupOutputs();

	bool success = obs_output_start(virtualCam);
	if (!success) {
		QString errorReason;

		const char *error = obs_output_get_last_error(virtualCam);
		if (error) {
			errorReason = QT_UTF8(error);
		} else {
			errorReason = QTStr("Output.StartFailedGeneric");
		}

		QMessageBox::critical(main, QTStr("Output.StartVirtualCamFailed"), errorReason);

		DestroyVirtualCamView();
	}

	log_vcam_changed(main->vcamConfig, true);

	return success;
}

void BasicOutputHandler::StopVirtualCam()
{
	if (main->vcamEnabled) {
		obs_output_stop(virtualCam);
	}
}

bool BasicOutputHandler::VirtualCamActive() const
{
	if (main->vcamEnabled) {
		return obs_output_active(virtualCam);
	}
	return false;
}

void BasicOutputHandler::UpdateVirtualCamOutputSource()
{
	if (!main->vcamEnabled || !virtualCamView)
		return;

	OBSSourceAutoRelease source;

	switch (main->vcamConfig.type) {
	case VCamOutputType::Invalid:
	case VCamOutputType::ProgramView:
		DestroyVirtualCameraScene();
		return;
	case VCamOutputType::PreviewOutput: {
		DestroyVirtualCameraScene();
		OBSSource s = main->GetCurrentSceneSource();
		obs_source_get_ref(s);
		source = s.Get();
		break;
	}
	case VCamOutputType::SceneOutput:
		DestroyVirtualCameraScene();
		source = obs_get_source_by_name(main->vcamConfig.scene.c_str());
		break;
	case VCamOutputType::SourceOutput:
		OBSSourceAutoRelease s = obs_get_source_by_name(main->vcamConfig.source.c_str());

		if (!vCamSourceScene)
			vCamSourceScene = obs_scene_create_private("vcam_source");
		source = obs_source_get_ref(obs_scene_get_source(vCamSourceScene));

		if (vCamSourceSceneItem && (obs_sceneitem_get_source(vCamSourceSceneItem) != s)) {
			obs_sceneitem_remove(vCamSourceSceneItem);
			vCamSourceSceneItem = nullptr;
		}

		if (!vCamSourceSceneItem) {
			vCamSourceSceneItem = obs_scene_add(vCamSourceScene, s);

			obs_sceneitem_set_bounds_type(vCamSourceSceneItem, OBS_BOUNDS_SCALE_INNER);
			obs_sceneitem_set_bounds_alignment(vCamSourceSceneItem, OBS_ALIGN_CENTER);

			const struct vec2 size = {
				(float)obs_source_get_width(source),
				(float)obs_source_get_height(source),
			};
			obs_sceneitem_set_bounds(vCamSourceSceneItem, &size);
		}
		break;
	}

	OBSSourceAutoRelease current = obs_view_get_source(virtualCamView, 0);
	if (source != current)
		obs_view_set_source(virtualCamView, 0, source);
}

void BasicOutputHandler::DestroyVirtualCamView()
{
	if (main->vcamConfig.type == VCamOutputType::ProgramView) {
		virtualCamVideo = nullptr;
		return;
	}

	obs_view_remove(virtualCamView);
	obs_view_set_source(virtualCamView, 0, nullptr);
	virtualCamVideo = nullptr;

	obs_view_destroy(virtualCamView);
	virtualCamView = nullptr;

	DestroyVirtualCameraScene();
}

void BasicOutputHandler::DestroyVirtualCameraScene()
{
	if (!vCamSourceScene)
		return;

	obs_scene_release(vCamSourceScene);
	vCamSourceScene = nullptr;
	vCamSourceSceneItem = nullptr;
}

/* ------------------------------------------------------------------------ */

struct SimpleOutput : BasicOutputHandler {
	OBSEncoder audioStreaming;
	OBSEncoder videoStreaming;
	OBSEncoder audioRecording;
	OBSEncoder audioArchive;
	OBSEncoder videoRecording;
	OBSEncoder audioTrack[MAX_AUDIO_MIXES];

	string videoEncoder;
	string videoQuality;
	bool usingRecordingPreset = false;
	bool recordingConfigured = false;
	bool ffmpegOutput = false;
	bool lowCPUx264 = false;

	SimpleOutput(OBSBasic *main_);

	int CalcCRF(int crf);

	void UpdateRecordingSettings_x264_crf(int crf);
	void UpdateRecordingSettings_qsv11(int crf, bool av1);
	void UpdateRecordingSettings_nvenc(int cqp);
	void UpdateRecordingSettings_nvenc_hevc_av1(int cqp);
	void UpdateRecordingSettings_amd_cqp(int cqp);
	void UpdateRecordingSettings_apple(int quality);
#ifdef ENABLE_HEVC
	void UpdateRecordingSettings_apple_hevc(int quality);
#endif
	void UpdateRecordingSettings();
	void UpdateRecordingAudioSettings();
	virtual void Update() override;

	void SetupOutputs() override;
	int GetAudioBitrate() const;

	void LoadRecordingPreset_Lossy(const char *encoder);
	void LoadRecordingPreset_Lossless();
	void LoadRecordingPreset();

	void LoadStreamingPreset_Lossy(const char *encoder);

	void UpdateRecording();
	bool ConfigureRecording(bool useReplayBuffer);

	bool IsVodTrackEnabled(obs_service_t *service);
	void SetupVodTrack(obs_service_t *service);

	virtual std::shared_future<void> SetupStreaming(obs_service_t *service,
							SetupStreamingContinuation_t continuation) override;
	virtual bool StartStreaming(obs_service_t *service) override;
	virtual bool StartRecording() override;
	virtual bool StartReplayBuffer() override;
	virtual void StopStreaming(bool force) override;
	virtual void StopRecording(bool force) override;
	virtual void StopReplayBuffer(bool force) override;
	virtual bool StreamingActive() const override;
	virtual bool RecordingActive() const override;
	virtual bool ReplayBufferActive() const override;
};

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
	if (strcmp(encoder, SIMPLE_ENCODER_OPENH264) == 0) {
		return "ffmpeg_openh264";
	} else if (strcmp(encoder, SIMPLE_ENCODER_X264) == 0) {
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

	return "ffmpeg_openh264";
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
		blog(LOG_INFO, "User is ignoring service bitrate limits.");
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
	int tracks = config_get_int(main->Config(), "SimpleOutput", "RecTracks");
	const char *recFormat = config_get_string(main->Config(), "SimpleOutput", "RecFormat2");
	bool flv = strcmp(recFormat, "flv") == 0;

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

const char *FindAudioEncoderFromCodec(const char *type)
{
	const char *alt_enc_id = nullptr;
	size_t i = 0;

	while (obs_enum_encoder_types(i++, &alt_enc_id)) {
		const char *codec = obs_get_encoder_codec(alt_enc_id);
		if (strcmp(type, codec) == 0) {
			return alt_enc_id;
		}
	}

	return nullptr;
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

	auto handle_multitrack_video_result = [&](std::optional<bool> multitrackVideoResult) {
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
				    [&, continuation](std::optional<bool> res) {
					    continuation(handle_multitrack_video_result(res));
				    });
}

static inline bool ServiceSupportsVodTrack(const char *service);

static void clear_archive_encoder(obs_output_t *output, const char *expected_name)
{
	obs_encoder_t *last = obs_output_get_audio_encoder(output, 1);
	bool clear = false;

	/* ensures that we don't remove twitch's soundtrack encoder */
	if (last) {
		const char *name = obs_encoder_get_name(last);
		clear = name && strcmp(name, expected_name) == 0;
		obs_encoder_release(last);
	}

	if (clear)
		obs_output_set_audio_encoder(output, nullptr, 1);
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
#else
	bool enableNewSocketLoop = false;
#endif
	bool enableDynBitrate = config_get_bool(main->Config(), "Output", "DynamicBitrate");

	if (multitrackVideo && multitrackVideoActive &&
	    !multitrackVideo->HandleIncompatibleSettings(main, main->Config(), service, useDelay, enableNewSocketLoop,
							 enableDynBitrate)) {
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

	auto streamOutput = StreamingOutput(); // shadowing is sort of bad, but also convenient

	obs_output_update(streamOutput, settings);

	if (!reconnect)
		maxRetries = 0;

	obs_output_set_delay(streamOutput, useDelay ? delaySec : 0, preserveDelay ? OBS_OUTPUT_DELAY_PRESERVE : 0);

	obs_output_set_reconnect_settings(streamOutput, maxRetries, retryDelay);

	if (!multitrackVideo || !multitrackVideoActive)
		SetupVodTrack(service);

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

void SimpleOutput::UpdateRecording()
{
	const char *recFormat = config_get_string(main->Config(), "SimpleOutput", "RecFormat2");
	bool flv = strcmp(recFormat, "flv") == 0;
	int tracks = config_get_int(main->Config(), "SimpleOutput", "RecTracks");
	int idx = 0;
	int idx2 = 0;
	const char *quality = config_get_string(main->Config(), "SimpleOutput", "RecQuality");

	if (replayBufferActive || recordingActive)
		return;

	if (usingRecordingPreset) {
		if (!ffmpegOutput)
			UpdateRecordingSettings();
	} else if (!obs_output_active(streamOutput)) {
		Update();
	}

	if (!Active())
		SetupOutputs();

	if (!ffmpegOutput) {
		obs_output_set_video_encoder(fileOutput, videoRecording);
		if (flv || strcmp(quality, "Stream") == 0) {
			obs_output_set_audio_encoder(fileOutput, audioRecording, 0);
		} else {
			for (int i = 0; i < MAX_AUDIO_MIXES; i++) {
				if ((tracks & (1 << i)) != 0) {
					obs_output_set_audio_encoder(fileOutput, audioTrack[i], idx++);
				}
			}
		}
	}
	if (replayBuffer) {
		obs_output_set_video_encoder(replayBuffer, videoRecording);
		if (flv || strcmp(quality, "Stream") == 0) {
			obs_output_set_audio_encoder(replayBuffer, audioRecording, 0);
		} else {
			for (int i = 0; i < MAX_AUDIO_MIXES; i++) {
				if ((tracks & (1 << i)) != 0) {
					obs_output_set_audio_encoder(replayBuffer, audioTrack[i], idx2++);
				}
			}
		}
	}

	recordingConfigured = true;
}

bool SimpleOutput::ConfigureRecording(bool updateReplayBuffer)
{
	const char *path = config_get_string(main->Config(), "SimpleOutput", "FilePath");
	const char *format = config_get_string(main->Config(), "SimpleOutput", "RecFormat2");
	const char *mux = config_get_string(main->Config(), "SimpleOutput", "MuxerCustom");
	bool noSpace = config_get_bool(main->Config(), "SimpleOutput", "FileNameWithoutSpace");
	const char *filenameFormat = config_get_string(main->Config(), "Output", "FilenameFormatting");
	bool overwriteIfExists = config_get_bool(main->Config(), "Output", "OverwriteIfExists");
	const char *rbPrefix = config_get_string(main->Config(), "SimpleOutput", "RecRBPrefix");
	const char *rbSuffix = config_get_string(main->Config(), "SimpleOutput", "RecRBSuffix");
	int rbTime = config_get_int(main->Config(), "SimpleOutput", "RecRBTime");
	int rbSize = config_get_int(main->Config(), "SimpleOutput", "RecRBSize");
	int tracks = config_get_int(main->Config(), "SimpleOutput", "RecTracks");

	bool is_fragmented = strncmp(format, "fragmented", 10) == 0;
	bool is_lossless = videoQuality == "Lossless";

	string f;

	OBSDataAutoRelease settings = obs_data_create();
	if (updateReplayBuffer) {
		f = GetFormatString(filenameFormat, rbPrefix, rbSuffix);
		string ext = GetFormatExt(format);
		obs_data_set_string(settings, "directory", path);
		obs_data_set_string(settings, "format", f.c_str());
		obs_data_set_string(settings, "extension", ext.c_str());
		obs_data_set_bool(settings, "allow_spaces", !noSpace);
		obs_data_set_int(settings, "max_time_sec", rbTime);
		obs_data_set_int(settings, "max_size_mb", usingRecordingPreset ? rbSize : 0);
	} else {
		f = GetFormatString(filenameFormat, nullptr, nullptr);
		string strPath = GetRecordingFilename(path, ffmpegOutput ? "avi" : format, noSpace, overwriteIfExists,
						      f.c_str(), ffmpegOutput);
		obs_data_set_string(settings, ffmpegOutput ? "url" : "path", strPath.c_str());
		if (ffmpegOutput)
			obs_output_set_mixers(fileOutput, tracks);
	}

	// Use fragmented MOV/MP4 if user has not already specified custom movflags
	if (is_fragmented && !is_lossless && (!mux || strstr(mux, "movflags") == NULL)) {
		string mux_frag = "movflags=frag_keyframe+empty_moov+delay_moov";
		if (mux) {
			mux_frag += " ";
			mux_frag += mux;
		}
		obs_data_set_string(settings, "muxer_settings", mux_frag.c_str());
	} else {
		if (is_fragmented && !is_lossless)
			blog(LOG_WARNING, "User enabled fragmented recording, "
					  "but custom muxer settings contained movflags.");
		obs_data_set_string(settings, "muxer_settings", mux);
	}

	if (updateReplayBuffer)
		obs_output_update(replayBuffer, settings);
	else
		obs_output_update(fileOutput, settings);

	return true;
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

void SimpleOutput::StopStreaming(bool force)
{
	auto output = StreamingOutput();
	if (force && output)
		obs_output_force_stop(output);
	else if (multitrackVideo && multitrackVideoActive)
		multitrackVideo->StopStreaming();
	else
		obs_output_stop(output);
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
	return obs_output_active(StreamingOutput());
}

bool SimpleOutput::RecordingActive() const
{
	return obs_output_active(fileOutput);
}

bool SimpleOutput::ReplayBufferActive() const
{
	return obs_output_active(replayBuffer);
}

/* ------------------------------------------------------------------------ */

struct AdvancedOutput : BasicOutputHandler {
	OBSEncoder streamAudioEnc;
	OBSEncoder streamArchiveEnc;
	OBSEncoder streamTrack[MAX_AUDIO_MIXES];
	OBSEncoder recordTrack[MAX_AUDIO_MIXES];
	OBSEncoder videoStreaming;
	OBSEncoder videoRecording;

	bool ffmpegOutput;
	bool ffmpegRecording;
	bool useStreamEncoder;
	bool useStreamAudioEncoder;
	bool usesBitrate = false;

	AdvancedOutput(OBSBasic *main_);

	inline void UpdateStreamSettings();
	inline void UpdateRecordingSettings();
	inline void UpdateAudioSettings();
	virtual void Update() override;

	inline std::optional<size_t> VodTrackMixerIdx(obs_service_t *service);
	inline void SetupVodTrack(obs_service_t *service);

	inline void SetupStreaming();
	inline void SetupRecording();
	inline void SetupFFmpeg();
	void SetupOutputs() override;
	int GetAudioBitrate(size_t i, const char *id) const;

	virtual std::shared_future<void> SetupStreaming(obs_service_t *service,
							SetupStreamingContinuation_t continuation) override;
	virtual bool StartStreaming(obs_service_t *service) override;
	virtual bool StartRecording() override;
	virtual bool StartReplayBuffer() override;
	virtual void StopStreaming(bool force) override;
	virtual void StopRecording(bool force) override;
	virtual void StopReplayBuffer(bool force) override;
	virtual bool StreamingActive() const override;
	virtual bool RecordingActive() const override;
	virtual bool ReplayBufferActive() const override;
	bool allowsMultiTrack();
};

static OBSData GetDataFromJsonFile(const char *jsonFile)
{
	const OBSBasic *basic = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());

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

		bool native_muxer = strcmp(recFormat, "hybrid_mp4") == 0;
		fileOutput = obs_output_create(native_muxer ? "mp4_output" : "ffmpeg_muxer", "adv_file_output", nullptr,
					       nullptr);
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
			blog(LOG_INFO, "User is ignoring service bitrate limits.");
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

static inline bool ServiceSupportsVodTrack(const char *service)
{
	static const char *vodTrackServices[] = {"Twitch"};

	for (const char *vodTrackService : vodTrackServices) {
		if (astrcmpi(vodTrackService, service) == 0)
			return true;
	}

	return false;
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
	int idx = 0;

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

	auto handle_multitrack_video_result = [&](std::optional<bool> multitrackVideoResult) {
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
				    VodTrackMixerIdx(service), [&, continuation](std::optional<bool> res) {
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
#else
	bool enableNewSocketLoop = false;
#endif
	bool enableDynBitrate = config_get_bool(main->Config(), "Output", "DynamicBitrate");

	if (multitrackVideo && multitrackVideoActive &&
	    !multitrackVideo->HandleIncompatibleSettings(main, main->Config(), service, useDelay, enableNewSocketLoop,
							 enableDynBitrate)) {
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

/* ------------------------------------------------------------------------ */

void BasicOutputHandler::SetupAutoRemux(const char *&container)
{
	bool autoRemux = config_get_bool(main->Config(), "Video", "AutoRemux");
	if (autoRemux && strcmp(container, "mp4") == 0)
		container = "mkv";
}

std::string BasicOutputHandler::GetRecordingFilename(const char *path, const char *container, bool noSpace,
						     bool overwrite, const char *format, bool ffmpeg)
{
	if (!ffmpeg)
		SetupAutoRemux(container);

	string dst = GetOutputFilename(path, container, noSpace, overwrite, format);
	lastRecordingPath = dst;
	return dst;
}

extern std::string DeserializeConfigText(const char *text);

std::shared_future<void> BasicOutputHandler::SetupMultitrackVideo(obs_service_t *service, std::string audio_encoder_id,
								  size_t main_audio_mixer,
								  std::optional<size_t> vod_track_mixer,
								  std::function<void(std::optional<bool>)> continuation)
{
	auto start_streaming_guard = std::make_shared<StartMultitrackVideoStreamingGuard>();
	if (!multitrackVideo) {
		continuation(std::nullopt);
		return start_streaming_guard->GetFuture();
	}

	multitrackVideoActive = false;

	streamDelayStarting.Disconnect();
	streamStopping.Disconnect();
	startStreaming.Disconnect();
	stopStreaming.Disconnect();

	bool is_custom = strncmp("rtmp_custom", obs_service_get_type(service), 11) == 0;

	std::optional<std::string> custom_config = std::nullopt;
	if (config_get_bool(main->Config(), "Stream1", "MultitrackVideoConfigOverrideEnabled"))
		custom_config = DeserializeConfigText(
			config_get_string(main->Config(), "Stream1", "MultitrackVideoConfigOverride"));

	OBSDataAutoRelease settings = obs_service_get_settings(service);
	QString key = obs_data_get_string(settings, "key");

	const char *service_name = "<unknown>";
	if (is_custom && obs_data_has_user_value(settings, "service_name")) {
		service_name = obs_data_get_string(settings, "service_name");
	} else if (!is_custom) {
		service_name = obs_data_get_string(settings, "service");
	}

	std::optional<std::string> custom_rtmp_url;
	auto server = obs_data_get_string(settings, "server");
	if (strcmp(server, "auto") != 0) {
		custom_rtmp_url = server;
	}

	auto service_custom_server = obs_data_get_bool(settings, "using_custom_server");
	if (custom_rtmp_url.has_value()) {
		blog(LOG_INFO, "Using %sserver '%s'", service_custom_server ? "custom " : "", custom_rtmp_url->c_str());
	}

	auto maximum_aggregate_bitrate =
		config_get_bool(main->Config(), "Stream1", "MultitrackVideoMaximumAggregateBitrateAuto")
			? std::nullopt
			: std::make_optional<uint32_t>(
				  config_get_int(main->Config(), "Stream1", "MultitrackVideoMaximumAggregateBitrate"));

	auto maximum_video_tracks = config_get_bool(main->Config(), "Stream1", "MultitrackVideoMaximumVideoTracksAuto")
					    ? std::nullopt
					    : std::make_optional<uint32_t>(config_get_int(
						      main->Config(), "Stream1", "MultitrackVideoMaximumVideoTracks"));

	auto stream_dump_config = GenerateMultitrackVideoStreamDumpConfig();

	auto continue_on_main_thread = [&, start_streaming_guard, service = OBSService{service},
					continuation =
						std::move(continuation)](std::optional<MultitrackVideoError> error) {
		if (error) {
			OBSDataAutoRelease service_settings = obs_service_get_settings(service);
			auto multitrack_video_name = QTStr("Basic.Settings.Stream.MultitrackVideoLabel");
			if (obs_data_has_user_value(service_settings, "multitrack_video_name")) {
				multitrack_video_name = obs_data_get_string(service_settings, "multitrack_video_name");
			}

			multitrackVideoActive = false;
			if (!error->ShowDialog(main, multitrack_video_name))
				return continuation(false);
			return continuation(std::nullopt);
		}

		multitrackVideoActive = true;

		auto signal_handler = multitrackVideo->StreamingSignalHandler();

		streamDelayStarting.Connect(signal_handler, "starting", OBSStreamStarting, this);
		streamStopping.Connect(signal_handler, "stopping", OBSStreamStopping, this);

		startStreaming.Connect(signal_handler, "start", OBSStartStreaming, this);
		stopStreaming.Connect(signal_handler, "stop", OBSStopStreaming, this);
		return continuation(true);
	};

	QThreadPool::globalInstance()->start([=, multitrackVideo = multitrackVideo.get(),
					      service_name = std::string{service_name}, service = OBSService{service},
					      stream_dump_config = OBSData{stream_dump_config},
					      start_streaming_guard = start_streaming_guard]() mutable {
		std::optional<MultitrackVideoError> error;
		try {
			multitrackVideo->PrepareStreaming(main, service_name.c_str(), service, custom_rtmp_url, key,
							  audio_encoder_id.c_str(), maximum_aggregate_bitrate,
							  maximum_video_tracks, custom_config, stream_dump_config,
							  main_audio_mixer, vod_track_mixer);
		} catch (const MultitrackVideoError &error_) {
			error.emplace(error_);
		}

		QMetaObject::invokeMethod(main, [=] { continue_on_main_thread(error); });
	});

	return start_streaming_guard->GetFuture();
}

OBSDataAutoRelease BasicOutputHandler::GenerateMultitrackVideoStreamDumpConfig()
{
	auto stream_dump_enabled = config_get_bool(main->Config(), "Stream1", "MultitrackVideoStreamDumpEnabled");

	if (!stream_dump_enabled)
		return nullptr;

	const char *path = config_get_string(main->Config(), "SimpleOutput", "FilePath");
	bool noSpace = config_get_bool(main->Config(), "SimpleOutput", "FileNameWithoutSpace");
	const char *filenameFormat = config_get_string(main->Config(), "Output", "FilenameFormatting");
	bool overwriteIfExists = config_get_bool(main->Config(), "Output", "OverwriteIfExists");
	bool useMP4 = config_get_bool(main->Config(), "Stream1", "MultitrackVideoStreamDumpAsMP4");

	string f;

	OBSDataAutoRelease settings = obs_data_create();
	f = GetFormatString(filenameFormat, nullptr, nullptr);
	string strPath = GetRecordingFilename(path, useMP4 ? "mp4" : "flv", noSpace, overwriteIfExists, f.c_str(),
					      // never remux stream dump
					      false);
	obs_data_set_string(settings, "path", strPath.c_str());

	if (useMP4) {
		obs_data_set_bool(settings, "use_mp4", true);
		obs_data_set_string(settings, "muxer_settings", "write_encoder_info=1");
	}

	return settings;
}

BasicOutputHandler *CreateSimpleOutputHandler(OBSBasic *main)
{
	return new SimpleOutput(main);
}

BasicOutputHandler *CreateAdvancedOutputHandler(OBSBasic *main)
{
	return new AdvancedOutput(main);
}
