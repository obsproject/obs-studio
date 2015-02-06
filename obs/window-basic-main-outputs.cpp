#include <string>
#include <QMessageBox>
#include "window-basic-main.hpp"
#include "window-basic-main-outputs.hpp"

using namespace std;

static void OBSStartStreaming(void *data, calldata_t *params)
{
	BasicOutputHandler *output = static_cast<BasicOutputHandler*>(data);
	QMetaObject::invokeMethod(output->main, "StreamingStart");

	UNUSED_PARAMETER(params);
}

static void OBSStopStreaming(void *data, calldata_t *params)
{
	BasicOutputHandler *output = static_cast<BasicOutputHandler*>(data);
	int code = (int)calldata_int(params, "code");

	QMetaObject::invokeMethod(output->main,
			"StreamingStop", Q_ARG(int, code));
	output->activeRefs--;
}

static void OBSStartRecording(void *data, calldata_t *params)
{
	BasicOutputHandler *output = static_cast<BasicOutputHandler*>(data);

	QMetaObject::invokeMethod(output->main, "RecordingStart");

	UNUSED_PARAMETER(params);
}

static void OBSStopRecording(void *data, calldata_t *params)
{
	BasicOutputHandler *output = static_cast<BasicOutputHandler*>(data);

	QMetaObject::invokeMethod(output->main, "RecordingStop");
	output->activeRefs--;

	UNUSED_PARAMETER(params);
}

/* ------------------------------------------------------------------------ */

struct SimpleOutput : BasicOutputHandler {
	OBSEncoder             aac;
	OBSEncoder             h264;

	SimpleOutput(OBSBasic *main_);

	virtual void Update() override;

	void SetupOutputs();

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
			nullptr);
	if (!streamOutput)
		throw "Failed to create stream output (simple output)";

	fileOutput = obs_output_create("flv_output", "simple_file_output",
			nullptr);
	if (!fileOutput)
		throw "Failed to create recording output (simple output)";

	h264 = obs_video_encoder_create("obs_x264", "simple_h264", nullptr);
	if (!h264)
		throw "Failed to create h264 encoder (simple output)";

	aac = obs_audio_encoder_create("libfdk_aac", "simple_aac", nullptr, 0);
	if (!aac)
		aac = obs_audio_encoder_create("ffmpeg_aac", "simple_aac",
				nullptr, 0);
	if (!aac)
		throw "Failed to create audio encoder (simple output)";

	signal_handler_connect(obs_output_get_signal_handler(streamOutput),
			"start", OBSStartStreaming, this);
	signal_handler_connect(obs_output_get_signal_handler(streamOutput),
			"stop", OBSStopStreaming, this);

	signal_handler_connect(obs_output_get_signal_handler(fileOutput),
			"start", OBSStartRecording, this);
	signal_handler_connect(obs_output_get_signal_handler(fileOutput),
			"stop", OBSStopRecording, this);
}

void SimpleOutput::Update()
{
	obs_data_t *h264Settings = obs_data_create();
	obs_data_t *aacSettings  = obs_data_create();

	int videoBitrate = config_get_uint(main->Config(), "SimpleOutput",
			"VBitrate");
	int videoBufsize = config_get_uint(main->Config(), "SimpleOutput",
			"Bufsize");
	int audioBitrate = config_get_uint(main->Config(), "SimpleOutput",
			"ABitrate");
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
		activeRefs++;
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

	strPath += GenerateTimeDateFilename("flv");

	SetupOutputs();

	obs_output_set_video_encoder(fileOutput, h264);
	obs_output_set_audio_encoder(fileOutput, aac, 0);

	obs_data_t *settings = obs_data_create();
	obs_data_set_string(settings, "path", strPath.c_str());

	obs_output_update(fileOutput, settings);

	obs_data_release(settings);

	if (obs_output_start(fileOutput)) {
		activeRefs++;
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

BasicOutputHandler *CreateSimpleOutputHandler(OBSBasic *main)
{
	return new SimpleOutput(main);
}
