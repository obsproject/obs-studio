#include <string>
#include <QMessageBox>
#include "audio-encoders.hpp"
#include "window-basic-main.hpp"
#include "window-basic-main-outputs.hpp"

using namespace std;

static void OBSStreamStarting(void *data, calldata_t *params)
{
	BasicOutputHandler *output = static_cast<BasicOutputHandler*>(data);
	obs_output_t *obj = (obs_output_t*)calldata_ptr(params, "output");

	int sec = (int)obs_output_get_active_delay(obj);
	if (sec == 0)
		return;

	output->delayActive = true;
	QMetaObject::invokeMethod(output->main,
			"StreamDelayStarting", Q_ARG(int, sec));
}

static void OBSStreamStopping(void *data, calldata_t *params)
{
	BasicOutputHandler *output = static_cast<BasicOutputHandler*>(data);
	obs_output_t *obj = (obs_output_t*)calldata_ptr(params, "output");

	int sec = (int)obs_output_get_active_delay(obj);
	if (sec == 0)
		return;

	QMetaObject::invokeMethod(output->main,
			"StreamDelayStopping", Q_ARG(int, sec));
}

static void OBSStartStreaming(void *data, calldata_t *params)
{
	BasicOutputHandler *output = static_cast<BasicOutputHandler*>(data);
	output->streamingActive = true;
	QMetaObject::invokeMethod(output->main, "StreamingStart");

	UNUSED_PARAMETER(params);
}

static void OBSStopStreaming(void *data, calldata_t *params)
{
	BasicOutputHandler *output = static_cast<BasicOutputHandler*>(data);
	int code = (int)calldata_int(params, "code");

	output->streamingActive = false;
	output->delayActive = false;
	QMetaObject::invokeMethod(output->main,
			"StreamingStop", Q_ARG(int, code));
}

static void OBSStartRecording(void *data, calldata_t *params)
{
	BasicOutputHandler *output = static_cast<BasicOutputHandler*>(data);

	output->recordingActive = true;
	QMetaObject::invokeMethod(output->main, "RecordingStart");

	UNUSED_PARAMETER(params);
}

static void OBSStopRecording(void *data, calldata_t *params)
{
	BasicOutputHandler *output = static_cast<BasicOutputHandler*>(data);
	int code = (int)calldata_int(params, "code");

	output->recordingActive = false;
	QMetaObject::invokeMethod(output->main,
			"RecordingStop", Q_ARG(int, code));

	UNUSED_PARAMETER(params);
}

/* ------------------------------------------------------------------------ */

static bool CreateAACEncoder(OBSEncoder &res, string &id, int bitrate,
		const char *name, size_t idx)
{
	const char *id_ = GetAACEncoderForBitrate(bitrate);
	if (!id_) {
		id.clear();
		res = nullptr;
		return false;
	}

	if (id == id_)
		return true;

	id = id_;
	res = obs_audio_encoder_create(id_, name, nullptr, idx, nullptr);

	if (res) {
		obs_encoder_release(res);
		return true;
	}

	return false;
}

/* ------------------------------------------------------------------------ */

struct SimpleOutput : BasicOutputHandler {
	OBSEncoder             aacStreaming;
	OBSEncoder             h264Streaming;
	OBSEncoder             aacRecording;
	OBSEncoder             h264Recording;

	string                 aacRecEncID;
	string                 aacStreamEncID;

	string                 videoEncoder;
	string                 videoQuality;
	bool                   usingRecordingPreset = false;
	bool                   ffmpegOutput = false;
	bool                   lowCPUx264 = false;

	SimpleOutput(OBSBasic *main_);

	int CalcCRF(int crf);

	void UpdateRecordingSettings_x264_crf(int crf);
	void UpdateRecordingSettings();
	void UpdateRecordingAudioSettings();
	virtual void Update() override;

	void SetupOutputs();
	int GetAudioBitrate() const;

	void LoadRecordingPreset_x264();
	void LoadRecordingPreset_Lossless();
	void LoadRecordingPreset();

	virtual bool StartStreaming(obs_service_t *service) override;
	virtual bool StartRecording() override;
	virtual void StopStreaming() override;
	virtual void ForceStopStreaming() override;
	virtual void StopRecording() override;
	virtual bool StreamingActive() const override;
	virtual bool RecordingActive() const override;
};

void SimpleOutput::LoadRecordingPreset_Lossless()
{
	fileOutput = obs_output_create("ffmpeg_output",
			"simple_ffmpeg_output", nullptr, nullptr);
	if (!fileOutput)
		throw "Failed to create recording FFmpeg output "
		      "(simple output)";
	obs_output_release(fileOutput);

	obs_data_t *settings = obs_data_create();
	obs_data_set_string(settings, "format_name", "avi");
	obs_data_set_string(settings, "video_encoder", "utvideo");
	obs_data_set_string(settings, "audio_encoder", "pcm_s16le");

	obs_output_update(fileOutput, settings);
	obs_data_release(settings);
}

void SimpleOutput::LoadRecordingPreset_x264()
{
	h264Recording = obs_video_encoder_create("obs_x264",
			"simple_h264_recording", nullptr, nullptr);
	if (!h264Recording)
		throw "Failed to create h264 recording encoder (simple output)";
	obs_encoder_release(h264Recording);

	if (!CreateAACEncoder(aacRecording, aacRecEncID, 192,
				"simple_aac_recording", 0))
		throw "Failed to create aac recording encoder (simple output)";
}

void SimpleOutput::LoadRecordingPreset()
{
	const char *quality = config_get_string(main->Config(), "SimpleOutput",
			"RecQuality");
	const char *encoder = config_get_string(main->Config(), "SimpleOutput",
			"RecEncoder");

	videoEncoder = encoder;
	videoQuality = quality;
	ffmpegOutput = false;

	if (strcmp(quality, "Stream") == 0) {
		h264Recording = h264Streaming;
		aacRecording = aacStreaming;
		usingRecordingPreset = false;
		return;

	} else if (strcmp(quality, "Lossless") == 0) {
		LoadRecordingPreset_Lossless();
		usingRecordingPreset = true;
		ffmpegOutput = true;
		return;

	} else {
		lowCPUx264  = strcmp(encoder, SIMPLE_ENCODER_X264_LOWCPU) == 0;
		LoadRecordingPreset_x264();
		usingRecordingPreset = true;
	}
}

SimpleOutput::SimpleOutput(OBSBasic *main_) : BasicOutputHandler(main_)
{
	streamOutput = obs_output_create("rtmp_output", "simple_stream",
			nullptr, nullptr);
	if (!streamOutput)
		throw "Failed to create stream output (simple output)";
	obs_output_release(streamOutput);

	h264Streaming = obs_video_encoder_create("obs_x264",
			"simple_h264_stream", nullptr, nullptr);
	if (!h264Streaming)
		throw "Failed to create h264 streaming encoder (simple output)";
	obs_encoder_release(h264Streaming);

	if (!CreateAACEncoder(aacStreaming, aacStreamEncID, GetAudioBitrate(),
				"simple_aac", 0))
		throw "Failed to create aac streaming encoder (simple output)";

	streamDelayStarting.Connect(obs_output_get_signal_handler(streamOutput),
			"starting", OBSStreamStarting, this);
	streamDelayStopping.Connect(obs_output_get_signal_handler(streamOutput),
			"stopping", OBSStreamStopping, this);

	startStreaming.Connect(obs_output_get_signal_handler(streamOutput),
			"start", OBSStartStreaming, this);
	stopStreaming.Connect(obs_output_get_signal_handler(streamOutput),
			"stop", OBSStopStreaming, this);

	LoadRecordingPreset();

	if (!ffmpegOutput) {
		fileOutput = obs_output_create("ffmpeg_muxer",
				"simple_file_output", nullptr, nullptr);
		if (!fileOutput)
			throw "Failed to create recording output "
			      "(simple output)";
		obs_output_release(fileOutput);
	}

	startRecording.Connect(obs_output_get_signal_handler(fileOutput),
			"start", OBSStartRecording, this);
	stopRecording.Connect(obs_output_get_signal_handler(fileOutput),
			"stop", OBSStopRecording, this);
}

int SimpleOutput::GetAudioBitrate() const
{
	int bitrate = (int)config_get_uint(main->Config(), "SimpleOutput",
			"ABitrate");

	return FindClosestAvailableAACBitrate(bitrate);
}

void SimpleOutput::Update()
{
	obs_data_t *h264Settings = obs_data_create();
	obs_data_t *aacSettings  = obs_data_create();

	int videoBitrate = config_get_uint(main->Config(), "SimpleOutput",
			"VBitrate");
	int audioBitrate = GetAudioBitrate();
	bool advanced = config_get_bool(main->Config(), "SimpleOutput",
			"UseAdvanced");
	const char *preset = config_get_string(main->Config(),
			"SimpleOutput", "Preset");
	const char *custom = config_get_string(main->Config(),
			"SimpleOutput", "x264Settings");

	obs_data_set_int(h264Settings, "bitrate", videoBitrate);

	if (advanced) {
		obs_data_set_string(h264Settings, "preset", preset);
		obs_data_set_string(h264Settings, "x264opts", custom);
	}

	obs_data_set_bool(aacSettings, "cbr", true);
	obs_data_set_int(aacSettings, "bitrate", audioBitrate);

	obs_service_apply_encoder_settings(main->GetService(),
			h264Settings, aacSettings);

	video_t *video = obs_get_video();
	enum video_format format = video_output_get_format(video);

	if (format != VIDEO_FORMAT_NV12 && format != VIDEO_FORMAT_I420)
		obs_encoder_set_preferred_video_format(h264Streaming,
				VIDEO_FORMAT_NV12);

	obs_encoder_update(h264Streaming, h264Settings);
	obs_encoder_update(aacStreaming,  aacSettings);

	obs_data_release(h264Settings);
	obs_data_release(aacSettings);
}

void SimpleOutput::UpdateRecordingAudioSettings()
{
	obs_data_t *settings = obs_data_create();
	obs_data_set_int(settings, "bitrate", 192);
	obs_data_set_bool(settings, "cbr", true);

	obs_encoder_update(aacRecording, settings);

	obs_data_release(settings);
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
	double crfResReduction =
		fmin(CROSS_DIST_CUTOFF, crossDist) / CROSS_DIST_CUTOFF;
	crfResReduction = (1.0 - crfResReduction) * 10.0;

	return crf - int(crfResReduction);
}

void SimpleOutput::UpdateRecordingSettings_x264_crf(int crf)
{
	obs_data_t *settings = obs_data_create();
	obs_data_set_int(settings, "bitrate", 1000);
	obs_data_set_int(settings, "buffer_size", 0);
	obs_data_set_int(settings, "crf", crf);
	obs_data_set_bool(settings, "use_bufsize", true);
	obs_data_set_bool(settings, "cbr", false);
	obs_data_set_string(settings, "profile", "high");
	obs_data_set_string(settings, "preset",
			lowCPUx264 ? "ultrafast" : "veryfast");

	obs_encoder_update(h264Recording, settings);

	obs_data_release(settings);
}

void SimpleOutput::UpdateRecordingSettings()
{
	if (astrcmp_n(videoEncoder.c_str(), "x264", 4) == 0) {
		if (videoQuality == "Small")
			UpdateRecordingSettings_x264_crf(CalcCRF(23));
		else if (videoQuality == "HQ")
			UpdateRecordingSettings_x264_crf(CalcCRF(16));
	}
}

inline void SimpleOutput::SetupOutputs()
{
	SimpleOutput::Update();
	obs_encoder_set_video(h264Streaming, obs_get_video());
	obs_encoder_set_audio(aacStreaming,  obs_get_audio());

	if (usingRecordingPreset) {
		if (ffmpegOutput) {
			obs_output_set_media(fileOutput, obs_get_video(),
					obs_get_audio());
		} else {
			obs_encoder_set_video(h264Recording, obs_get_video());
			obs_encoder_set_audio(aacRecording,  obs_get_audio());
		}
	}
}

bool SimpleOutput::StartStreaming(obs_service_t *service)
{
	if (!Active())
		SetupOutputs();

	obs_output_set_video_encoder(streamOutput, h264Streaming);
	obs_output_set_audio_encoder(streamOutput, aacStreaming, 0);
	obs_output_set_service(streamOutput, service);

	bool reconnect = config_get_bool(main->Config(), "Output",
			"Reconnect");
	int retryDelay = config_get_uint(main->Config(), "Output",
			"RetryDelay");
	int maxRetries = config_get_uint(main->Config(), "Output",
			"MaxRetries");
	bool useDelay = config_get_bool(main->Config(), "Output",
			"DelayEnable");
	int delaySec = config_get_int(main->Config(), "Output",
			"DelaySec");
	bool preserveDelay = config_get_bool(main->Config(), "Output",
			"DelayPreserve");
	if (!reconnect)
		maxRetries = 0;

	obs_output_set_delay(streamOutput, useDelay ? delaySec : 0,
			preserveDelay ? OBS_OUTPUT_DELAY_PRESERVE : 0);

	obs_output_set_reconnect_settings(streamOutput, maxRetries,
			retryDelay);

	if (obs_output_start(streamOutput)) {
		return true;
	}

	return false;
}

bool SimpleOutput::StartRecording()
{
	if (usingRecordingPreset) {
		if (!ffmpegOutput)
			UpdateRecordingSettings();
	} else if (!obs_output_active(streamOutput)) {
		Update();
	}

	if (!Active())
		SetupOutputs();

	const char *path = config_get_string(main->Config(),
			"SimpleOutput", "FilePath");
	const char *format = config_get_string(main->Config(),
			"SimpleOutput", "RecFormat");
	const char *mux = config_get_string(main->Config(), "SimpleOutput",
			"MuxerCustom");
	bool noSpace = config_get_bool(main->Config(), "SimpleOutput",
			"FileNameWithoutSpace");

	os_dir_t *dir = path ? os_opendir(path) : nullptr;

	if (!dir) {
		QMessageBox::information(main,
				QTStr("Output.BadPath.Title"),
				QTStr("Output.BadPath.Text"));
		return false;
	}

	os_closedir(dir);

	string strPath;
	strPath += path;

	char lastChar = strPath.back();
	if (lastChar != '/' && lastChar != '\\')
		strPath += "/";

	strPath += GenerateTimeDateFilename(ffmpegOutput ? "avi" : format,
			noSpace);

	SetupOutputs();

	if (!ffmpegOutput) {
		obs_output_set_video_encoder(fileOutput, h264Recording);
		obs_output_set_audio_encoder(fileOutput, aacRecording, 0);
	}

	obs_data_t *settings = obs_data_create();
	obs_data_set_string(settings, ffmpegOutput ? "url" : "path",
			strPath.c_str());
	obs_data_set_string(settings, "muxer_settings", mux);

	obs_output_update(fileOutput, settings);

	obs_data_release(settings);

	if (obs_output_start(fileOutput)) {
		return true;
	}

	return false;
}

void SimpleOutput::StopStreaming()
{
	obs_output_stop(streamOutput);
}

void SimpleOutput::ForceStopStreaming()
{
	obs_output_force_stop(streamOutput);
}

void SimpleOutput::StopRecording()
{
	obs_output_stop(fileOutput);
}

bool SimpleOutput::StreamingActive() const
{
	return obs_output_active(streamOutput);
}

bool SimpleOutput::RecordingActive() const
{
	return obs_output_active(fileOutput);
}

/* ------------------------------------------------------------------------ */

struct AdvancedOutput : BasicOutputHandler {
	OBSEncoder             aacTrack[4];
	OBSEncoder             h264Streaming;
	OBSEncoder             h264Recording;

	bool                   ffmpegOutput;
	bool                   ffmpegRecording;
	bool                   useStreamEncoder;

	string                 aacEncoderID[4];

	AdvancedOutput(OBSBasic *main_);

	inline void UpdateStreamSettings();
	inline void UpdateRecordingSettings();
	inline void UpdateAudioSettings();
	virtual void Update() override;

	inline void SetupStreaming();
	inline void SetupRecording();
	inline void SetupFFmpeg();
	void SetupOutputs();
	int GetAudioBitrate(size_t i) const;

	virtual bool StartStreaming(obs_service_t *service) override;
	virtual bool StartRecording() override;
	virtual void StopStreaming() override;
	virtual void ForceStopStreaming() override;
	virtual void StopRecording() override;
	virtual bool StreamingActive() const override;
	virtual bool RecordingActive() const override;
};

static OBSData GetDataFromJsonFile(const char *jsonFile)
{
	char fullPath[512];

	int ret = GetProfilePath(fullPath, sizeof(fullPath), jsonFile);
	if (ret > 0) {
		BPtr<char> jsonData = os_quick_read_utf8_file(fullPath);
		if (!!jsonData) {
			obs_data_t *data = obs_data_create_from_json(jsonData);
			OBSData dataRet(data);
			obs_data_release(data);
			return dataRet;
		}
	}

	return nullptr;
}

AdvancedOutput::AdvancedOutput(OBSBasic *main_) : BasicOutputHandler(main_)
{
	const char *recType = config_get_string(main->Config(), "AdvOut",
			"RecType");
	const char *streamEncoder = config_get_string(main->Config(), "AdvOut",
			"Encoder");
	const char *recordEncoder = config_get_string(main->Config(), "AdvOut",
			"RecEncoder");

	ffmpegOutput = astrcmpi(recType, "FFmpeg") == 0;
	ffmpegRecording = ffmpegOutput &&
		config_get_bool(main->Config(), "AdvOut", "FFOutputToFile");
	useStreamEncoder = astrcmpi(recordEncoder, "none") == 0;

	OBSData streamEncSettings = GetDataFromJsonFile("streamEncoder.json");
	OBSData recordEncSettings = GetDataFromJsonFile("recordEncoder.json");

	streamOutput = obs_output_create("rtmp_output", "adv_stream",
			nullptr, nullptr);
	if (!streamOutput)
		throw "Failed to create stream output (advanced output)";
	obs_output_release(streamOutput);

	if (ffmpegOutput) {
		fileOutput = obs_output_create("ffmpeg_output",
				"adv_ffmpeg_output", nullptr, nullptr);
		if (!fileOutput)
			throw "Failed to create recording FFmpeg output "
			      "(advanced output)";
		obs_output_release(fileOutput);
	} else {
		fileOutput = obs_output_create("ffmpeg_muxer",
				"adv_file_output", nullptr, nullptr);
		if (!fileOutput)
			throw "Failed to create recording output "
			      "(advanced output)";
		obs_output_release(fileOutput);

		if (!useStreamEncoder) {
			h264Recording = obs_video_encoder_create(recordEncoder,
					"recording_h264", recordEncSettings,
					nullptr);
			if (!h264Recording)
				throw "Failed to create recording h264 "
				      "encoder (advanced output)";
			obs_encoder_release(h264Recording);
		}
	}

	h264Streaming = obs_video_encoder_create(streamEncoder,
			"streaming_h264", streamEncSettings, nullptr);
	if (!h264Streaming)
		throw "Failed to create streaming h264 encoder "
		      "(advanced output)";
	obs_encoder_release(h264Streaming);

	for (int i = 0; i < 4; i++) {
		char name[9];
		sprintf(name, "adv_aac%d", i);

		if (!CreateAACEncoder(aacTrack[i], aacEncoderID[i],
					GetAudioBitrate(i), name, i))
			throw "Failed to create audio encoder "
			      "(advanced output)";
	}

	streamDelayStarting.Connect(obs_output_get_signal_handler(streamOutput),
			"starting", OBSStreamStarting, this);
	streamDelayStopping.Connect(obs_output_get_signal_handler(streamOutput),
			"stopping", OBSStreamStopping, this);

	startStreaming.Connect(obs_output_get_signal_handler(streamOutput),
			"start", OBSStartStreaming, this);
	stopStreaming.Connect(obs_output_get_signal_handler(streamOutput),
			"stop", OBSStopStreaming, this);

	startRecording.Connect(obs_output_get_signal_handler(fileOutput),
			"start", OBSStartRecording, this);
	stopRecording.Connect(obs_output_get_signal_handler(fileOutput),
			"stop", OBSStopRecording, this);
}

void AdvancedOutput::UpdateStreamSettings()
{
	bool applyServiceSettings = config_get_bool(main->Config(), "AdvOut",
			"ApplyServiceSettings");

	OBSData settings = GetDataFromJsonFile("streamEncoder.json");

	if (applyServiceSettings)
		obs_service_apply_encoder_settings(main->GetService(),
				settings, nullptr);

	video_t *video = obs_get_video();
	enum video_format format = video_output_get_format(video);

	if (format != VIDEO_FORMAT_NV12 && format != VIDEO_FORMAT_I420)
		obs_encoder_set_preferred_video_format(h264Streaming,
				VIDEO_FORMAT_NV12);

	obs_encoder_update(h264Streaming, settings);
}

inline void AdvancedOutput::UpdateRecordingSettings()
{
	OBSData settings = GetDataFromJsonFile("recordEncoder.json");
	obs_encoder_update(h264Recording, settings);
}

void AdvancedOutput::Update()
{
	UpdateStreamSettings();
	if (!useStreamEncoder && !ffmpegOutput)
		UpdateRecordingSettings();
	UpdateAudioSettings();
}

inline void AdvancedOutput::SetupStreaming()
{
	bool rescale = config_get_bool(main->Config(), "AdvOut",
			"Rescale");
	const char *rescaleRes = config_get_string(main->Config(), "AdvOut",
			"RescaleRes");
	bool multitrack = config_get_bool(main->Config(), "AdvOut",
			"Multitrack");
	int trackIndex = config_get_int(main->Config(), "AdvOut",
			"TrackIndex");
	int trackCount = config_get_int(main->Config(), "AdvOut",
			"TrackCount");
	unsigned int cx = 0;
	unsigned int cy = 0;

	if (rescale && rescaleRes && *rescaleRes) {
		if (sscanf(rescaleRes, "%ux%u", &cx, &cy) != 2) {
			cx = 0;
			cy = 0;
		}
	}

	obs_encoder_set_scaled_size(h264Streaming, cx, cy);
	obs_encoder_set_video(h264Streaming, obs_get_video());

	obs_output_set_video_encoder(streamOutput, h264Streaming);

	if (multitrack) {
		int i = 0;
		for (; i < trackCount; i++)
			obs_output_set_audio_encoder(streamOutput, aacTrack[i],
					i);
		for (; i < 4; i++)
			obs_output_set_audio_encoder(streamOutput, nullptr, i);

	} else {
		obs_output_set_audio_encoder(streamOutput,
				aacTrack[trackIndex - 1], 0);
	}
}

inline void AdvancedOutput::SetupRecording()
{
	const char *path = config_get_string(main->Config(), "AdvOut",
			"RecFilePath");
	const char *mux = config_get_string(main->Config(), "AdvOut",
			"RecMuxerCustom");
	bool rescale = config_get_bool(main->Config(), "AdvOut",
			"RecRescale");
	const char *rescaleRes = config_get_string(main->Config(), "AdvOut",
			"RecRescaleRes");
	int tracks = config_get_int(main->Config(), "AdvOut", "RecTracks");
	obs_data_t *settings = obs_data_create();
	unsigned int cx = 0;
	unsigned int cy = 0;
	int idx = 0;

	if (useStreamEncoder) {
		obs_output_set_video_encoder(fileOutput, h264Streaming);
	} else {
		if (rescale && rescaleRes && *rescaleRes) {
			if (sscanf(rescaleRes, "%ux%u", &cx, &cy) != 2) {
				cx = 0;
				cy = 0;
			}
		}

		obs_encoder_set_scaled_size(h264Recording, cx, cy);
		obs_encoder_set_video(h264Recording, obs_get_video());
		obs_output_set_video_encoder(fileOutput, h264Recording);
	}

	for (int i = 0; i < MAX_AUDIO_MIXES; i++) {
		if ((tracks & (1<<i)) != 0) {
			obs_output_set_audio_encoder(fileOutput, aacTrack[i],
					idx++);
		}
	}

	obs_data_set_string(settings, "path", path);
	obs_data_set_string(settings, "muxer_settings", mux);
	obs_output_update(fileOutput, settings);
	obs_data_release(settings);
}

inline void AdvancedOutput::SetupFFmpeg()
{
	const char *url = config_get_string(main->Config(), "AdvOut", "FFURL");
	int vBitrate = config_get_int(main->Config(), "AdvOut",
			"FFVBitrate");
	bool rescale = config_get_bool(main->Config(), "AdvOut",
			"FFRescale");
	const char *rescaleRes = config_get_string(main->Config(), "AdvOut",
			"FFRescaleRes");
	const char *formatName = config_get_string(main->Config(), "AdvOut",
			"FFFormat");
	const char *mimeType = config_get_string(main->Config(), "AdvOut",
			"FFFormatMimeType");
	const char *muxCustom = config_get_string(main->Config(), "AdvOut",
			"FFMCustom");
	const char *vEncoder = config_get_string(main->Config(), "AdvOut",
			"FFVEncoder");
	int vEncoderId = config_get_int(main->Config(), "AdvOut",
			"FFVEncoderId");
	const char *vEncCustom = config_get_string(main->Config(), "AdvOut",
			"FFVCustom");
	int aBitrate = config_get_int(main->Config(), "AdvOut",
			"FFABitrate");
	int aTrack = config_get_int(main->Config(), "AdvOut",
			"FFAudioTrack");
	const char *aEncoder = config_get_string(main->Config(), "AdvOut",
			"FFAEncoder");
	int aEncoderId = config_get_int(main->Config(), "AdvOut",
			"FFAEncoderId");
	const char *aEncCustom = config_get_string(main->Config(), "AdvOut",
			"FFACustom");
	obs_data_t *settings = obs_data_create();

	obs_data_set_string(settings, "url", url);
	obs_data_set_string(settings, "format_name", formatName);
	obs_data_set_string(settings, "format_mime_type", mimeType);
	obs_data_set_string(settings, "muxer_settings", muxCustom);
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

	obs_output_set_mixer(fileOutput, aTrack - 1);
	obs_output_set_media(fileOutput, obs_get_video(), obs_get_audio());
	obs_output_update(fileOutput, settings);

	obs_data_release(settings);
}

static inline void SetEncoderName(obs_encoder_t *encoder, const char *name,
		const char *defaultName)
{
	obs_encoder_set_name(encoder, (name && *name) ? name : defaultName);
}

inline void AdvancedOutput::UpdateAudioSettings()
{
	const char *name1 = config_get_string(main->Config(), "AdvOut",
			"Track1Name");
	const char *name2 = config_get_string(main->Config(), "AdvOut",
			"Track2Name");
	const char *name3 = config_get_string(main->Config(), "AdvOut",
			"Track3Name");
	const char *name4 = config_get_string(main->Config(), "AdvOut",
			"Track4Name");
	bool applyServiceSettings = config_get_bool(main->Config(), "AdvOut",
			"ApplyServiceSettings");
	int streamTrackIndex = config_get_int(main->Config(), "AdvOut",
			"TrackIndex");
	obs_data_t *settings[4];

	for (size_t i = 0; i < 4; i++) {
		settings[i] = obs_data_create();
		obs_data_set_int(settings[i], "bitrate", GetAudioBitrate(i));
	}

	SetEncoderName(aacTrack[0], name1, "Track1");
	SetEncoderName(aacTrack[1], name2, "Track2");
	SetEncoderName(aacTrack[2], name3, "Track3");
	SetEncoderName(aacTrack[3], name4, "Track4");

	for (size_t i = 0; i < 4; i++) {
		if (applyServiceSettings && (int)(i + 1) == streamTrackIndex)
			obs_service_apply_encoder_settings(main->GetService(),
					nullptr, settings[i]);

		obs_encoder_update(aacTrack[i], settings[i]);
		obs_data_release(settings[i]);
	}
}

void AdvancedOutput::SetupOutputs()
{
	obs_encoder_set_video(h264Streaming, obs_get_video());
	if (h264Recording)
		obs_encoder_set_video(h264Recording, obs_get_video());
	obs_encoder_set_audio(aacTrack[0], obs_get_audio());
	obs_encoder_set_audio(aacTrack[1], obs_get_audio());
	obs_encoder_set_audio(aacTrack[2], obs_get_audio());
	obs_encoder_set_audio(aacTrack[3], obs_get_audio());

	SetupStreaming();

	if (ffmpegOutput)
		SetupFFmpeg();
	else
		SetupRecording();
}

int AdvancedOutput::GetAudioBitrate(size_t i) const
{
	const char *names[] = {
		"Track1Bitrate", "Track2Bitrate",
		"Track3Bitrate", "Track4Bitrate",
	};
	int bitrate = (int)config_get_uint(main->Config(), "AdvOut", names[i]);
	return FindClosestAvailableAACBitrate(bitrate);
}

bool AdvancedOutput::StartStreaming(obs_service_t *service)
{
	if (!useStreamEncoder ||
	    (!ffmpegOutput && !obs_output_active(fileOutput))) {
		UpdateStreamSettings();
	}

	UpdateAudioSettings();

	if (!Active())
		SetupOutputs();

	obs_output_set_service(streamOutput, service);

	bool reconnect = config_get_bool(main->Config(), "Output", "Reconnect");
	int retryDelay = config_get_int(main->Config(), "Output", "RetryDelay");
	int maxRetries = config_get_int(main->Config(), "Output", "MaxRetries");
	bool useDelay = config_get_bool(main->Config(), "Output",
			"DelayEnable");
	int delaySec = config_get_int(main->Config(), "Output",
			"DelaySec");
	bool preserveDelay = config_get_bool(main->Config(), "Output",
			"DelayPreserve");
	if (!reconnect)
		maxRetries = 0;

	obs_output_set_delay(streamOutput, useDelay ? delaySec : 0,
			preserveDelay ? OBS_OUTPUT_DELAY_PRESERVE : 0);

	obs_output_set_reconnect_settings(streamOutput, maxRetries,
			retryDelay);

	if (obs_output_start(streamOutput)) {
		return true;
	}

	return false;
}

bool AdvancedOutput::StartRecording()
{
	const char *path;
	const char *format;
	bool noSpace = false;

	if (!useStreamEncoder) {
		if (!ffmpegOutput) {
			UpdateRecordingSettings();
		}
	} else if (!obs_output_active(streamOutput)) {
		UpdateStreamSettings();
	}

	UpdateAudioSettings();

	if (!Active())
		SetupOutputs();

	if (!ffmpegOutput || ffmpegRecording) {
		path = config_get_string(main->Config(), "AdvOut",
				ffmpegRecording ? "FFFilePath" : "RecFilePath");
		format = config_get_string(main->Config(), "AdvOut",
				ffmpegRecording ? "FFExtension" : "RecFormat");
		noSpace = config_get_bool(main->Config(), "AdvOut",
				ffmpegRecording ?
				"FFFileNameWithoutSpace" :
				"RecFileNameWithoutSpace");

		os_dir_t *dir = path ? os_opendir(path) : nullptr;

		if (!dir) {
			QMessageBox::information(main,
					QTStr("Output.BadPath.Title"),
					QTStr("Output.BadPath.Text"));
			return false;
		}

		os_closedir(dir);

		string strPath;
		strPath += path;

		char lastChar = strPath.back();
		if (lastChar != '/' && lastChar != '\\')
			strPath += "/";

		strPath += GenerateTimeDateFilename(format, noSpace);

		obs_data_t *settings = obs_data_create();
		obs_data_set_string(settings,
				ffmpegRecording ? "url" : "path",
				strPath.c_str());

		obs_output_update(fileOutput, settings);

		obs_data_release(settings);
	}

	if (obs_output_start(fileOutput)) {
		return true;
	}

	return false;
}

void AdvancedOutput::StopStreaming()
{
	obs_output_stop(streamOutput);
}

void AdvancedOutput::ForceStopStreaming()
{
	obs_output_force_stop(streamOutput);
}

void AdvancedOutput::StopRecording()
{
	obs_output_stop(fileOutput);
}

bool AdvancedOutput::StreamingActive() const
{
	return obs_output_active(streamOutput);
}

bool AdvancedOutput::RecordingActive() const
{
	return obs_output_active(fileOutput);
}

/* ------------------------------------------------------------------------ */

BasicOutputHandler *CreateSimpleOutputHandler(OBSBasic *main)
{
	return new SimpleOutput(main);
}

BasicOutputHandler *CreateAdvancedOutputHandler(OBSBasic *main)
{
	return new AdvancedOutput(main);
}
