#include <string>
#include <QMessageBox>
#include "audio-encoders.hpp"
#include "window-basic-main.hpp"
#include "window-basic-main-outputs.hpp"

using namespace std;

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
	OBSEncoder             aac;
	OBSEncoder             h264;

	string                 aacEncoderID;

	SimpleOutput(OBSBasic *main_);

	virtual void Update() override;

	void SetupOutputs();
	int GetAudioBitrate() const;

	virtual bool StartStreaming(obs_service_t *service) override;
	virtual bool StartRecording() override;
	virtual void StopStreaming() override;
	virtual void StopRecording() override;
	virtual bool StreamingActive() const override;
	virtual bool RecordingActive() const override;
};

SimpleOutput::SimpleOutput(OBSBasic *main_) : BasicOutputHandler(main_)
{
	streamOutput = obs_output_create("rtmp_output", "simple_stream",
			nullptr, nullptr);
	if (!streamOutput)
		throw "Failed to create stream output (simple output)";
	obs_output_release(streamOutput);

	fileOutput = obs_output_create("ffmpeg_muxer", "simple_file_output",
			nullptr, nullptr);
	if (!fileOutput)
		throw "Failed to create recording output (simple output)";
	obs_output_release(fileOutput);

	h264 = obs_video_encoder_create("obs_x264", "simple_h264", nullptr,
			nullptr);
	if (!h264)
		throw "Failed to create h264 encoder (simple output)";
	obs_encoder_release(h264);

	if (!CreateAACEncoder(aac, aacEncoderID, GetAudioBitrate(),
				"simple_aac", 0))
		throw "Failed to create audio encoder (simple output)";

	startStreaming.Connect(obs_output_get_signal_handler(streamOutput),
			"start", OBSStartStreaming, this);
	stopStreaming.Connect(obs_output_get_signal_handler(streamOutput),
			"stop", OBSStopStreaming, this);

	startRecording.Connect(obs_output_get_signal_handler(fileOutput),
			"start", OBSStartRecording, this);
	stopRecording.Connect(obs_output_get_signal_handler(fileOutput),
			"stop", OBSStopRecording, this);
}

int SimpleOutput::GetAudioBitrate() const
{
	return config_get_uint(main->Config(), "SimpleOutput", "ABitrate");
}

void SimpleOutput::Update()
{
	obs_data_t *h264Settings = obs_data_create();
	obs_data_t *aacSettings  = obs_data_create();

	int videoBitrate = config_get_uint(main->Config(), "SimpleOutput",
			"VBitrate");
	int videoBufsize = config_get_uint(main->Config(), "SimpleOutput",
			"VBufsize");
	int audioBitrate = GetAudioBitrate();
	bool advanced = config_get_bool(main->Config(), "SimpleOutput",
			"UseAdvanced");
	bool useCBR = config_get_bool(main->Config(), "SimpleOutput",
			"UseCBR");
	bool useBufsize = config_get_bool(main->Config(), "SimpleOutput",
			"UseBufsize");
	const char *preset = config_get_string(main->Config(),
			"SimpleOutput", "Preset");
	const char *custom = config_get_string(main->Config(),
			"SimpleOutput", "x264Settings");

	obs_data_set_int(h264Settings, "bitrate", videoBitrate);
	obs_data_set_bool(h264Settings, "use_bufsize", useBufsize);
	obs_data_set_int(h264Settings, "buffer_size", videoBufsize);

	if (advanced) {
		obs_data_set_string(h264Settings, "preset", preset);
		obs_data_set_string(h264Settings, "x264opts", custom);
		obs_data_set_bool(h264Settings, "cbr", useCBR);
	} else {
		obs_data_set_bool(h264Settings, "cbr", true);
	}

	obs_data_set_int(aacSettings, "bitrate", audioBitrate);

	obs_service_apply_encoder_settings(main->GetServices()[0],
			h264Settings, aacSettings);

	video_t *video = obs_get_video();
	enum video_format format = video_output_get_format(video);

	if (format != VIDEO_FORMAT_NV12 && format != VIDEO_FORMAT_I420)
		obs_encoder_set_preferred_video_format(h264, VIDEO_FORMAT_NV12);

	obs_encoder_update(h264, h264Settings);
	obs_encoder_update(aac,  aacSettings);

	obs_data_release(h264Settings);
	obs_data_release(aacSettings);
}

inline void SimpleOutput::SetupOutputs()
{
	SimpleOutput::Update();
	obs_encoder_set_video(h264, obs_get_video());
	obs_encoder_set_audio(aac,  obs_get_audio());
}

bool SimpleOutput::StartStreaming(obs_service_t *service)
{
	if (!Active())
		SetupOutputs();

	obs_output_set_video_encoder(streamOutput, h264);
	obs_output_set_audio_encoder(streamOutput, aac, 0);
	obs_output_set_service(streamOutput, service);

	bool reconnect = config_get_bool(main->Config(), "SimpleOutput",
			"Reconnect");
	int retryDelay = config_get_uint(main->Config(), "SimpleOutput",
			"RetryDelay");
	int maxRetries = config_get_uint(main->Config(), "SimpleOutput",
			"MaxRetries");
	if (!reconnect)
		maxRetries = 0;

	obs_output_set_reconnect_settings(streamOutput, maxRetries,
			retryDelay);

	if (obs_output_start(streamOutput)) {
		return true;
	}

	return false;
}

bool SimpleOutput::StartRecording()
{
	if (!Active())
		SetupOutputs();

	const char *path = config_get_string(main->Config(),
			"SimpleOutput", "FilePath");
	const char *format = config_get_string(main->Config(),
			"SimpleOutput", "RecFormat");

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

	strPath += GenerateTimeDateFilename(format);

	SetupOutputs();

	obs_output_set_video_encoder(fileOutput, h264);
	obs_output_set_audio_encoder(fileOutput, aac, 0);

	obs_data_t *settings = obs_data_create();
	obs_data_set_string(settings, "path", strPath.c_str());

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
		obs_service_apply_encoder_settings(main->GetServices()[0],
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
			obs_service_apply_encoder_settings(
					main->GetServices()[0],
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
	return config_get_uint(main->Config(), "AdvOut", names[i]);
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

	bool reconnect = config_get_bool(main->Config(), "AdvOut", "Reconnect");
	int retryDelay = config_get_int(main->Config(), "AdvOut", "RetryDelay");
	int maxRetries = config_get_int(main->Config(), "AdvOut", "MaxRetries");
	if (!reconnect)
		maxRetries = 0;

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

		strPath += GenerateTimeDateFilename(format);

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
