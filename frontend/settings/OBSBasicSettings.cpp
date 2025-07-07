/*
 * Copyright (c) 2013-2021, Hugh Bailey <obs.jim@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <QMessageBox>
#include <QScrollBar>
#include <QPushButton>
#include <QStandardPaths>
#include <QFileDialog>
#include <QToolTip>
#include <QScreen>
#include <QWindow>

#include "qt-wrappers.hpp"
#include "obs-app.hpp"
#include "obs-frontend-api.h"
#include "window-basic-main.hpp"
#include "window-basic-settings.hpp"
#include "window-basic-auto-config.hpp"
#include "platform.hpp"
#include "obs-config-helper.hpp"
#include "url-config.hpp"
#include "ui_OBSBasicSettings.h"

#include <algorithm>
#include <cmath>

#include <obs.hpp>
#include <obs-module.h>
#include <obs-hotkey.h>
#include <util/config-file.h>
#include <util/platform.h>
#include <util/dstr.hpp>
#include <util/util.hpp>
#include <util/circlebuf.h>
#include <media-io/video-frame.h>
#include <media-io/video-io.h>
#include <media-io/audio-io.h>

using namespace std;

/* ------------------------------------------------------------------------- */

#define SIMPLE_ENCODERS_BEGIN \
	"jim_x264", "obs_qsv", "obs_x264", "ffmpeg_aac", "libfdk_aac"

#define SIMPLE_ENCODERS_AMF_H264 "amd_amf_h264"

#define SIMPLE_ENCODERS_NVENC_H264 "ffmpeg_nvenc", "nvenc_h264"

#define SIMPLE_ENCODERS_APPLE_H264 "com.apple.videotoolbox.videoencoder.h264.gva", "vt_h264_hw"

#define SIMPLE_ENCODERS_END \
	nullptr

static const char *const simple_encoders_whitelist[] = {SIMPLE_ENCODERS_BEGIN,
							SIMPLE_ENCODERS_END};

static const char *const simple_encoders_whitelist_amf[] = {
	SIMPLE_ENCODERS_BEGIN, SIMPLE_ENCODERS_AMF_H264, SIMPLE_ENCODERS_END};

static const char *const simple_encoders_whitelist_nvenc[] = {
	SIMPLE_ENCODERS_BEGIN, SIMPLE_ENCODERS_NVENC_H264, SIMPLE_ENCODERS_END};

static const char *const simple_encoders_whitelist_apple[] = {
	SIMPLE_ENCODERS_BEGIN, SIMPLE_ENCODERS_APPLE_H264, SIMPLE_ENCODERS_END};

struct SimpleOutputPreviewFormat {
	const char *format;
	const char *conversion;
};

static SimpleOutputPreviewFormat preview_formats[] = {
	{"I420", QTStr("Basic.Settings.Video.Format.I420")},
	{"NV12", QTStr("Basic.Settings.Video.Format.NV12")},
	{"I444", QTStr("Basic.Settings.Video.Format.I444")},
	{"P010", QTStr("Basic.Settings.Video.Format.P010")},
	{"RGB", QTStr("Basic.Settings.Video.Format.RGB")},
	{nullptr, nullptr}};

static const char *sdr_white_levels[] = {
	QTStr("Basic.Settings.Video.SDRWhiteLevel.300nits"),
	QTStr("Basic.Settings.Video.SDRWhiteLevel.203nits"),
	nullptr};

static const char *hdr_nominal_peak_levels[] = {
	QTStr("Basic.Settings.Video.HDRNominalPeakLevel.1000nits"),
	QTStr("Basic.Settings.Video.HDRNominalPeakLevel.600nits"),
	QTStr("Basic.Settings.Video.HDRNominalPeakLevel.500nits"),
	QTStr("Basic.Settings.Video.HDRNominalPeakLevel.400nits"),
	QTStr("Basic.Settings.Video.HDRNominalPeakLevel.300nits"),
	nullptr};

static const char *color_range_strings[] = {
	QTStr("Basic.Settings.Video.ColorRange.Partial"),
	QTStr("Basic.Settings.Video.ColorRange.Full"), nullptr};

/* ------------------------------------------------------------------------- */

enum class RescaleQuality {
	None,
	Point,
	Bilinear,
	Bicubic,
	Lanczos,
};

static const char *rescale_quality_strings[] = {
	QTStr("Basic.Settings.Output.RescaleNone"),
	QTStr("Basic.Settings.Output.RescalePoint"),
	QTStr("Basic.Settings.Output.RescaleBilinear"),
	QTStr("Basic.Settings.Output.RescaleBicubic"),
	QTStr("Basic.Settings.Output.RescaleLanczos"),
	nullptr,
};

static RescaleQuality GetRescaleQualityValue(const char *val)
{
	if (astrcmpi(val, "point") == 0)
		return RescaleQuality::Point;
	else if (astrcmpi(val, "bilinear") == 0)
		return RescaleQuality::Bilinear;
	else if (astrcmpi(val, "bicubic") == 0)
		return RescaleQuality::Bicubic;
	else if (astrcmpi(val, "lanczos") == 0)
		return RescaleQuality::Lanczos;
	else
		return RescaleQuality::None;
}

static const char *GetRescaleQualityName(RescaleQuality val)
{
	switch (val) {
	case RescaleQuality::Point:
		return "point";
	case RescaleQuality::Bilinear:
		return "bilinear";
	case RescaleQuality::Bicubic:
		return "bicubic";
	case RescaleQuality::Lanczos:
		return "lanczos";
	case RescaleQuality::None:
		return "none";
	}

	return "none";
}

/* ------------------------------------------------------------------------- */

// Copied from OBSBasic.cpp for OVI population in SaveVideoSettings
static inline enum obs_scale_type GetScaleTypeFilter(config_t *config, const char* filter_key = "ScaleFilter") // Added default arg
{
	const char *scaleTypeStr = config_get_string(config, "Video", filter_key);

	if (astrcmpi(scaleTypeStr, "bilinear") == 0)
		return OBS_SCALE_BILINEAR;
	else if (astrcmpi(scaleTypeStr, "lanczos") == 0)
		return OBS_SCALE_LANCZOS;
	else if (astrcmpi(scaleTypeStr, "area") == 0) // "area" was an option in LoadDownscaleFilters
		return OBS_SCALE_AREA;
	else if (astrcmpi(scaleTypeStr, "bicubic") == 0) // Default / common
		return OBS_SCALE_BICUBIC;
	else // Default if string is unknown or "disable" (though UI should prevent "disable")
		return OBS_SCALE_BICUBIC; // Or OBS_SCALE_DISABLE if appropriate
}

static inline enum video_format GetVideoFormatFromName(const char *name)
{
	if (!name) return VIDEO_FORMAT_NV12; // Default if null
	if (astrcmpi(name, "I420") == 0)
		return VIDEO_FORMAT_I420;
	else if (astrcmpi(name, "NV12") == 0)
		return VIDEO_FORMAT_NV12;
	else if (astrcmpi(name, "I444") == 0)
		return VIDEO_FORMAT_I444;
	// Note: The UI for color format in settings uses "P010" for data, but obs_video_info uses I010 for 10-bit 4:2:0
	// This might need careful mapping. Assuming UI string "P010" implies VIDEO_FORMAT_P010 here for now.
	else if (astrcmpi(name, "P010") == 0)
		return VIDEO_FORMAT_P010;
	else if (astrcmpi(name, "RGB") == 0)
		return VIDEO_FORMAT_RGB;
	else
		return VIDEO_FORMAT_NV12; // Default
}

static inline enum video_colorspace GetVideoColorSpaceFromName(const char *name)
{
	if (!name) return VIDEO_CS_709; // Default if null
	// UI uses "Rec. 709", "Rec. 2100 (PQ)", "Rec. 2100 (HLG)", "sRGB"
	// Config stores "709", "2100PQ", "2100HLG", "sRGB" (or similar based on SaveCombo)
	if (strcmp(name, "601") == 0) // From obs_video_info, might not be directly settable in this UI
		return VIDEO_CS_601;
	else if (strcmp(name, "709") == 0)
		return VIDEO_CS_709;
	else if (strcmp(name, "Rec. 709") == 0) // UI Text
		return VIDEO_CS_709;
	else if (strcmp(name, "2100PQ") == 0)
		return VIDEO_CS_2100_PQ;
	else if (strcmp(name, "Rec. 2100 (PQ)") == 0) // UI Text
		return VIDEO_CS_2100_PQ;
	else if (strcmp(name, "2100HLG") == 0)
		return VIDEO_CS_2100_HLG;
	else if (strcmp(name, "Rec. 2100 (HLG)") == 0) // UI Text
		return VIDEO_CS_2100_HLG;
	else if (strcmp(name, "sRGB") == 0) // sRGB from UI implies VIDEO_CS_SRGB
		return VIDEO_CS_SRGB;
	else
		return VIDEO_CS_709; // Default
}


static bool EncoderAvailable(const char *encoder)
{
	const char *temp;
	size_t i = 0;
	while (obs_enum_encoder_types(i++, &temp)) {
		if (strcmp(temp, encoder) == 0)
			return true;
	}

	return false;
}

static bool AMDAvailable()
{
	return EncoderAvailable("amd_amf_h264");
}

static bool NVENCAvailable()
{
	return EncoderAvailable("ffmpeg_nvenc") ||
	       EncoderAvailable("nvenc_h264");
}

static bool QSVAvailable()
{
	return EncoderAvailable("obs_qsv_avc");
}

static bool AppleAvailable()
{
	return EncoderAvailable("com.apple.videotoolbox.videoencoder.h264.gva") ||
	       EncoderAvailable("vt_h264_hw");
}

/* ------------------------------------------------------------------------- */

OBSBasicSettings::OBSBasicSettings(QWidget *parent)
	: QDialog(parent),
	  ui(new Ui::OBSBasicSettings),
	  main(static_cast<OBSBasic *>(parent)->GetMainWindow()),
	  output(obs_frontend_get_streaming_output()),
	  simpleOutput(config_get_bool(main->Config(), "Output", "ModeSimple")),
	  streamEncoderChanged(false),
	  recordEncoderChanged(false),
	  streamEncoderProps(nullptr),
	  recordEncoderProps(nullptr),
	  streamEncoderProps_v_stream(nullptr), // Added for vertical
	  recordEncoderProps_v_rec(nullptr),    // Added for vertical
	  ffmpegOutput(false),
	  ffmpegRecFormat(nullptr),
	  ffmpegUseRescale(false),
	  loading(false),
	  outputConfig(main->Config()),
	  streamDelayStarting(false),
	  streamDelayStopping(false),
	  streamDelayChanging(false),
	  recDelayStarting(false),
	  recDelayStopping(false),
	  outputRes(0),
	  outputRes_v(0) // Added for vertical
{
	DStr 국내산핫산_Horizontal = QTStr("Basic.Settings.Video.Horizontal");
	DStr 국내산핫산_Vertical = QTStr("Basic.Settings.Video.Vertical");

	ui->setupUi(this);

	if (!output) {
		ui->advOutEncoder->setDisabled(true);
		ui->advOutEncoder_v->setDisabled(true); // Added for vertical
	}

	HookWidget(ui->baseResolution, &OBSBasicSettings::BaseResolutionChanged);
	HookWidget(ui->outputResolution,
		   &OBSBasicSettings::OutputResolutionChanged);
	HookWidget(ui->baseResolution_v, &OBSBasicSettings::BaseResolutionChanged_V);     // Added for vertical
	HookWidget(ui->outputResolution_v, &OBSBasicSettings::OutputResolutionChanged_V); // Added for vertical

	HookWidget(ui->fpsCommon);
	HookWidget(ui->fpsInteger);
	HookWidget(ui->fpsFraction);
	HookWidget(ui->fpsCommon_v);     // Added for vertical
	HookWidget(ui->fpsInteger_v); // Added for vertical
	HookWidget(ui->fpsFraction_v); // Added for vertical

	HookWidget(ui->outputMode);

	HookWidget(ui->simpleOutVBitrate);
	HookWidget(ui->simpleOutABitrate);
	HookWidget(ui->simpleOutUseAdv);
	HookWidget(ui->simpleOutAdvEncoder);
	HookWidget(ui->simpleOutAdvPreset);
	HookWidget(ui->simpleOutAdvTune);
	HookWidget(ui->simpleOutAdvCustom);
	HookWidget(ui->simpleOutRecPath);
	HookWidget(ui->simpleOutRecQuality);
	HookWidget(ui->simpleOutRecFormat);
	HookWidget(ui->simpleOutRecEncoder);

	HookWidget(ui->simpleOutVBitrate_v); // Added for vertical
	HookWidget(ui->simpleOutABitrate_v); // Added for vertical
	HookWidget(ui->simpleOutUseAdv_v);   // Added for vertical
	HookWidget(ui->simpleOutAdvEncoder_v); // Added for vertical
	HookWidget(ui->simpleOutAdvPreset_v);  // Added for vertical
	HookWidget(ui->simpleOutAdvTune_v);    // Added for vertical
	HookWidget(ui->simpleOutAdvCustom_v);  // Added for vertical
	HookWidget(ui->simpleOutRecPath_v);    // Added for vertical
	HookWidget(ui->simpleOutRecQuality_v); // Added for vertical
	HookWidget(ui->simpleOutRecFormat_v);  // Added for vertical
	HookWidget(ui->simpleOutRecEncoder_v); // Added for vertical

	HookWidget(ui->advOutEncoder);
	HookWidget(ui->advOutUseRescale);
	HookWidget(ui->advOutRescale);
	HookWidget(ui->advOutRescaleFilter);
	HookWidget(ui->advOutEncoder_v);       // Added for vertical
	HookWidget(ui->advOutUseRescale_v);    // Added for vertical
	HookWidget(ui->advOutRescale_v);       // Added for vertical
	HookWidget(ui->advOutRescaleFilter_v); // Added for vertical

	HookWidget(ui->advOutRecType);
	HookWidget(ui->advOutRecPath);
	HookWidget(ui->advOutRecFormat);
	HookWidget(ui->advOutRecEncoder);
	HookWidget(ui->advOutRecType_v);       // Added for vertical
	HookWidget(ui->advOutRecPath_v);       // Added for vertical
	HookWidget(ui->advOutRecFormat_v);     // Added for vertical
	HookWidget(ui->advOutRecEncoder_v);    // Added for vertical
	HookWidget(ui->advOutFFFormat);
	HookWidget(ui->advOutFFFormat_v); // Added for vertical
	HookWidget(ui->advOutFFAEncoder);
	HookWidget(ui->advOutFFAEncoder_v); // Added for vertical
	HookWidget(ui->advOutFFVEncoder);
	HookWidget(ui->advOutFFVEncoder_v); // Added for vertical
	HookWidget(ui->advOutFFVBitrate);
	HookWidget(ui->advOutFFVBitrate_v); // Added for vertical
	HookWidget(ui->advOutFFUseRescale);
	HookWidget(ui->advOutFFUseRescale_v); // Added for vertical
	HookWidget(ui->advOutFFRescale);
	HookWidget(ui->advOutFFRescale_v); // Added for vertical
	HookWidget(ui->advOutFFRescaleFilter);
	HookWidget(ui->advOutFFRescaleFilter_v); // Added for vertical
	HookWidget(ui->advOutFFMuxer);
	HookWidget(ui->advOutFFMuxer_v); // Added for vertical
	HookWidget(ui->advOutFFABitrate);
	HookWidget(ui->advOutFFABitrate_v); // Added for vertical

	HookWidget(ui->advOutFFTrack1);
	HookWidget(ui->advOutFFTrack2);
	HookWidget(ui->advOutFFTrack3);
	HookWidget(ui->advOutFFTrack4);
	HookWidget(ui->advOutFFTrack5);
	HookWidget(ui->advOutFFTrack6);
	HookWidget(ui->advOutFFTrack1_v); // Added for vertical
	HookWidget(ui->advOutFFTrack2_v); // Added for vertical
	HookWidget(ui->advOutFFTrack3_v); // Added for vertical
	HookWidget(ui->advOutFFTrack4_v); // Added for vertical
	HookWidget(ui->advOutFFTrack5_v); // Added for vertical
	HookWidget(ui->advOutFFTrack6_v); // Added for vertical

	HookWidget(ui->advOutTrack1);
	HookWidget(ui->advOutTrack2);
	HookWidget(ui->advOutTrack3);
	HookWidget(ui->advOutTrack4);
	HookWidget(ui->advOutTrack5);
	HookWidget(ui->advOutTrack6);
	HookWidget(ui->advOutTrack1_v); // Added for vertical
	HookWidget(ui->advOutTrack2_v); // Added for vertical
	HookWidget(ui->advOutTrack3_v); // Added for vertical
	HookWidget(ui->advOutTrack4_v); // Added for vertical
	HookWidget(ui->advOutTrack5_v); // Added for vertical
	HookWidget(ui->advOutTrack6_v); // Added for vertical

	HookWidget(ui->advOutRecTrack1);
	HookWidget(ui->advOutRecTrack2);
	HookWidget(ui->advOutRecTrack3);
	HookWidget(ui->advOutRecTrack4);
	HookWidget(ui->advOutRecTrack5);
	HookWidget(ui->advOutRecTrack6);
	HookWidget(ui->advOutRecTrack1_v); // Added for vertical
	HookWidget(ui->advOutRecTrack2_v); // Added for vertical
	HookWidget(ui->advOutRecTrack3_v); // Added for vertical
	HookWidget(ui->advOutRecTrack4_v); // Added for vertical
	HookWidget(ui->advOutRecTrack5_v); // Added for vertical
	HookWidget(ui->advOutRecTrack6_v); // Added for vertical

	HookWidget(ui->advOutAudioBitrate);
	HookWidget(ui->advOutReplayBufferEnable);

	HookWidget(ui->previewFormat);
	HookWidget(ui->colorFormat);
	HookWidget(ui->colorSpace);
	HookWidget(ui->colorRange);
	HookWidget(ui->sdrWhiteLevel);
	HookWidget(ui->hdrNominalPeakLevel);

	HookWidget(ui->filenameFormatting);
	HookWidget(ui->overwriteIfExists);
	HookWidget(ui->replayBufferNameFormat);
	HookWidget(ui->replayBufferOverwrite);

	HookWidget(ui->enableReplayBuffer);
	HookWidget(ui->replayBufferSeconds);
	HookWidget(ui->replayBufferPath);
	HookWidget(ui->replayBufferFormat);
	HookWidget(ui->replayBufferEncoder);
	HookWidget(ui->replayBufferSuffix);

	/* stream delay */
	HookWidget(ui->streamDelayEnable);
	HookWidget(ui->streamDelaySec);
	HookWidget(ui->streamDelayPreserve);
	HookWidget(ui->streamDelayPassword);

	/* automatic reconnect */
	HookWidget(ui->autoReconnectEnable);
	HookWidget(ui->retryDelaySec);
	HookWidget(ui->maxRetries);

	HookWidget(ui->bindToIP);
	HookWidget(ui->enableNewSocketLoop);
	HookWidget(ui->enableNetworkOptimizations);
	HookWidget(ui->lowLatencyMode);

	ui->streamDelayPassword->setEchoMode(QLineEdit::Password);

	// Video Settings
	// Horizontal Tab
	ui->videoSettingsTabWidget->setTabText(0, 국내산핫산_Horizontal);
	// Vertical Tab
	ui->videoSettingsTabWidget->setTabText(1, 국내산핫산_Vertical);

	// Output Settings (Simple)
	// Horizontal Streaming Tab
	ui->simpleStreamingTabs->setTabText(0, 국내산핫산_Horizontal);
	// Vertical Streaming Tab
	ui->simpleStreamingTabs->setTabText(1, 국내산핫산_Vertical);
	// Horizontal Recording Tab
	ui->simpleRecordingTabs->setTabText(0, 국내산핫산_Horizontal);
	// Vertical Recording Tab
	ui->simpleRecordingTabs->setTabText(1, 국내산핫산_Vertical);

	// Output Settings (Advanced)
	// Horizontal Streaming Tab (already named by UI)
	// Vertical Streaming Tab (already named by UI)
	// Horizontal Recording Tab (already named by UI)
	// Vertical Recording Tab (already named by UI)

	connect(ui->buttonBox->button(QDialogButtonBox::Apply),
		&QPushButton::clicked, this, &OBSBasicSettings::Apply);

	connect(ui->outputMode, SIGNAL(currentIndexChanged(int)), this,
		SLOT(OutputModeChanged(int)));

	connect(ui->simpleOutUseAdv, SIGNAL(clicked(bool)), this,
		SLOT(UpdateSimpleAdvanced()));
	connect(ui->simpleOutAdvEncoder, SIGNAL(currentIndexChanged(int)), this,
		SLOT(UpdateSimpleAdvanced()));
	connect(ui->simpleOutRecPathBrowse, SIGNAL(clicked()), this,
		SLOT(BrowseSimpleRecPath()));
	connect(ui->simpleOutRecQuality, SIGNAL(currentIndexChanged(int)), this,
		SLOT(UpdateSimpleRecording()));
	connect(ui->simpleOutRecEncoder, SIGNAL(currentIndexChanged(int)), this,
		SLOT(UpdateSimpleRecording()));

	connect(ui->simpleOutUseAdv_v, SIGNAL(clicked(bool)), this, // Added for vertical
		SLOT(UpdateSimpleAdvanced_V()));
	connect(ui->simpleOutAdvEncoder_v, SIGNAL(currentIndexChanged(int)), this, // Added for vertical
		SLOT(UpdateSimpleAdvanced_V()));
	connect(ui->simpleOutRecPathBrowse_v, SIGNAL(clicked()), this, // Added for vertical
		SLOT(BrowseSimpleRecPath_V()));
	connect(ui->simpleOutRecQuality_v, SIGNAL(currentIndexChanged(int)), this, // Added for vertical
		SLOT(UpdateSimpleRecording_V()));
	connect(ui->simpleOutRecEncoder_v, SIGNAL(currentIndexChanged(int)), this, // Added for vertical
		SLOT(UpdateSimpleRecording_V()));

	connect(ui->advOutRecPathBrowse, SIGNAL(clicked()), this,
		SLOT(BrowseAdvRecPath()));
	connect(ui->advOutRecPathBrowse_v, SIGNAL(clicked()), this, // Added for vertical
		SLOT(BrowseAdvRecPath_V()));
	connect(ui->advOutFFRecPathBrowse, SIGNAL(clicked()), this,
		SLOT(BrowseFFRecPath()));
	connect(ui->advOutFFRecPathBrowse_v, SIGNAL(clicked()), this, // Added for vertical
		SLOT(BrowseFFRecPath_V()));
	connect(ui->advOutRecType, SIGNAL(currentIndexChanged(int)), this,
		SLOT(UpdateAdvStandardRecording()));
	connect(ui->advOutRecType_v, SIGNAL(currentIndexChanged(int)), this, // Added for vertical
		SLOT(UpdateAdvStandardRecording_V()));
	connect(ui->advOutUseRescale, SIGNAL(clicked()), this,
		SLOT(UpdateAdvOut()));
	connect(ui->advOutUseRescale_v, SIGNAL(clicked()), this, // Added for vertical
		SLOT(UpdateAdvOut_V()));
	connect(ui->advOutFFUseRescale, SIGNAL(clicked()), this,
		SLOT(UpdateAdvFFOut()));
	connect(ui->advOutFFUseRescale_v, SIGNAL(clicked()), this, // Added for vertical
		SLOT(UpdateAdvFFOut_V()));
	connect(ui->advOutEncoder, SIGNAL(currentIndexChanged(int)), this,
		SLOT(on_advOutEncoder_currentIndexChanged(int)));
	connect(ui->advOutEncoder_v, SIGNAL(currentIndexChanged(int)), this, // Added for vertical
		SLOT(on_advOutEncoder_v_stream_currentIndexChanged(int)));
	connect(ui->advOutRecEncoder, SIGNAL(currentIndexChanged(int)), this,
		SLOT(on_advOutRecEncoder_currentIndexChanged(int)));
	connect(ui->advOutRecEncoder_v, SIGNAL(currentIndexChanged(int)), this, // Added for vertical
		SLOT(on_advOutRecEncoder_v_rec_currentIndexChanged(int)));
	connect(ui->advOutFFVEncoder, SIGNAL(currentIndexChanged(int)), this,
		SLOT(on_advOutFFVEncoder_currentIndexChanged(int)));
	connect(ui->advOutFFVEncoder_v, SIGNAL(currentIndexChanged(int)), this, // Added for vertical
		SLOT(on_advOutFFVEncoder_v_currentIndexChanged(int)));
	connect(ui->advOutFFAEncoder, SIGNAL(currentIndexChanged(int)), this,
		SLOT(on_advOutFFAEncoder_currentIndexChanged(int)));
	connect(ui->advOutFFAEncoder_v, SIGNAL(currentIndexChanged(int)), this, // Added for vertical
		SLOT(on_advOutFFAEncoder_v_currentIndexChanged(int)));

	connect(ui->colorFormat, SIGNAL(currentIndexChanged(int)), this,
		SLOT(ColorFormatChanged(int)));
	connect(ui->colorSpace, SIGNAL(currentIndexChanged(int)), this,
		SLOT(ColorSpaceChanged(int)));

	connect(ui->streamDelayEnable, SIGNAL(clicked(bool)), this,
		SLOT(StreamDelayEnableChanged(bool)));
	connect(ui->streamDelaySec, SIGNAL(valueChanged(int)), this,
		SLOT(StreamDelaySecChanged(int)));
	connect(ui->streamDelayPreserve, SIGNAL(clicked(bool)), this,
		SLOT(StreamDelayPreserveChanged(bool)));

	connect(ui->autoReconnectEnable, SIGNAL(clicked(bool)), this,
		SLOT(AutoReconnectEnableChanged(bool)));
	connect(ui->retryDelaySec, SIGNAL(valueChanged(int)), this,
		SLOT(RetryDelaySecChanged(int)));
	connect(ui->maxRetries, SIGNAL(valueChanged(int)), this,
		SLOT(MaxRetriesChanged(int)));

	connect(ui->enableReplayBuffer, SIGNAL(clicked(bool)), this,
		SLOT(EnableReplayBufferChanged(bool)));
	connect(ui->replayBufferSeconds, SIGNAL(valueChanged(int)), this,
		SLOT(ReplayBufferSecondsChanged(int)));
	connect(ui->replayBufferPathBrowse, SIGNAL(clicked()), this,
		SLOT(BrowseReplayBufferPath()));

	connect(ui->fpsType, SIGNAL(currentIndexChanged(int)), this,
		SLOT(on_fpsType_currentIndexChanged(int)));
	connect(ui->fpsType_v, SIGNAL(currentIndexChanged(int)), this, // Added for vertical
		SLOT(on_fpsType_v_currentIndexChanged(int)));

	connect(ui->baseResolution, SIGNAL(editTextChanged(QString)), this,
		SLOT(on_baseResolution_editTextChanged(QString)));
	connect(ui->baseResolution_v, SIGNAL(editTextChanged(QString)), this, // Added for vertical
		SLOT(on_baseResolution_v_editTextChanged(QString)));
	connect(ui->outputResolution, SIGNAL(editTextChanged(QString)), this,
		SLOT(on_outputResolution_editTextChanged(QString)));
	connect(ui->outputResolution_v, SIGNAL(editTextChanged(QString)), this, // Added for vertical
		SLOT(on_outputResolution_v_editTextChanged(QString)));

	connect(ui->useDualOutputCheckBox, &QCheckBox::stateChanged, this, &OBSBasicSettings::on_useDualOutputCheckBox_stateChanged); // Added

	// Hooks and Connects for Vertical Advanced Stream Service Settings
	// IMPORTANT: The following UI widget names (e.g., ui->advService_v_stream) are placeholders.
	// They MUST be updated if/when these widgets are added to OBSBasicSettings.ui in the advOutputStreamTab_v.
	// Currently, these widgets DO NOT EXIST in the UI file for the vertical advanced stream tab.

	// Placeholder: Assumed QComboBox for vertical stream service type
	// if (ui->advService_v_stream) { // This widget does not exist yet
	//     HookWidget(ui->advService_v_stream);
	//     connect(ui->advService_v_stream, SIGNAL(currentIndexChanged(int)), this, SLOT(on_advOutVStreamService_currentIndexChanged()));
	// }

	// Placeholder: Assumed QLineEdit or QComboBox for vertical stream server
	// if (ui->advServer_v_stream) { // This widget does not exist yet
	//     HookWidget(ui->advServer_v_stream);
	//     // connect if it's a QComboBox, or handle textChanged if QLineEdit
	// }

	// Placeholder: Assumed QLineEdit for vertical stream key
	// if (ui->advKey_v_stream) { // This widget does not exist yet
	//     HookWidget(ui->advKey_v_stream);
	//     // connect(ui->advKey_v_stream, &QLineEdit::textChanged, this, &OBSBasicSettings::on_advOutVStreamKey_textChanged); // New slot needed
	// }

	// Placeholder: Assumed QCheckBox for "Use Authentication" for vertical stream
	// if (ui->advAuthUseStreamKey_v_stream) { // This widget does not exist yet
	//     HookWidget(ui->advAuthUseStreamKey_v_stream);
	//     connect(ui->advAuthUseStreamKey_v_stream, &QCheckBox::clicked, this, &OBSBasicSettings::on_advOutVStreamUseAuth_clicked);
	// }

	// Placeholder: Assumed QLineEdit for vertical stream username
	// if (ui->advAuthUsername_v_stream) { // This widget does not exist yet
	//     HookWidget(ui->advAuthUsername_v_stream);
	//     // connect(ui->advAuthUsername_v_stream, &QLineEdit::textChanged, this, &OBSBasicSettings::on_advOutVStreamUsername_textChanged); // New slot needed
	// }

	// Placeholder: Assumed QLineEdit for vertical stream password
	// if (ui->advAuthPassword_v_stream) { // This widget does not exist yet
	//     HookWidget(ui->advAuthPassword_v_stream);
	//     // connect(ui->advAuthPassword_v_stream, &QLineEdit::textChanged, this, &OBSBasicSettings::on_advOutVStreamPassword_textChanged); // New slot needed
	// }

	// Placeholder: Assumed QCheckBox for "Show all services" for vertical stream
	// if (ui->advShowAllServices_v_stream) { // This widget does not exist yet
	//     HookWidget(ui->advShowAllServices_v_stream);
	//     connect(ui->advShowAllServices_v_stream, &QCheckBox::clicked, this, &OBSBasicSettings::on_advOutVStreamShowAll_clicked);
	// }

	// Placeholder: Assumed QPushButton for "Connect Account" for vertical stream
	// if (ui->advConnectAccount_v_stream) { // This widget does not exist yet
	//    connect(ui->advConnectAccount_v_stream, &QPushButton::clicked, this, &OBSBasicSettings::on_advOutVStreamConnectAccount_clicked); // New slot needed
	// }

	// Placeholder: Assumed QPushButton for "Disconnect Account" for vertical stream
	// if (ui->advDisconnectAccount_v_stream) { // This widget does not exist yet
	//    connect(ui->advDisconnectAccount_v_stream, &QPushButton::clicked, this, &OBSBasicSettings::on_advOutVStreamDisconnectAccount_clicked); // New slot needed
	// }

	// Placeholder: Assumed QCheckBox for "Ignore streaming service setting recommendations" for vertical stream
	// if (ui->advIgnoreRec_v_stream) { // This widget does not exist yet
	//    connect(ui->advIgnoreRec_v_stream, &QCheckBox::clicked, this, &OBSBasicSettings::on_advOutVStreamIgnoreRec_clicked); // New slot needed
	// }


	Load();

	int idx = ui->settingPageList->currentRow();
	if (idx < 0)
		idx = 0;
	ui->settingPageList->setCurrentRow(idx);

	setFixedSize(sizeHint());
	ui->scrollArea->setWidgetResizable(true);

	QObject::connect(ui->settingPageList, &QListWidget::currentRowChanged,
			 ui->stackedWidget, &QStackedWidget::setCurrentIndex);

	AutoDetectService();

	obs_frontend_push_ui_translation(obs_module_get_string);
	obs_module_set_locale(obs_frontend_get_locale());
}

OBSBasicSettings::~OBSBasicSettings()
{
	obs_module_set_locale(nullptr);
	obs_frontend_pop_ui_translation();

	Save();

	delete streamEncoderProps;
	delete recordEncoderProps;
	delete streamEncoderProps_v_stream; // Added for vertical
	delete recordEncoderProps_v_rec;    // Added for vertical
	delete ui;
}

void OBSBasicSettings::Load()
{
	blockSaving = true;
	loading = true;

	LoadGeneralSettings();
	LoadStreamSettings();
	LoadVideoSettings();
	LoadAudioSettings();
	LoadHotkeySettings();
	LoadAdvancedSettings();

	simpleOutput = config_get_bool(main->Config(), "Output", "ModeSimple");
	if (simpleOutput) {
		ui->outputMode->setCurrentIndex(0);
	} else {
		ui->outputMode->setCurrentIndex(1);
	}

	OutputModeChanged(simpleOutput ? 0 : 1);

	UpdateStreamDelayWarning();
	UpdateRecDelayWarning();

	loading = false;
	blockSaving = false;
}

void OBSBasicSettings::Save()
{
	if (blockSaving)
		return;

	SaveGeneralSettings();
	SaveStreamSettings();
	SaveOutputSettings();
	SaveVideoSettings();
	SaveAudioSettings();
	SaveHotkeySettings();
	SaveAdvancedSettings();

	config_save_safe(main->Config(), "tmp", nullptr);
}

void OBSBasicSettings::ShowCategory(const QString &category)
{
	for (int i = 0; i < ui->settingPageList->count(); i++) {
		QListWidgetItem *item = ui->settingPageList->item(i);
		if (item->text() == category) {
			ui->settingPageList->setCurrentRow(i);
			break;
		}
	}

	setVisible(true);
	raise();
	activateWindow();
}

void OBSBasicSettings::Apply()
{
	Save();
	emit Accepted();
}

void OBSBasicSettings::accept()
{
	Apply();
	QDialog::accept();
}

void OBSBasicSettings::reject()
{
	/* prompt to save if settings were changed */
	if (SettingsChanged()) {
		QMessageBox::StandardButton button;

		button = OBSMessageBox::question(
			this, QTStr("Basic.Settings.UnappliedChanges"),
			QTStr("Basic.Settings.UnappliedChanges.Message").arg(windowTitle()),
			QMessageBox::Yes | QMessageBox::No |
				QMessageBox::Cancel);

		if (button == QMessageBox::Cancel) {
			return;
		} else if (button == QMessageBox::Yes) {
			Apply();
		}
	}

	QDialog::reject();
}

/* ------------------------------------------------------------------------- */
/* General */

void OBSBasicSettings::LoadGeneralSettings()
{
	const char *lang =
		config_get_string(main->Config(), "General", "Language");
	const char *theme =
		config_get_string(main->Config(), "General", "Theme");
	bool streamActive = obs_frontend_streaming_active();
	bool recordActive = obs_frontend_recording_active();

	ui->language->blockSignals(true);
	ui->theme->blockSignals(true);

	ui->language->clear();
	ui->theme->clear();

	vector<string> languages;
	vector<string> themes;
	main->GetLanguages(languages);
	main->GetThemes(themes);

	for (auto &iter : languages) {
		ui->language->addItem(iter.c_str());
	}
	for (auto &iter : themes) {
		ui->theme->addItem(iter.c_str());
	}

	SetComboByText(ui->language, lang);
	SetComboByText(ui->theme, theme);

	ui->language->blockSignals(false);
	ui->theme->blockSignals(false);

	ui->enableAutoUpdates->setChecked(config_get_bool(
		main->Config(), "General", "EnableAutoUpdates"));

	ui->openStats->setChecked(
		config_get_bool(main->Config(), "General", "OpenStatsOnStartup"));

	ui->showCursorOverCapture->setChecked(config_get_bool(
		main->Config(), "General", "ShowCursorOverCapture"));
	ui->projectorsAlwaysOnTop->setChecked(config_get_bool(
		main->Config(), "General", "ProjectorAlwaysOnTop"));
	ui->snappingEnabled->setChecked(config_get_bool(
		main->Config(), "General", "SnappingEnabled"));
	ui->snapDistance->setValue(
		config_get_int(main->Config(), "General", "SnapDistance"));
	ui->screenSnapping->setChecked(
		config_get_bool(main->Config(), "General", "ScreenSnapping"));
	ui->sourceSnapping->setChecked(
		config_get_bool(main->Config(), "General", "SourceSnapping"));
	ui->centerSnapping->setChecked(
		config_get_bool(main->Config(), "General", "CenterSnapping"));

	ui->saveProjectors->setChecked(
		config_get_bool(main->Config(), "General", "SaveProjectors"));

	ui->systrayEnabled->setChecked(
		config_get_bool(main->Config(), "General", "SysTrayEnabled"));
	ui->systrayMinimizeToTray->setChecked(config_get_bool(
		main->Config(), "General", "SysTrayMinimizeToTray"));
	ui->systrayAlwaysMinimize->setChecked(config_get_bool(
		main->Config(), "General", "SysTrayAlwaysMinimizeToTray"));
	ui->systrayStartMinimized->setChecked(config_get_bool(
		main->Config(), "General", "SysTrayStartMinimized"));

	ui->studioModeEnable->setChecked(
		config_get_bool(main->Config(), "General", "StudioMode"));
	ui->pauseWhenNotActive->setChecked(config_get_bool(
		main->Config(), "General", "PauseWhenNotActive"));
	ui->showTransitions->setChecked(
		config_get_bool(main->Config(), "General", "ShowTransitions"));
	ui->showQuickTransitions->setChecked(config_get_bool(
		main->Config(), "General", "ShowQuickTransitions"));
	ui->showTransitionProperties->setChecked(config_get_bool(
		main->Config(), "General", "ShowTransitionProperties"));
	ui->multiViewLayout->setCurrentIndex(config_get_int(
		main->Config(), "General", "MultiviewLayout"));
	ui->multiViewMouseSwitch->setChecked(config_get_bool(
		main->Config(), "General", "MultiviewMouseSwitch"));
	ui->multiViewDrawSourceNames->setChecked(config_get_bool(
		main->Config(), "General", "MultiviewDrawNames"));
	ui->multiViewDrawSafeAreas->setChecked(config_get_bool(
		main->Config(), "General", "MultiviewDrawSafeAreas"));

	ui->autoStartStream->setChecked(
		config_get_bool(main->Config(), "Output", "AutoStartStream"));
	ui->autoStartRecord->setChecked(
		config_get_bool(main->Config(), "Output", "AutoStartRecord"));
	ui->autoStartReplayBuffer->setChecked(config_get_bool(
		main->Config(), "Output", "AutoStartReplayBuffer"));

	ui->confirmOnStart->setChecked(config_get_bool(
		main->Config(), "Output", "ConfirmOnStart"));
	ui->confirmOnStop->setChecked(
		config_get_bool(main->Config(), "Output", "ConfirmOnStop"));
	ui->confirmOnExit->setChecked(
		config_get_bool(main->Config(), "General", "ConfirmOnExit"));

	ui->autoStartStream->setDisabled(streamActive || recordActive);
	ui->autoStartRecord->setDisabled(streamActive || recordActive);
	ui->autoStartReplayBuffer->setDisabled(streamActive || recordActive);

	GeneralConfigChanged();
}

void OBSBasicSettings::SaveGeneralSettings()
{
	if (WidgetChanged(ui->language)) {
		SaveCombo(ui->language, "General", "Language");
		main->SetLanguage(ui->language->currentText());
	}
	if (WidgetChanged(ui->theme)) {
		SaveCombo(ui->theme, "General", "Theme");
		main->SetTheme(ui->theme->currentText());
	}

	if (WidgetChanged(ui->enableAutoUpdates))
		config_set_bool(main->Config(), "General", "EnableAutoUpdates",
				ui->enableAutoUpdates->isChecked());

	if (WidgetChanged(ui->openStats))
		config_set_bool(main->Config(), "General", "OpenStatsOnStartup",
				ui->openStats->isChecked());

	if (WidgetChanged(ui->showCursorOverCapture))
		config_set_bool(main->Config(), "General",
				"ShowCursorOverCapture",
				ui->showCursorOverCapture->isChecked());
	if (WidgetChanged(ui->projectorsAlwaysOnTop))
		config_set_bool(main->Config(), "General",
				"ProjectorAlwaysOnTop",
				ui->projectorsAlwaysOnTop->isChecked());
	if (WidgetChanged(ui->snappingEnabled))
		config_set_bool(main->Config(), "General", "SnappingEnabled",
				ui->snappingEnabled->isChecked());
	if (WidgetChanged(ui->snapDistance))
		config_set_int(main->Config(), "General", "SnapDistance",
			       ui->snapDistance->value());
	if (WidgetChanged(ui->screenSnapping))
		config_set_bool(main->Config(), "General", "ScreenSnapping",
				ui->screenSnapping->isChecked());
	if (WidgetChanged(ui->sourceSnapping))
		config_set_bool(main->Config(), "General", "SourceSnapping",
				ui->sourceSnapping->isChecked());
	if (WidgetChanged(ui->centerSnapping))
		config_set_bool(main->Config(), "General", "CenterSnapping",
				ui->centerSnapping->isChecked());

	if (WidgetChanged(ui->saveProjectors))
		config_set_bool(main->Config(), "General", "SaveProjectors",
				ui->saveProjectors->isChecked());

	if (WidgetChanged(ui->systrayEnabled))
		config_set_bool(main->Config(), "General", "SysTrayEnabled",
				ui->systrayEnabled->isChecked());
	if (WidgetChanged(ui->systrayMinimizeToTray))
		config_set_bool(main->Config(), "General",
				"SysTrayMinimizeToTray",
				ui->systrayMinimizeToTray->isChecked());
	if (WidgetChanged(ui->systrayAlwaysMinimize))
		config_set_bool(main->Config(), "General",
				"SysTrayAlwaysMinimizeToTray",
				ui->systrayAlwaysMinimize->isChecked());
	if (WidgetChanged(ui->systrayStartMinimized))
		config_set_bool(main->Config(), "General",
				"SysTrayStartMinimized",
				ui->systrayStartMinimized->isChecked());

	if (WidgetChanged(ui->studioModeEnable))
		config_set_bool(main->Config(), "General", "StudioMode",
				ui->studioModeEnable->isChecked());
	if (WidgetChanged(ui->pauseWhenNotActive))
		config_set_bool(main->Config(), "General", "PauseWhenNotActive",
				ui->pauseWhenNotActive->isChecked());
	if (WidgetChanged(ui->showTransitions))
		config_set_bool(main->Config(), "General", "ShowTransitions",
				ui->showTransitions->isChecked());
	if (WidgetChanged(ui->showQuickTransitions))
		config_set_bool(main->Config(), "General",
				"ShowQuickTransitions",
				ui->showQuickTransitions->isChecked());
	if (WidgetChanged(ui->showTransitionProperties))
		config_set_bool(main->Config(), "General",
				"ShowTransitionProperties",
				ui->showTransitionProperties->isChecked());
	if (WidgetChanged(ui->multiViewLayout))
		config_set_int(main->Config(), "General", "MultiviewLayout",
			       ui->multiViewLayout->currentIndex());
	if (WidgetChanged(ui->multiViewMouseSwitch))
		config_set_bool(main->Config(), "General",
				"MultiviewMouseSwitch",
				ui->multiViewMouseSwitch->isChecked());
	if (WidgetChanged(ui->multiViewDrawSourceNames))
		config_set_bool(main->Config(), "General", "MultiviewDrawNames",
				ui->multiViewDrawSourceNames->isChecked());
	if (WidgetChanged(ui->multiViewDrawSafeAreas))
		config_set_bool(main->Config(), "General",
				"MultiviewDrawSafeAreas",
				ui->multiViewDrawSafeAreas->isChecked());

	if (WidgetChanged(ui->autoStartStream))
		config_set_bool(main->Config(), "Output", "AutoStartStream",
				ui->autoStartStream->isChecked());
	if (WidgetChanged(ui->autoStartRecord))
		config_set_bool(main->Config(), "Output", "AutoStartRecord",
				ui->autoStartRecord->isChecked());
	if (WidgetChanged(ui->autoStartReplayBuffer))
		config_set_bool(main->Config(), "Output",
				"AutoStartReplayBuffer",
				ui->autoStartReplayBuffer->isChecked());

	if (WidgetChanged(ui->confirmOnStart))
		config_set_bool(main->Config(), "Output", "ConfirmOnStart",
				ui->confirmOnStart->isChecked());
	if (WidgetChanged(ui->confirmOnStop))
		config_set_bool(main->Config(), "Output", "ConfirmOnStop",
				ui->confirmOnStop->isChecked());
	if (WidgetChanged(ui->confirmOnExit))
		config_set_bool(main->Config(), "General", "ConfirmOnExit",
				ui->confirmOnExit->isChecked());

	GeneralConfigChanged();
}

void OBSBasicSettings::GeneralConfigChanged()
{
	ui->systrayMinimizeToTray->setEnabled(ui->systrayEnabled->isChecked());
	ui->systrayAlwaysMinimize->setEnabled(ui->systrayEnabled->isChecked());
	ui->systrayStartMinimized->setEnabled(ui->systrayEnabled->isChecked());

	ui->snapDistance->setEnabled(ui->snappingEnabled->isChecked());
	ui->screenSnapping->setEnabled(ui->snappingEnabled->isChecked());
	ui->sourceSnapping->setEnabled(ui->snappingEnabled->isChecked());
	ui->centerSnapping->setEnabled(ui->snappingEnabled->isChecked());

	ui->showQuickTransitions->setEnabled(ui->showTransitions->isChecked());
	ui->showTransitionProperties->setEnabled(
		ui->showTransitions->isChecked());

	main->GeneralConfigChange();
}

/* ------------------------------------------------------------------------- */
/* Stream */

static bool ServiceCanBandwidthTest(OBSService service)
{
	obs_properties_t *props = obs_service_properties(service);
	obs_property_t *p = obs_properties_get(props, "bwtest");
	bool bwTest = obs_property_bool(p);
	obs_properties_destroy(props);
	return bwTest;
}

static bool ServiceCanShowAll(OBSService service)
{
	obs_properties_t *props = obs_service_properties(service);
	obs_property_t *p = obs_properties_get(props, "show_all");
	bool showAll = obs_property_bool(p);
	obs_properties_destroy(props);
	return showAll;
}

static bool ServiceCanUseKey(OBSService service)
{
	obs_properties_t *props = obs_service_properties(service);
	obs_property_t *p = obs_properties_get(props, "key");
	bool canUseKey = p != nullptr;
	obs_properties_destroy(props);
	return canUseKey;
}

static bool ServiceCanUseUsername(OBSService service)
{
	obs_properties_t *props = obs_service_properties(service);
	obs_property_t *p = obs_properties_get(props, "username");
	bool canUseUsername = p != nullptr;
	obs_properties_destroy(props);
	return canUseUsername;
}

static bool ServiceCanUsePassword(OBSService service)
{
	obs_properties_t *props = obs_service_properties(service);
	obs_property_t *p = obs_properties_get(props, "password");
	bool canUsePassword = p != nullptr;
	obs_properties_destroy(props);
	return canUsePassword;
}

static void SetServicePasswordEcho(OBSService service, QLineEdit *edit)
{
	obs_properties_t *props = obs_service_properties(service);
	obs_property_t *p = obs_properties_get(props, "password");
	auto type = obs_property_get_type(p);
	obs_properties_destroy(props);

	edit->setEchoMode(type == OBS_PROPERTY_TEXT_PASSWORD
				  ? QLineEdit::Password
				  : QLineEdit::Normal);
}

void OBSBasicSettings::UpdateServiceRecommendations()
{
	OBSService service = main->GetService();
	if (!service) {
		ui->ignoreStreamRec->setEnabled(false);
		return;
	}

	ui->ignoreStreamRec->setEnabled(true);
	obs_properties_t *props = obs_service_properties(service);
	obs_property_t *p =
		obs_properties_get(props, "recommendations_enabled");
	bool enabled = obs_property_bool(p);
	obs_properties_destroy(props);

	ui->ignoreStreamRec->setChecked(!enabled);
}

void OBSBasicSettings::UpdateServiceKeyWidgets()
{
	OBSService service = main->GetService();
	if (!service) {
		ui->streamKey->setEnabled(false);
		return;
	}

	SetServicePasswordEcho(service, ui->streamKey);
	ui->streamKey->setEnabled(ServiceCanUseKey(service));
}

void OBSBasicSettings::UpdateServiceAuthWidgets()
{
	OBSService service = main->GetService();
	if (!service) {
		ui->authUsername->setEnabled(false);
		ui->authPassword->setEnabled(false);
		ui->authUseStreamKey->setChecked(true);
		ui->authUseStreamKey->setEnabled(false);
		return;
	}

	SetServicePasswordEcho(service, ui->authPassword);
	ui->authUsername->setEnabled(ServiceCanUseUsername(service));
	ui->authPassword->setEnabled(ServiceCanUsePassword(service));
	ui->authUseStreamKey->setEnabled(ServiceCanUseKey(service));
}

void OBSBasicSettings::on_service_currentIndexChanged()
{
	if (loading)
		return;

	bool streamActive = obs_frontend_streaming_active();
	OBSService newService = ui->service->currentData().value<OBSService>();
	if (!newService)
		return;

	const char *type = obs_service_get_type(newService);
	if (strcmp(type, "rtmp_custom") == 0) {
		ui->showAllServices->setEnabled(true);
		ui->server->setEditable(true);
	} else {
		ui->showAllServices->setEnabled(ServiceCanShowAll(newService));
		ui->server->setEditable(false);
	}

	main->SetService(newService);
	UpdateServiceKeyWidgets();
	UpdateServiceAuthWidgets();
	UpdateServiceRecommendations();
	UpdateServerList();
	UpdateBandwidthTestEnable();

	if (streamActive) {
		ui->service->setDisabled(true);
		ui->server->setDisabled(true);
		ui->streamKey->setDisabled(true);
	}

	SetWidgetChanged(ui->service);
}

void OBSBasicSettings::on_server_currentIndexChanged()
{
	if (loading)
		return;

	main->SetServer(ui->server->currentText().toUtf8().constData());
	UpdateBandwidthTestEnable();
	SetWidgetChanged(ui->server);
}

void OBSBasicSettings::on_streamKey_textChanged()
{
	if (loading)
		return;

	main->SetKey(ui->streamKey->text().toUtf8().constData());
	SetWidgetChanged(ui->streamKey);
}

void OBSBasicSettings::on_authUsername_textChanged()
{
	if (loading)
		return;

	main->SetUsername(ui->authUsername->text().toUtf8().constData());
	SetWidgetChanged(ui->authUsername);
}

void OBSBasicSettings::on_authPassword_textChanged()
{
	if (loading)
		return;

	main->SetPassword(ui->authPassword->text().toUtf8().constData());
	SetWidgetChanged(ui->authPassword);
}

void OBSBasicSettings::on_authUseStreamKey_clicked()
{
	if (loading)
		return;

	main->SetUseAuth(ui->authUseStreamKey->isChecked() == false);
	SetWidgetChanged(ui->authUseStreamKey);
	UpdateAuthMethod();
}

void OBSBasicSettings::on_showAllServices_clicked()
{
	if (loading)
		return;

	config_set_bool(main->Config(), "Stream", "ShowAll",
			ui->showAllServices->isChecked());
	main->SaveService();
	UpdateServiceList();
	SetWidgetChanged(ui->showAllServices);
}

void OBSBasicSettings::on_ignoreStreamRec_clicked()
{
	if (loading)
		return;

	OBSService service = main->GetService();
	if (!service)
		return;

	obs_data_t *settings = obs_service_get_settings(service);
	obs_data_set_bool(settings, "recommendations_enabled",
			  !ui->ignoreStreamRec->isChecked());
	obs_service_update(service, settings);
	obs_data_release(settings);

	SetWidgetChanged(ui->ignoreStreamRec);
}

void OBSBasicSettings::on_connectAccount_clicked()
{
	OBSService service = main->GetService();
	if (!service)
		return;

	obs_properties_t *props = obs_service_properties(service);
	obs_property_t *button =
		obs_properties_get(props, "connect_account");

	if (button) {
		obs_property_button_clicked(button, service);

		/* update any properties that may have been changed by auth */
		obs_data_t *settings = obs_service_get_settings(service);
		const char *key = obs_data_get_string(settings, "key");
		const char *username = obs_data_get_string(settings, "username");
		const char *password = obs_data_get_string(settings, "password");
		bool use_auth = obs_data_get_bool(settings, "use_auth");

		ui->streamKey->setText(key);
		ui->authUsername->setText(username);
		ui->authPassword->setText(password);
		ui->authUseStreamKey->setChecked(!use_auth);

		obs_data_release(settings);

		/* update the service list in case the service changed (e.g. Twitch -> Restream) */
		UpdateServiceList();

		/* update the server list in case the server changed */
		UpdateServerList();

		/* update the enable state of the stream key widget */
		UpdateServiceKeyWidgets();

		/* update the enable state of the username/password widgets */
		UpdateServiceAuthWidgets();
		UpdateAuthMethod();

		/* update the enable state of the bandwidth test button */
		UpdateBandwidthTestEnable();
	}

	obs_properties_destroy(props);
}

void OBSBasicSettings::on_disconnectAccount_clicked()
{
	OBSService service = main->GetService();
	if (!service)
		return;

	obs_properties_t *props = obs_service_properties(service);
	obs_property_t *button =
		obs_properties_get(props, "disconnect_account");

	if (button) {
		obs_property_button_clicked(button, service);

		/* update any properties that may have been changed by auth */
		obs_data_t *settings = obs_service_get_settings(service);
		const char *key = obs_data_get_string(settings, "key");
		const char *username = obs_data_get_string(settings, "username");
		const char *password = obs_data_get_string(settings, "password");
		bool use_auth = obs_data_get_bool(settings, "use_auth");

		ui->streamKey->setText(key);
		ui->authUsername->setText(username);
		ui->authPassword->setText(password);
		ui->authUseStreamKey->setChecked(!use_auth);

		obs_data_release(settings);

		/* update the server list in case the server changed */
		UpdateServerList();

		/* update the enable state of the stream key widget */
		UpdateServiceKeyWidgets();

		/* update the enable state of the username/password widgets */
		UpdateServiceAuthWidgets();
		UpdateAuthMethod();

		/* update the enable state of the bandwidth test button */
		UpdateBandwidthTestEnable();
	}

	obs_properties_destroy(props);
}

void OBSBasicSettings::on_bandwidthTestEnable_clicked()
{
	OBSService service = main->GetService();
	if (!service)
		return;

	AutoConfig wiz(this, service);
	wiz.exec();
}

void OBSBasicSettings::on_twitchAddonChoice_currentIndexChanged(int index)
{
	if (loading)
		return;

	config_set_int(main->Config(), "Twitch", "AddonChoice", index);
	SetWidgetChanged(ui->twitchAddonChoice);
}

void OBSBasicSettings::UpdateBandwidthTestEnable()
{
	OBSService service = main->GetService();
	if (!service) {
		ui->bandwidthTestEnable->setEnabled(false);
		return;
	}

	ui->bandwidthTestEnable->setEnabled(ServiceCanBandwidthTest(service));
}

void OBSBasicSettings::UpdateServicePage()
{
	OBSService service = main->GetService();
	if (!service) {
		ui->connectAccount->setVisible(false);
		ui->disconnectAccount->setVisible(false);
		ui->twitchInfoLabel->setVisible(false);
		ui->twitchAddonChoice->setVisible(false);
		ui->twitchAddonLabel->setVisible(false);
		return;
	}

	obs_properties_t *props = obs_service_properties(service);
	obs_property_t *connect_button =
		obs_properties_get(props, "connect_account");
	obs_property_t *disconnect_button =
		obs_properties_get(props, "disconnect_account");
	obs_property_t *twitch_dashboard =
		obs_properties_get(props, "twitch_dashboard_link");

	bool has_connect = !!connect_button;
	bool has_disconnect = !!disconnect_button;
	bool has_twitch_dashboard = !!twitch_dashboard;

	ui->connectAccount->setVisible(has_connect);
	ui->disconnectAccount->setVisible(has_disconnect);
	ui->twitchInfoLabel->setVisible(has_twitch_dashboard);
	ui->twitchAddonChoice->setVisible(has_twitch_dashboard);
	ui->twitchAddonLabel->setVisible(has_twitch_dashboard);

	if (has_connect) {
		ui->connectAccount->setText(
			obs_property_description(connect_button));
	}
	if (has_disconnect) {
		ui->disconnectAccount->setText(
			obs_property_description(disconnect_button));
	}
	if (has_twitch_dashboard) {
		ui->twitchInfoLabel->setText(QString("<a href=\"%1\">%2</a>")
						     .arg(obs_property_string(
							  twitch_dashboard))
						     .arg(QTStr("Basic.Settings.Stream.Twitch.InfoLinkText")));
	}

	obs_properties_destroy(props);
}

void OBSBasicSettings::UpdateServiceList()
{
	bool showAll = ui->showAllServices->isChecked();
	const char *cur_service_id = nullptr;
	OBSService cur_service = main->GetService();
	if (cur_service)
		cur_service_id = obs_service_get_id(cur_service);

	ui->service->blockSignals(true);
	ui->service->clear();

	size_t idx = 0;
	const char *service_id;
	while (obs_enum_service_types(idx++, &service_id)) {
		OBSService service = obs_service_create(
			service_id, service_id, nullptr, nullptr);
		if (!service)
			continue;

		if (showAll || !ServiceCanShowAll(service)) {
			ui->service->addItem(obs_service_get_name(service),
					     QVariant::fromValue(service));
		} else {
			obs_service_release(service);
		}
	}

	if (cur_service_id) {
		int index = ui->service->findData(
			QVariant::fromValue(main->GetService()));
		if (index != -1) {
			ui->service->setCurrentIndex(index);
		} else {
			/* if current service isn't found, re-add it to ensure its still there */
			ui->service->addItem(obs_service_get_name(cur_service),
					     QVariant::fromValue(cur_service));
			ui->service->setCurrentIndex(ui->service->count() - 1);
		}
	}

	ui->service->blockSignals(false);

	const char *type = obs_service_get_type(main->GetService());
	if (strcmp(type, "rtmp_custom") == 0) {
		ui->showAllServices->setEnabled(true);
	} else {
		ui->showAllServices->setEnabled(
			ServiceCanShowAll(main->GetService()));
	}

	UpdateServicePage();
}

void OBSBasicSettings::UpdateServerList()
{
	OBSService service = main->GetService();
	if (!service)
		return;

	const char *current_server =
		config_get_string(main->Config(), "Stream", "Server");
	const char *type = obs_service_get_type(service);
	bool custom = strcmp(type, "rtmp_custom") == 0;

	obs_properties_t *props = obs_service_properties(service);
	obs_property_t *servers = obs_properties_get(props, "server");

	ui->server->blockSignals(true);
	ui->server->clear();

	if (custom) {
		ui->server->addItem(current_server);

	} else if (servers) {
		obs_combo_format format = obs_property_combo_format(servers);
		const char *val;

		obs_property_info_clear(obs_property_data(servers));

		size_t count = obs_property_list_item_count(servers);
		for (size_t i = 0; i < count; i++) {
			if (format == OBS_COMBO_FORMAT_STRING)
				val = obs_property_list_item_string(servers, i);
			else
				val = obs_property_list_item_name(servers, i);

			ui->server->addItem(val);
		}
	}

	SetComboByText(ui->server, current_server);
	ui->server->blockSignals(false);

	obs_properties_destroy(props);
}

void OBSBasicSettings::UpdateAuthMethod()
{
	bool useStreamKey = ui->authUseStreamKey->isChecked();
	bool useAccount = ui->connectAccount->isVisible();

	ui->authUsername->setEnabled(!useStreamKey && !useAccount);
	ui->authPassword->setEnabled(!useStreamKey && !useAccount);
	ui->streamKey->setEnabled(useStreamKey && !useAccount);
}

void OBSBasicSettings::AutoDetectService()
{
	const char *serviceType =
		config_get_string(main->Config(), "Stream", "Service");
	if (strcmp(serviceType, "Twitch") == 0) {
		QMessageBox::StandardButton button;

		QString title = QTStr(
			"Basic.Settings.Stream.TOS.Twitch.Title");
		QString text = QTStr(
			"Basic.Settings.Stream.TOS.Twitch.Text");
		QString learnMore = QTStr(
			"Basic.Settings.Stream.TOS.Twitch.LearnMore");
		QString accept = QTStr(
			"Basic.Settings.Stream.TOS.Twitch.Accept");
		QString decline = QTStr(
			"Basic.Settings.Stream.TOS.Twitch.Decline");

		text.replace("{learnMoreLink}",
			      QString("<a href=\"%1\">%2</a>")
				      .arg(TWITCH_TOS_URL)
				      .arg(learnMore));

		button = OBSMessageBox::question 버튼이없는창(this, title, text,
						      accept, decline);

		if (button == QMessageBox::DestructiveRole) {
			SetComboByText(ui->service, "YouTube / YouTube Gaming");
			main->SetService(
				ui->service->currentData().value<OBSService>());
		}
	}
}

void OBSBasicSettings::LoadStreamSettings()
{
	loading = true;

	bool streamActive = obs_frontend_streaming_active();

	const char *type =
		config_get_string(main->Config(), "Stream", "Service");
	const char *key = config_get_string(main->Config(), "Stream", "Key");
	const char *username =
		config_get_string(main->Config(), "Stream", "Username");
	const char *password =
		config_get_string(main->Config(), "Stream", "Password");
	const char *server =
		config_get_string(main->Config(), "Stream", "Server");
	bool use_auth = config_get_bool(main->Config(), "Stream", "UseAuth");
	bool show_all = config_get_bool(main->Config(), "Stream", "ShowAll");

	main->SetService(obs_service_create(type, type, nullptr, nullptr));
	main->SetServer(server);
	main->SetKey(key);
	main->SetUsername(username);
	main->SetPassword(password);
	main->SetUseAuth(use_auth);

	ui->showAllServices->setChecked(show_all);
	UpdateServiceList();
	UpdateServerList();
	UpdateServiceKeyWidgets();
	UpdateServiceAuthWidgets();
	UpdateServiceRecommendations();
	UpdateBandwidthTestEnable();

	ui->streamKey->setText(key);
	ui->authUsername->setText(username);
	ui->authPassword->setText(password);
	ui->authUseStreamKey->setChecked(!use_auth);
	UpdateAuthMethod();

	ui->twitchAddonChoice->setCurrentIndex(
		config_get_int(main->Config(), "Twitch", "AddonChoice"));

	if (streamActive) {
		ui->service->setDisabled(true);
		ui->server->setDisabled(true);
		ui->streamKey->setDisabled(true);
		ui->showAllServices->setDisabled(true);
		ui->bandwidthTestEnable->setDisabled(true);
		ui->connectAccount->setDisabled(true);
		ui->disconnectAccount->setDisabled(true);
		ui->authUsername->setDisabled(true);
		ui->authPassword->setDisabled(true);
		ui->authUseStreamKey->setDisabled(true);
	}

	loading = false;
}

void OBSBasicSettings::SaveStreamSettings()
{
	if (WidgetChanged(ui->service) || WidgetChanged(ui->server) ||
	    WidgetChanged(ui->streamKey) || WidgetChanged(ui->authUsername) ||
	    WidgetChanged(ui->authPassword) ||
	    WidgetChanged(ui->authUseStreamKey) ||
	    WidgetChanged(ui->showAllServices) ||
	    WidgetChanged(ui->ignoreStreamRec)) {
		main->SaveService();
	}

	if (WidgetChanged(ui->twitchAddonChoice))
		config_set_int(main->Config(), "Twitch", "AddonChoice",
				 ui->twitchAddonChoice->currentIndex());
}

/* ------------------------------------------------------------------------- */
/* Output */

void OBSBasicSettings::OutputModeChanged(int i)
{
	bool simple = (i == 0);
	if (!loading && simple == simpleOutput)
		return;

	simpleOutput = simple;

	ui->stackedOutput->setCurrentIndex(simple ? 0 : 1);

	if (!loading) {
		config_set_bool(main->Config(), "Output", "ModeSimple",
				simpleOutput);
		SetWidgetChanged(ui->outputMode);
	}

	blockSaving = true;
	LoadOutputSettings();
	blockSaving = false;
}

/* ------------------------------------------------------------------------- */
/* Output (Simple) */

const char *OBSBasicSettings::GetEncoderForFormat(const char *format)
{
	if (strcmp(format, "flv") == 0) {
		return EncoderAvailable("obs_x264") ? "obs_x264"
						      : "jim_x264";
	} else if (strcmp(format, "mp4") == 0 || strcmp(format, "mov") == 0 ||
		   strcmp(format, "mkv") == 0 || strcmp(format, "ts") == 0) {
		if (NVENCAvailable()) {
			return EncoderAvailable("nvenc_h264") ? "nvenc_h264"
							      : "ffmpeg_nvenc";
		} else if (AMDAvailable()) {
			return "amd_amf_h264";
		} else if (QSVAvailable()) {
			return "obs_qsv_avc";
		} else if (AppleAvailable()) {
			return EncoderAvailable("vt_h264_hw")
				       ? "vt_h264_hw"
				       : "com.apple.videotoolbox.videoencoder.h264.gva";
		} else {
			return EncoderAvailable("obs_x264") ? "obs_x264"
							      : "jim_x264";
		}
	}

	return EncoderAvailable("obs_x264") ? "obs_x264" : "jim_x264";
}

void OBSBasicSettings::UpdateSimpleRecording()
{
	if (loading)
		return;

	bool useCustom = ui->simpleOutRecQuality->currentIndex() == 4;
	const char *curFormat = ui->simpleOutRecFormat->currentText()
					  .toLower()
					  .toUtf8()
					  .constData();

	ui->simpleOutRecEncoder->setEnabled(useCustom);
	if (useCustom) {
		const char *encoder = GetEncoderForFormat(curFormat);
		SetComboByText(ui->simpleOutRecEncoder,
			       obs_encoder_get_display_name(encoder));
	}
}

void OBSBasicSettings::UpdateSimpleRecording_V() // Added for vertical
{
	if (loading)
		return;

	bool useCustom = ui->simpleOutRecQuality_v->currentIndex() == 4;
	const char *curFormat = ui->simpleOutRecFormat_v->currentText()
					  .toLower()
					  .toUtf8()
					  .constData();

	ui->simpleOutRecEncoder_v->setEnabled(useCustom);
	if (useCustom) {
		const char *encoder = GetEncoderForFormat(curFormat);
		SetComboByText(ui->simpleOutRecEncoder_v,
			       obs_encoder_get_display_name(encoder));
	}
}

void OBSBasicSettings::UpdateSimpleAdvanced()
{
	if (loading)
		return;

	bool useAdv = ui->simpleOutUseAdv->isChecked();
	const char *encoder = ui->simpleOutAdvEncoder->currentData()
				      .toString()
				      .toUtf8()
				      .constData();

	bool streaming = obs_frontend_streaming_active();
	bool useCPU = strcmp(encoder, "obs_x264") == 0 ||
		      strcmp(encoder, "jim_x264") == 0;

	ui->simpleOutAdvEncoder->setEnabled(useAdv && !streaming);
	ui->simpleOutAdvPreset->setEnabled(useAdv && useCPU && !streaming);
	ui->simpleOutAdvTune->setEnabled(useAdv && useCPU && !streaming);
	ui->simpleOutAdvCustom->setEnabled(useAdv && !streaming);

	if (useAdv && !streaming) {
		if (!useCPU) {
			SetComboByText(ui->simpleOutAdvPreset, "hq");
			ui->simpleOutAdvTune->setCurrentIndex(0);
		}
	}
}

void OBSBasicSettings::UpdateSimpleAdvanced_V() // Added for vertical
{
	if (loading)
		return;

	bool useAdv = ui->simpleOutUseAdv_v->isChecked();
	const char *encoder = ui->simpleOutAdvEncoder_v->currentData()
				      .toString()
				      .toUtf8()
				      .constData();

	bool streaming = obs_frontend_streaming_active(); // Should this be tied to a vertical stream active flag?
	bool useCPU = strcmp(encoder, "obs_x264") == 0 ||
		      strcmp(encoder, "jim_x264") == 0;

	ui->simpleOutAdvEncoder_v->setEnabled(useAdv && !streaming);
	ui->simpleOutAdvPreset_v->setEnabled(useAdv && useCPU && !streaming);
	ui->simpleOutAdvTune_v->setEnabled(useAdv && useCPU && !streaming);
	ui->simpleOutAdvCustom_v->setEnabled(useAdv && !streaming);

	if (useAdv && !streaming) {
		if (!useCPU) {
			SetComboByText(ui->simpleOutAdvPreset_v, "hq");
			ui->simpleOutAdvTune_v->setCurrentIndex(0);
		}
	}
}

void OBSBasicSettings::BrowseSimpleRecPath()
{
	QString path = QFileDialog::getExistingDirectory(
		this, QTStr("Basic.Settings.Output.Simple.Recording.Path"),
		ui->simpleOutRecPath->text());

	if (!path.isEmpty()) {
		ui->simpleOutRecPath->setText(path);
		SetWidgetChanged(ui->simpleOutRecPath);
	}
}

void OBSBasicSettings::BrowseSimpleRecPath_V() // Added for vertical
{
	QString path = QFileDialog::getExistingDirectory(
		this, QTStr("Basic.Settings.Output.Simple.Recording.Path"),
		ui->simpleOutRecPath_v->text());

	if (!path.isEmpty()) {
		ui->simpleOutRecPath_v->setText(path);
		SetWidgetChanged(ui->simpleOutRecPath_v);
	}
}

static void InitStreamEncoders(QComboBox *box, bool hwEncoders)
{
	const char *const *encoders;

	if (hwEncoders) {
		if (AppleAvailable())
			encoders = simple_encoders_whitelist_apple;
		else if (NVENCAvailable())
			encoders = simple_encoders_whitelist_nvenc;
		else if (AMDAvailable())
			encoders = simple_encoders_whitelist_amf;
		else
			encoders = simple_encoders_whitelist;
	} else {
		encoders = simple_encoders_whitelist;
	}

	box->clear();

	for (const char *const *walk = encoders; *walk; walk++) {
		const char *enc_name = *walk;
		if (!EncoderAvailable(enc_name))
			continue;

		const char *name = obs_encoder_get_display_name(enc_name);
		box->addItem(name, enc_name);
	}
}

static void InitRecEncoders(QComboBox *box)
{
	box->clear();

	const char *encoders[] = {"obs_qsv_avc",
				  "amd_amf_h264",
				  "ffmpeg_nvenc",
				  "nvenc_h264",
				  "com.apple.videotoolbox.videoencoder.h264.gva",
				  "vt_h264_hw",
				  "obs_x264",
				  "jim_x264",
				  nullptr};

	for (const char **walk = encoders; *walk; walk++) {
		const char *enc_name = *walk;
		if (!EncoderAvailable(enc_name))
			continue;

		const char *name = obs_encoder_get_display_name(enc_name);
		box->addItem(name, enc_name);
	}
}

static void SetEncoderFromSettings(QComboBox *box, const char *name)
{
	int idx = box->findData(name);
	if (idx == -1) {
		const char *disp = obs_encoder_get_display_name(name);
		box->insertItem(0, disp, name);
		idx = 0;
	}

	box->setCurrentIndex(idx);
}

QString OBSBasicSettings::SimpleOutGetSelectedAudioTracks()
{
	QString tracks;
	if (ui->simpleOutTrack1->isChecked())
		tracks += "1";
	if (ui->simpleOutTrack2->isChecked())
		tracks += (tracks.isEmpty() ? "" : ",") + QString("2");
	if (ui->simpleOutTrack3->isChecked())
		tracks += (tracks.isEmpty() ? "" : ",") + QString("3");
	if (ui->simpleOutTrack4->isChecked())
		tracks += (tracks.isEmpty() ? "" : ",") + QString("4");
	if (ui->simpleOutTrack5->isChecked())
		tracks += (tracks.isEmpty() ? "" : ",") + QString("5");
	if (ui->simpleOutTrack6->isChecked())
		tracks += (tracks.isEmpty() ? "" : ",") + QString("6");
	return tracks;
}

QString OBSBasicSettings::SimpleOutGetSelectedAudioTracks_V() // Added for vertical
{
	QString tracks;
	if (ui->simpleOutTrack1_v->isChecked())
		tracks += "1";
	if (ui->simpleOutTrack2_v->isChecked())
		tracks += (tracks.isEmpty() ? "" : ",") + QString("2");
	if (ui->simpleOutTrack3_v->isChecked())
		tracks += (tracks.isEmpty() ? "" : ",") + QString("3");
	if (ui->simpleOutTrack4_v->isChecked())
		tracks += (tracks.isEmpty() ? "" : ",") + QString("4");
	if (ui->simpleOutTrack5_v->isChecked())
		tracks += (tracks.isEmpty() ? "" : ",") + QString("5");
	if (ui->simpleOutTrack6_v->isChecked())
		tracks += (tracks.isEmpty() ? "" : ",") + QString("6");
	return tracks;
}

void OBSBasicSettings::LoadSimpleOutputSettings()
{
	bool streamActive = obs_frontend_streaming_active();
	bool recordActive = obs_frontend_recording_active();
	bool replayActive = obs_frontend_replay_buffer_active();
	bool hwEncoders =
		config_get_bool(main->Config(), "Output", "EnforceStreamService");

	// Horizontal Streaming
	ui->simpleOutVBitrate->setValue(
		config_get_int(main->Config(), "Output", "VBitrate"));
	ui->simpleOutABitrate->setValue(
		config_get_int(main->Config(), "Output", "ABitrate"));
	ui->simpleOutUseAdv->setChecked(
		config_get_bool(main->Config(), "Output", "UseAdvanced"));
	ui->simpleOutAdvPreset->clear();
	ui->simpleOutAdvTune->clear();
	ui->simpleOutAdvPreset->addItem("ultrafast");
	ui->simpleOutAdvPreset->addItem("superfast");
	ui->simpleOutAdvPreset->addItem("veryfast");
	ui->simpleOutAdvPreset->addItem("faster");
	ui->simpleOutAdvPreset->addItem("fast");
	ui->simpleOutAdvPreset->addItem("medium");
	ui->simpleOutAdvPreset->addItem("slow");
	ui->simpleOutAdvPreset->addItem("slower");
	ui->simpleOutAdvPreset->addItem("veryslow");
	ui->simpleOutAdvPreset->addItem("placebo");
	ui->simpleOutAdvTune->addItem(QTStr("None"));
	ui->simpleOutAdvTune->addItem("film");
	ui->simpleOutAdvTune->addItem("animation");
	ui->simpleOutAdvTune->addItem("grain");
	ui->simpleOutAdvTune->addItem("stillimage");
	ui->simpleOutAdvTune->addItem("psnr");
	ui->simpleOutAdvTune->addItem("ssim");
	ui->simpleOutAdvTune->addItem("fastdecode");
	ui->simpleOutAdvTune->addItem("zerolatency");

	InitStreamEncoders(ui->simpleOutAdvEncoder, hwEncoders);
	SetEncoderFromSettings(ui->simpleOutAdvEncoder,
			       config_get_string(main->Config(), "Output",
						 "StreamEncoder"));
	SetComboByText(ui->simpleOutAdvPreset,
		       config_get_string(main->Config(), "Output", "Preset"));
	SetComboByText(ui->simpleOutAdvTune,
		       config_get_string(main->Config(), "Output", "Tune"));
	ui->simpleOutAdvCustom->setText(config_get_string(
		main->Config(), "Output", "StreamCustom"));

	// Horizontal Recording
	QString 온도니_path = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
	ui->simpleOutRecPath->setText(config_get_string(
		main->Config(), "Output", "RecPath", 온도니_path.toUtf8().constData()));
	ui->simpleOutRecQuality->setCurrentIndex(
		config_get_int(main->Config(), "Output", "RecQuality"));
	SetComboByText(ui->simpleOutRecFormat,
		       config_get_string(main->Config(), "Output", "RecFormat"));
	InitRecEncoders(ui->simpleOutRecEncoder);
	SetEncoderFromSettings(ui->simpleOutRecEncoder,
			       config_get_string(main->Config(), "Output",
						 "RecEncoder"));

	QString tracks_h = config_get_string(main->Config(), "Output", "RecTracks");
	ui->simpleOutTrack1->setChecked(tracks_h.contains("1"));
	ui->simpleOutTrack2->setChecked(tracks_h.contains("2"));
	ui->simpleOutTrack3->setChecked(tracks_h.contains("3"));
	ui->simpleOutTrack4->setChecked(tracks_h.contains("4"));
	ui->simpleOutTrack5->setChecked(tracks_h.contains("5"));
	ui->simpleOutTrack6->setChecked(tracks_h.contains("6"));

	// Vertical Streaming
	ui->simpleOutVBitrate_v->setValue(
		config_get_int(main->Config(), "Output", "VBitrate_V_Stream"));
	ui->simpleOutABitrate_v->setValue(
		config_get_int(main->Config(), "Output", "ABitrate_V_Stream")); // Audio is global, but keep UI separate for now
	ui->simpleOutUseAdv_v->setChecked(
		config_get_bool(main->Config(), "Output", "UseAdvanced_V_Stream"));
	ui->simpleOutAdvPreset_v->clear();
	ui->simpleOutAdvTune_v->clear();
	ui->simpleOutAdvPreset_v->addItems(ui->simpleOutAdvPreset->model()->stringList()); // Copy from horizontal
	ui->simpleOutAdvTune_v->addItems(ui->simpleOutAdvTune->model()->stringList());     // Copy from horizontal

	InitStreamEncoders(ui->simpleOutAdvEncoder_v, hwEncoders); // Assuming same encoder availability
	SetEncoderFromSettings(ui->simpleOutAdvEncoder_v,
			       config_get_string(main->Config(), "Output", "StreamEncoder_V_Stream"));
	SetComboByText(ui->simpleOutAdvPreset_v,
		       config_get_string(main->Config(), "Output", "Preset_V_Stream"));
	SetComboByText(ui->simpleOutAdvTune_v,
		       config_get_string(main->Config(), "Output", "Tune_V_Stream"));
	ui->simpleOutAdvCustom_v->setText(config_get_string(
		main->Config(), "Output", "StreamCustom_V_Stream"));

	// Vertical Recording
	ui->simpleOutRecPath_v->setText(config_get_string(
		main->Config(), "Output", "RecPath_V_Rec", 온도니_path.toUtf8().constData()));
	ui->simpleOutRecQuality_v->setCurrentIndex(
		config_get_int(main->Config(), "Output", "RecQuality_V_Rec"));
	SetComboByText(ui->simpleOutRecFormat_v,
		       config_get_string(main->Config(), "Output", "RecFormat_V_Rec"));
	InitRecEncoders(ui->simpleOutRecEncoder_v);
	SetEncoderFromSettings(ui->simpleOutRecEncoder_v,
			       config_get_string(main->Config(), "Output", "RecEncoder_V_Rec"));

	QString tracks_v = config_get_string(main->Config(), "Output", "RecTracks_V_Rec");
	ui->simpleOutTrack1_v->setChecked(tracks_v.contains("1"));
	ui->simpleOutTrack2_v->setChecked(tracks_v.contains("2"));
	ui->simpleOutTrack3_v->setChecked(tracks_v.contains("3"));
	ui->simpleOutTrack4_v->setChecked(tracks_v.contains("4"));
	ui->simpleOutTrack5_v->setChecked(tracks_v.contains("5"));
	ui->simpleOutTrack6_v->setChecked(tracks_v.contains("6"));

	UpdateSimpleAdvanced();
	UpdateSimpleRecording();
	UpdateSimpleAdvanced_V();   // Added for vertical
	UpdateSimpleRecording_V(); // Added for vertical

	if (streamActive) {
		ui->simpleOutVBitrate->setDisabled(true);
		ui->simpleOutUseAdv->setDisabled(true);
		ui->simpleOutAdvEncoder->setDisabled(true);
		ui->simpleOutAdvPreset->setDisabled(true);
		ui->simpleOutAdvTune->setDisabled(true);
		ui->simpleOutAdvCustom->setDisabled(true);

		ui->simpleOutVBitrate_v->setDisabled(true); // Added for vertical
		ui->simpleOutUseAdv_v->setDisabled(true);   // Added for vertical
		ui->simpleOutAdvEncoder_v->setDisabled(true); // Added for vertical
		ui->simpleOutAdvPreset_v->setDisabled(true);  // Added for vertical
		ui->simpleOutAdvTune_v->setDisabled(true);    // Added for vertical
		ui->simpleOutAdvCustom_v->setDisabled(true);  // Added for vertical
	}
	if (recordActive || replayActive) {
		ui->simpleOutRecPath->setDisabled(true);
		ui->simpleOutRecPathBrowse->setDisabled(true);
		ui->simpleOutRecQuality->setDisabled(true);
		ui->simpleOutRecFormat->setDisabled(true);
		ui->simpleOutRecEncoder->setDisabled(true);

		ui->simpleOutRecPath_v->setDisabled(true);    // Added for vertical
		ui->simpleOutRecPathBrowse_v->setDisabled(true); // Added for vertical
		ui->simpleOutRecQuality_v->setDisabled(true); // Added for vertical
		ui->simpleOutRecFormat_v->setDisabled(true);  // Added for vertical
		ui->simpleOutRecEncoder_v->setDisabled(true); // Added for vertical
	}

	ui->simpleOutABitrate->setDisabled(true); // Audio bitrate is global
	ui->simpleOutABitrate_v->setDisabled(true); // Audio bitrate is global

	// Ensure vertical simple output tabs are enabled/disabled based on dual output mode
	bool dualOutputEnabled = ui->useDualOutputCheckBox->isChecked();
	ui->simpleStreamingTabs->setTabEnabled(1, dualOutputEnabled); // Vertical Streaming Tab
	ui->simpleRecordingTabs->setTabEnabled(1, dualOutputEnabled); // Vertical Recording Tab
}

/* ------------------------------------------------------------------------- */
/* Output (Advanced) */

static void InitAdvEncoders(QComboBox *box, const char *default_encoder)
{
	box->clear();

	size_t i = 0;
	const char *id;
	while (obs_enum_encoder_types(i++, &id)) {
		uint32_t caps = obs_get_encoder_caps(id);
		if ((caps & OBS_ENCODER_CAP_STREAMING) == 0)
			continue;
		if ((caps & ENCODER_HIDE_FLAGS) != 0)
			continue;

		QString name = obs_encoder_get_display_name(id);
		if (caps & OBS_ENCODER_CAP_DEPRECATED)
			name += " (" + QTStr("Deprecated") + ")";

		box->addItem(name, id);
	}

	if (box->count() == 0) {
		box->addItem(obs_encoder_get_display_name(default_encoder),
			     default_encoder);
	}
}

static void InitAdvRecEncoders(QComboBox *box, const char *default_encoder)
{
	box->clear();

	size_t i = 0;
	const char *id;
	while (obs_enum_encoder_types(i++, &id)) {
		uint32_t caps = obs_get_encoder_caps(id);
		if ((caps & OBS_ENCODER_CAP_RECORDING) == 0)
			continue;
		if ((caps & ENCODER_HIDE_FLAGS) != 0)
			continue;

		QString name = obs_encoder_get_display_name(id);
		if (caps & OBS_ENCODER_CAP_DEPRECATED)
			name += " (" + QTStr("Deprecated") + ")";

		box->addItem(name, id);
	}

	if (box->count() == 0) {
		box->addItem(obs_encoder_get_display_name(default_encoder),
			     default_encoder);
	}
}

static void InitAdvFFVEncoders(QComboBox *box, const char *default_encoder)
{
	box->clear();

	size_t i = 0;
	const char *id;
	while (obs_enum_encoder_types(i++, &id)) {
		uint32_t caps = obs_get_encoder_caps(id);
		if ((caps & OBS_ENCODER_CAP_RECORDING) == 0)
			continue;
		if ((caps & ENCODER_HIDE_FLAGS) != 0)
			continue;
		if (strcmp(obs_encoder_get_type(id), "ffmpeg_mux") != 0)
			continue;

		QString name = obs_encoder_get_display_name(id);
		if (caps & OBS_ENCODER_CAP_DEPRECATED)
			name += " (" + QTStr("Deprecated") + ")";

		box->addItem(name, id);
	}

	if (box->count() == 0) {
		box->addItem(obs_encoder_get_display_name(default_encoder),
			     default_encoder);
	}
}

static void InitAdvFFAEncoders(QComboBox *box, const char *default_encoder)
{
	box->clear();

	size_t i = 0;
	const char *id;
	while (obs_enum_encoder_types(i++, &id)) {
		uint32_t caps = obs_get_encoder_caps(id);
		if ((caps & OBS_ENCODER_CAP_RECORDING) == 0)
			continue;
		if ((caps & ENCODER_HIDE_FLAGS) != 0)
			continue;
		if (strcmp(obs_encoder_get_type(id), "ffmpeg_aac") != 0 &&
		    strcmp(obs_encoder_get_type(id), "ffmpeg_opus") != 0 &&
		    strcmp(obs_encoder_get_type(id), "libfdk_aac") != 0)
			continue;

		QString name = obs_encoder_get_display_name(id);
		if (caps & OBS_ENCODER_CAP_DEPRECATED)
			name += " (" + QTStr("Deprecated") + ")";

		box->addItem(name, id);
	}

	if (box->count() == 0) {
		box->addItem(obs_encoder_get_display_name(default_encoder),
			     default_encoder);
	}
}

void OBSBasicSettings::UpdateAdvOut()
{
	if (loading)
		return;

	ui->advOutRescale->setEnabled(ui->advOutUseRescale->isChecked());
	ui->advOutRescaleFilter->setEnabled(ui->advOutUseRescale->isChecked());
	SetWidgetChanged(ui->advOutUseRescale);
}

void OBSBasicSettings::UpdateAdvOut_V() // Added for vertical
{
	if (loading)
		return;

	ui->advOutRescale_v->setEnabled(ui->advOutUseRescale_v->isChecked());
	ui->advOutRescaleFilter_v->setEnabled(ui->advOutUseRescale_v->isChecked());
	SetWidgetChanged(ui->advOutUseRescale_v);
}

void OBSBasicSettings::UpdateAdvFFOut()
{
	if (loading)
		return;

	ui->advOutFFRescale->setEnabled(ui->advOutFFUseRescale->isChecked());
	ui->advOutFFRescaleFilter->setEnabled(
		ui->advOutFFUseRescale->isChecked());
	SetWidgetChanged(ui->advOutFFUseRescale);
}

void OBSBasicSettings::UpdateAdvFFOut_V() // Added for vertical
{
	if (loading)
		return;

	ui->advOutFFRescale_v->setEnabled(ui->advOutFFUseRescale_v->isChecked());
	ui->advOutFFRescaleFilter_v->setEnabled(
		ui->advOutFFUseRescale_v->isChecked());
	SetWidgetChanged(ui->advOutFFUseRescale_v);
}

void OBSBasicSettings::UpdateAdvStandardRecording()
{
	if (loading)
		return;

	bool useCustom = ui->advOutRecType->currentIndex() == 1;
	const char *curFormat = ui->advOutRecFormat->currentText()
					  .toLower()
					  .toUtf8()
					  .constData();

	ui->advOutRecEncoder->setEnabled(useCustom);
	if (useCustom) {
		const char *encoder = GetEncoderForFormat(curFormat);
		SetComboByText(ui->advOutRecEncoder,
			       obs_encoder_get_display_name(encoder));
	}

	SetWidgetChanged(ui->advOutRecType);
}

void OBSBasicSettings::UpdateAdvStandardRecording_V() // Added for vertical
{
	if (loading)
		return;

	bool useCustom = ui->advOutRecType_v->currentIndex() == 1;
	const char *curFormat = ui->advOutRecFormat_v->currentText()
					  .toLower()
					  .toUtf8()
					  .constData();

	ui->advOutRecEncoder_v->setEnabled(useCustom);
	if (useCustom) {
		const char *encoder = GetEncoderForFormat(curFormat);
		SetComboByText(ui->advOutRecEncoder_v,
			       obs_encoder_get_display_name(encoder));
	}

	SetWidgetChanged(ui->advOutRecType_v);
}

void OBSBasicSettings::BrowseAdvRecPath()
{
	QString path = QFileDialog::getExistingDirectory(
		this, QTStr("Basic.Settings.Output.Adv.Recording.Path"),
		ui->advOutRecPath->text());

	if (!path.isEmpty()) {
		ui->advOutRecPath->setText(path);
		SetWidgetChanged(ui->advOutRecPath);
	}
}

void OBSBasicSettings::BrowseAdvRecPath_V() // Added for vertical
{
	QString path = QFileDialog::getExistingDirectory(
		this, QTStr("Basic.Settings.Output.Adv.Recording.Path"),
		ui->advOutRecPath_v->text());

	if (!path.isEmpty()) {
		ui->advOutRecPath_v->setText(path);
		SetWidgetChanged(ui->advOutRecPath_v);
	}
}

void OBSBasicSettings::BrowseFFRecPath()
{
	QString path = QFileDialog::getExistingDirectory(
		this, QTStr("Basic.Settings.Output.Adv.Recording.Path"),
		ui->advOutFFRecPath->text());

	if (!path.isEmpty()) {
		ui->advOutFFRecPath->setText(path);
		SetWidgetChanged(ui->advOutFFRecPath);
	}
}

void OBSBasicSettings::BrowseFFRecPath_V() // Added for vertical
{
	QString path = QFileDialog::getExistingDirectory(
		this, QTStr("Basic.Settings.Output.Adv.Recording.Path"),
		ui->advOutFFRecPath_v->text());

	if (!path.isEmpty()) {
		ui->advOutFFRecPath_v->setText(path);
		SetWidgetChanged(ui->advOutFFRecPath_v);
	}
}

void OBSBasicSettings::ResetEncoders()
{
	delete streamEncoderProps;
	delete recordEncoderProps;
	delete streamEncoderProps_v_stream; // Added for vertical
	delete recordEncoderProps_v_rec;    // Added for vertical

	streamEncoderProps = nullptr;
	recordEncoderProps = nullptr;
	streamEncoderProps_v_stream = nullptr; // Added for vertical
	recordEncoderProps_v_rec = nullptr;    // Added for vertical

	// Horizontal Streaming
	const char *streamEncoder =
		config_get_string(main->Config(), "AdvOut", "Encoder");
	curAdvStreamEncoder = streamEncoder;
	streamEncoderProps = CreateEncoderPropertyView(streamEncoder,
						       "streamEncoder.json");
	if (streamEncoderProps) {
		streamEncoderProps->setSizePolicy(QSizePolicy::Preferred,
						  QSizePolicy::Minimum);
		ui->advOutEncoderLayout->addWidget(streamEncoderProps);
		connect(streamEncoderProps, &OBSPropertiesView::Changed, this,
			&OBSBasicSettings::UpdateStreamDelayEstimate);
	}

	// Horizontal Recording (Standard)
	const char *recEncoder =
		config_get_string(main->Config(), "AdvOut", "RecEncoder");
	curAdvRecEncoder = recEncoder;
	recordEncoderProps =
		CreateEncoderPropertyView(recEncoder, "recordEncoder.json");
	if (recordEncoderProps) {
		recordEncoderProps->setSizePolicy(QSizePolicy::Preferred,
						  QSizePolicy::Minimum);
		ui->advOutRecEncoderLayout->addWidget(recordEncoderProps);
	}

	// Vertical Streaming
	const char *streamEncoder_v =
		config_get_string(main->Config(), "AdvOut", "Encoder_V_Stream");
	curAdvStreamEncoder_v_stream = streamEncoder_v;
	streamEncoderProps_v_stream = CreateEncoderPropertyView(
		streamEncoder_v, "streamEncoder_v_stream.json");
	if (streamEncoderProps_v_stream) {
		streamEncoderProps_v_stream->setSizePolicy(
			QSizePolicy::Preferred, QSizePolicy::Minimum);
		ui->advOutEncoderLayout_v->addWidget(streamEncoderProps_v_stream);
		// connect(streamEncoderProps_v_stream, &OBSPropertiesView::Changed, this, &OBSBasicSettings::UpdateVerticalStreamDelayEstimate); // TODO if needed
	}

	// Vertical Recording (Standard)
	const char *recEncoder_v =
		config_get_string(main->Config(), "AdvOut", "RecEncoder_V_Rec");
	curAdvRecEncoder_v_rec = recEncoder_v;
	recordEncoderProps_v_rec = CreateEncoderPropertyView(
		recEncoder_v, "recordEncoder_v_rec.json");
	if (recordEncoderProps_v_rec) {
		recordEncoderProps_v_rec->setSizePolicy(QSizePolicy::Preferred,
							QSizePolicy::Minimum);
		ui->advOutRecEncoderLayout_v->addWidget(recordEncoderProps_v_rec);
	}

	UpdateStreamDelayEstimate();
	// UpdateVerticalStreamDelayEstimate(); // TODO if needed
}

void OBSBasicSettings::on_advOutEncoder_currentIndexChanged(int)
{
	if (loading)
		return;

	const char *type = GetCurrentEncoder(ui->advOutEncoder);
	if (strcmp(curAdvStreamEncoder.c_str(), type) == 0)
		return;

	curAdvStreamEncoder = type;

	delete streamEncoderProps;
	streamEncoderProps =
		CreateEncoderPropertyView(type, "streamEncoder.json", true);
	if (streamEncoderProps) {
		streamEncoderProps->setSizePolicy(QSizePolicy::Preferred,
						  QSizePolicy::Minimum);
		ui->advOutEncoderLayout->addWidget(streamEncoderProps);
		connect(streamEncoderProps, &OBSPropertiesView::Changed, this,
			&OBSBasicSettings::UpdateStreamDelayEstimate);
	}

	streamEncoderChanged = true;
	SetWidgetChanged(ui->advOutEncoder);
	UpdateStreamDelayEstimate();
}

void OBSBasicSettings::on_advOutEncoder_v_stream_currentIndexChanged(int) // Added for vertical
{
	if (loading)
		return;

	const char *type = GetCurrentEncoder(ui->advOutEncoder_v);
	if (strcmp(curAdvStreamEncoder_v_stream.c_str(), type) == 0)
		return;

	curAdvStreamEncoder_v_stream = type;

	delete streamEncoderProps_v_stream;
	streamEncoderProps_v_stream = CreateEncoderPropertyView(
		type, "streamEncoder_v_stream.json", true);
	if (streamEncoderProps_v_stream) {
		streamEncoderProps_v_stream->setSizePolicy(
			QSizePolicy::Preferred, QSizePolicy::Minimum);
		ui->advOutEncoderLayout_v->addWidget(streamEncoderProps_v_stream);
		// connect(streamEncoderProps_v_stream, &OBSPropertiesView::Changed, this, &OBSBasicSettings::UpdateVerticalStreamDelayEstimate); // TODO if needed
	}

	// streamEncoderChanged_v = true; // Need a new flag if separate tracking is needed
	SetWidgetChanged(ui->advOutEncoder_v);
	// UpdateVerticalStreamDelayEstimate(); // TODO if needed
}

void OBSBasicSettings::on_advOutRecEncoder_currentIndexChanged(int)
{
	if (loading)
		return;

	const char *type = GetCurrentEncoder(ui->advOutRecEncoder);
	if (strcmp(curAdvRecEncoder.c_str(), type) == 0)
		return;

	curAdvRecEncoder = type;

	delete recordEncoderProps;
	recordEncoderProps =
		CreateEncoderPropertyView(type, "recordEncoder.json", true);
	if (recordEncoderProps) {
		recordEncoderProps->setSizePolicy(QSizePolicy::Preferred,
						  QSizePolicy::Minimum);
		ui->advOutRecEncoderLayout->addWidget(recordEncoderProps);
	}

	recordEncoderChanged = true;
	SetWidgetChanged(ui->advOutRecEncoder);
}

void OBSBasicSettings::on_advOutRecEncoder_v_rec_currentIndexChanged(int) // Added for vertical
{
	if (loading)
		return;

	const char *type = GetCurrentEncoder(ui->advOutRecEncoder_v);
	if (strcmp(curAdvRecEncoder_v_rec.c_str(), type) == 0)
		return;

	curAdvRecEncoder_v_rec = type;

	delete recordEncoderProps_v_rec;
	recordEncoderProps_v_rec = CreateEncoderPropertyView(
		type, "recordEncoder_v_rec.json", true);
	if (recordEncoderProps_v_rec) {
		recordEncoderProps_v_rec->setSizePolicy(QSizePolicy::Preferred,
							QSizePolicy::Minimum);
		ui->advOutRecEncoderLayout_v->addWidget(recordEncoderProps_v_rec);
	}

	// recordEncoderChanged_v = true; // Need a new flag if separate tracking is needed
	SetWidgetChanged(ui->advOutRecEncoder_v);
}

void OBSBasicSettings::on_advOutFFVEncoder_currentIndexChanged(int)
{
	if (loading)
		return;

	SetWidgetChanged(ui->advOutFFVEncoder);
}

void OBSBasicSettings::on_advOutFFVEncoder_v_currentIndexChanged(int) // Added for vertical
{
	if (loading)
		return;

	SetWidgetChanged(ui->advOutFFVEncoder_v);
}

void OBSBasicSettings::on_advOutFFAEncoder_currentIndexChanged(int)
{
	if (loading)
		return;

	SetWidgetChanged(ui->advOutFFAEncoder);
}

void OBSBasicSettings::on_advOutFFAEncoder_v_currentIndexChanged(int) // Added for vertical
{
	if (loading)
		return;

	SetWidgetChanged(ui->advOutFFAEncoder_v);
}

void OBSBasicSettings::UpdateAndSaveEncoder(const char *encoder,
					    const char *&curVal,
					    OBSPropertiesView *&props,
					    const char *jsonFile,
					    const char *configName)
{
	if (strcmp(curVal, encoder) != 0) {
		curVal = encoder;
		config_set_string(outputConfig, "AdvOut", configName, encoder);
	}

	WriteJsonData(props, jsonFile);
}

void OBSBasicSettings::LoadAdvOutRescale(QComboBox *rescale,
					 QComboBox *rescaleFilter,
					 const char *rescaleName,
					 const char *filterName)
{
	const char *rescale_val =
		config_get_string(outputConfig, "AdvOut", rescaleName);
	int filter_val =
		config_get_int(outputConfig, "AdvOut", filterName);

	rescale->blockSignals(true);
	rescale->clear();

	char str[1024];
	snprintf(str, sizeof(str), "%s (%dx%d)",
		 QTStr("Basic.Settings.Output.SameAsSource").toUtf8().constData(),
		 main->GetVideoCX(), main->GetVideoCY());
	rescale->addItem(str, QVariant(OBS_SCALE_DISABLE));

	vector<string> list;
	main->GetOutputResolutions(list);
	for (auto &iter : list) {
		rescale->addItem(iter.c_str(), QVariant(OBS_SCALE_OUTPUT));
	}

	rescale->setCurrentText(rescale_val);
	rescale->blockSignals(false);

	rescaleFilter->blockSignals(true);
	rescaleFilter->clear();
	for (const char *const *rescale_str = rescale_quality_strings;
	     *rescale_str; rescale_str++) {
		rescaleFilter->addItem(*rescale_str);
	}

	rescaleFilter->setCurrentIndex(filter_val);
	rescaleFilter->blockSignals(false);
}

void OBSBasicSettings::SaveAdvOutRescale(QComboBox *rescale,
					 QComboBox *rescaleFilter,
					 const char *rescaleName,
					 const char *filterName)
{
	if (WidgetChanged(rescale))
		SaveCombo(rescale, "AdvOut", rescaleName);
	if (WidgetChanged(rescaleFilter))
		config_set_int(outputConfig, "AdvOut", filterName,
			       rescaleFilter->currentIndex());
}

void OBSBasicSettings::SwapMultiTrack(const char *protocol)
{
	bool showMultiTrack = strcmp(protocol, "rtmp_multi") == 0;

	ui->advOutTrack1->setVisible(!showMultiTrack);
	ui->advOutTrack2->setVisible(!showMultiTrack);
	ui->advOutTrack3->setVisible(!showMultiTrack);
	ui->advOutTrack4->setVisible(!showMultiTrack);
	ui->advOutTrack5->setVisible(!showMultiTrack);
	ui->advOutTrack6->setVisible(!showMultiTrack);
	ui->advOutMultiTrack1->setVisible(showMultiTrack);
	ui->advOutMultiTrack2->setVisible(showMultiTrack);
	ui->advOutMultiTrack3->setVisible(showMultiTrack);
	ui->advOutMultiTrack4->setVisible(showMultiTrack);
	ui->advOutMultiTrack5->setVisible(showMultiTrack);
	ui->advOutMultiTrack6->setVisible(showMultiTrack);
}

void OBSBasicSettings::LoadAdvOutputStreamingSettings()
{
	bool streamActive = obs_frontend_streaming_active();
	const char *default_encoder = EncoderAvailable("obs_x264")
					      ? "obs_x264"
					      : "jim_x264";

	InitAdvEncoders(ui->advOutEncoder, default_encoder);
	const char *encoder =
		config_get_string(outputConfig, "AdvOut", "Encoder");
	if (!SetComboByData(ui->advOutEncoder, encoder)) {
		uint32_t caps = obs_get_encoder_caps(encoder);
		if ((caps & ENCODER_HIDE_FLAGS) != 0) {
			QString encName = obs_encoder_get_display_name(encoder);
			if (caps & OBS_ENCODER_CAP_DEPRECATED)
				encName += " (" + QTStr("Deprecated") + ")";
			ui->advOutEncoder->insertItem(0, encName, encoder);
			SetComboByData(ui->advOutEncoder, encoder);
		}
	}

	LoadAdvOutRescale(ui->advOutRescale, ui->advOutRescaleFilter,
			  "RescaleRes", "RescaleFilter");
	ui->advOutUseRescale->setChecked(
		config_get_int(outputConfig, "AdvOut", "RescaleFilter") !=
		OBS_SCALE_DISABLE);

	int trackIndex = config_get_int(outputConfig, "AdvOut", "TrackIndex");
	switch (trackIndex) {
	case 1:
		ui->advOutTrack1->setChecked(true);
		break;
	case 2:
		ui->advOutTrack2->setChecked(true);
		break;
	case 3:
		ui->advOutTrack3->setChecked(true);
		break;
	case 4:
		ui->advOutTrack4->setChecked(true);
		break;
	case 5:
		ui->advOutTrack5->setChecked(true);
		break;
	case 6:
		ui->advOutTrack6->setChecked(true);
		break;
	}

	int audioMixes =
		config_get_int(outputConfig, "AdvOut", "StreamMultiTrackAudioMixes");
	ui->advOutMultiTrack1->setChecked(audioMixes & (1 << 0));
	ui->advOutMultiTrack2->setChecked(audioMixes & (1 << 1));
	ui->advOutMultiTrack3->setChecked(audioMixes & (1 << 2));
	ui->advOutMultiTrack4->setChecked(audioMixes & (1 << 3));
	ui->advOutMultiTrack5->setChecked(audioMixes & (1 << 4));
	ui->advOutMultiTrack6->setChecked(audioMixes & (1 << 5));

	ui->advOutEncoder->setDisabled(streamActive);
	ui->advOutUseRescale->setDisabled(streamActive);
	ui->advOutRescale->setDisabled(streamActive ||
				       !ui->advOutUseRescale->isChecked());
	ui->advOutRescaleFilter->setDisabled(
		streamActive || !ui->advOutUseRescale->isChecked());

	const char *protocol = "";
	OBSService service = main->GetService();
	if (service) {
		obs_data_t *settings = obs_service_get_settings(service);
		protocol = obs_data_get_string(settings, "protocol");
		obs_data_release(settings);
	}

	SwapMultiTrack(protocol);
}

// Function to load Vertical Advanced Streaming Settings
void OBSBasicSettings::LoadAdvOutputStreamingSettings_V()
{
	blog(LOG_INFO, "Loading Vertical Advanced Streaming Settings.");
	bool streamActive = obs_frontend_streaming_active(); // TODO: This should ideally check a vertical-specific stream active flag

	// Encoder
	const char *default_encoder = EncoderAvailable("obs_x264") ? "obs_x264" : "jim_x264";
	InitAdvEncoders(ui->advOutEncoder_v, default_encoder); // Populates the combobox
	const char *encoder_v = config_get_string(outputConfig, "AdvOut", "Encoder_V_Stream");
	if (!SetComboByData(ui->advOutEncoder_v, encoder_v)) {
		uint32_t caps = obs_get_encoder_caps(encoder_v);
		if ((caps & ENCODER_HIDE_FLAGS) != 0) {
			QString encName = obs_encoder_get_display_name(encoder_v);
			if (caps & OBS_ENCODER_CAP_DEPRECATED)
				encName += " (" + QTStr("Deprecated") + ")";
			ui->advOutEncoder_v->insertItem(0, encName, encoder_v);
			SetComboByData(ui->advOutEncoder_v, encoder_v);
		}
	}

	// Rescale Output
	LoadAdvOutRescale(ui->advOutRescale_v, ui->advOutRescaleFilter_v, "RescaleRes_V_Stream", "RescaleFilter_V_Stream");
	ui->advOutUseRescale_v->setChecked(config_get_int(outputConfig, "AdvOut", "RescaleFilter_V_Stream") != OBS_SCALE_DISABLE);
	UpdateAdvOut_V(); // Update enabled state of rescale widgets

	// Audio Track (Assuming vertical stream uses its own audio track selection, separate from horizontal)
	// Note: The UI for multi-track (advOutMultiTrackX_v) was not explicitly added in the .ui changes for vertical stream.
	// If it's needed, those widgets (advOutMultiTrack1_v etc.) must be added to OBSBasicSettings.ui first.
	// For now, using single track selection (advOutTrackX_v).
	int trackIndex_v = config_get_int(outputConfig, "AdvOut", "TrackIndex_V_Stream");
	QRadioButton* trackButtons_v[] = {ui->advOutTrack1_v, ui->advOutTrack2_v, ui->advOutTrack3_v, ui->advOutTrack4_v, ui->advOutTrack5_v, ui->advOutTrack6_v};
	for (int i = 0; i < 6; ++i) {
		if (trackButtons_v[i]) trackButtons_v[i]->setChecked(trackIndex_v == (i + 1));
	}

	// Disabling UI elements if stream is active
	ui->advOutEncoder_v->setDisabled(streamActive);
	ui->advOutUseRescale_v->setDisabled(streamActive);
	// Rescale options are updated by UpdateAdvOut_V based on checkbox and streamActive

	// --- Vertical Advanced Stream Service Settings ---
	// The following code assumes UI elements (e.g., ui->advService_v_stream, ui->advServer_v_stream, ui->advKey_v_stream)
	// for vertical advanced stream service configuration will be added to the UI.
	// Currently, these are placeholders as the UI elements do not exist in advOutputStreamTab_v.

	const char* v_service_type_id = config_get_string(outputConfig, "AdvOut", "VStream_ServiceType");
	const char* v_server_url      = config_get_string(outputConfig, "AdvOut", "VStream_Server");
	const char* v_stream_key      = config_get_string(outputConfig, "AdvOut", "VStream_Key");
	bool v_use_auth           = config_get_bool(outputConfig, "AdvOut", "VStream_UseAuth", false); // Default to not use auth
	const char* v_username        = config_get_string(outputConfig, "AdvOut", "VStream_Username");
	const char* v_password        = config_get_string(outputConfig, "AdvOut", "VStream_Password");
	bool v_show_all           = config_get_bool(outputConfig, "AdvOut", "VStream_ShowAll", false);
	// bool v_ignore_rec         = config_get_bool(outputConfig, "AdvOut", "VStream_IgnoreRec", false); // If an "ignore recs" checkbox is added

	// Placeholder for populating a vertical service list QComboBox (e.g., ui->advService_v_stream)
	// if (ui->advService_v_stream) {
	//     PopulateServiceList(ui->advService_v_stream, v_show_all, v_service_type_id); // Needs a dedicated PopulateServiceList_V or similar
	//     SetComboByData(ui->advService_v_stream, v_service_type_id); // Or SetComboByText if using display names
	//     UpdateVStreamServicePage(); // Helper to manage visibility of connect/disconnect buttons for vertical service
	// }

	// Placeholder for setting server URL (e.g., ui->advServer_v_stream)
	// if (ui->advServer_v_stream) {
	//     ui->advServer_v_stream->setText(v_server_url ? v_server_url : "");
	// }

	// Placeholder for setting stream key (e.g., ui->advKey_v_stream)
	// if (ui->advKey_v_stream) {
	//     ui->advKey_v_stream->setText(v_stream_key ? v_stream_key : "");
	//     // Potentially set echo mode based on service properties if service combo is working
	// }

	// Placeholder for auth related widgets (e.g., ui->advAuthUseStreamKey_v_stream, etc.)
	// if (ui->advAuthUseStreamKey_v_stream) {
	//     ui->advAuthUseStreamKey_v_stream->setChecked(!v_use_auth); // Assuming checkbox means "use key" so !use_auth
	// }
	// if (ui->advAuthUsername_v_stream) {
	//     ui->advAuthUsername_v_stream->setText(v_username ? v_username : "");
	// }
	// if (ui->advAuthPassword_v_stream) {
	//     ui->advAuthPassword_v_stream->setText(v_password ? v_password : "");
	//     // Potentially set echo mode
	// }
	// UpdateVStreamAuthMethod(); // Helper to manage enabled states of auth widgets

	// Placeholder for "Show all services" checkbox for vertical stream
	// if (ui->advShowAllServices_v_stream) {
	//    ui->advShowAllServices_v_stream->setChecked(v_show_all);
	// }

	// Placeholder for "Ignore recommendations" checkbox for vertical stream
	// if (ui->advIgnoreRec_v_stream) {
	//    ui->advIgnoreRec_v_stream->setChecked(v_ignore_rec);
	// }

	blog(LOG_INFO, "LoadAdvOutputStreamingSettings_V: Loaded vertical advanced stream service settings from config. UI interaction is placeholder as widgets are missing.");
	blog(LOG_INFO, "V_ServiceType (config): %s", v_service_type_id ? v_service_type_id : "N/A");
	blog(LOG_INFO, "V_Server (config): %s", v_server_url ? v_server_url : "N/A");
	blog(LOG_INFO, "V_Key (config): %s", v_stream_key ? v_stream_key : "N/A");

	// Actual UI updates would go here if widgets existed
	if (ui->advService_v_stream) { // This widget is hypothetical
		PopulateVStreamServiceList(v_show_all, v_service_type_id);
		UpdateVStreamServerList(); // Depends on selected service
		UpdateVStreamKeyAuthWidgets(); // Depends on selected service
	}
}

void OBSBasicSettings::PopulateVStreamServiceList(bool showAll, const char* current_service_id_v)
{
	if (!ui->advService_v_stream) return; // Guard against missing UI

	ui->advService_v_stream->blockSignals(true);
	ui->advService_v_stream->clear();

	OBSService currentService = nullptr;
	if (current_service_id_v && *current_service_id_v) {
		currentService = obs_service_create(current_service_id_v, "temp_v_service", nullptr, nullptr);
	}

	size_t idx = 0;
	const char *service_id;
	while (obs_enum_service_types(idx++, &service_id)) {
		OBSService service = obs_service_create(service_id, service_id, nullptr, nullptr);
		if (!service) continue;

		if (showAll || strcmp(obs_service_get_type(service), "rtmp_custom") == 0 || !ServiceCanShowAll(service)) {
			ui->advService_v_stream->addItem(obs_service_get_name(service), QVariant::fromValue(service));
		} else {
			obs_service_release(service);
		}
	}

	if (currentService) {
		int currentIdx = ui->advService_v_stream->findData(QVariant::fromValue(currentService));
		if (currentIdx != -1) {
			ui->advService_v_stream->setCurrentIndex(currentIdx);
		} else {
			// If not found (e.g. was hidden by showAll), add it back
			ui->advService_v_stream->addItem(obs_service_get_name(currentService), QVariant::fromValue(currentService));
			ui->advService_v_stream->setCurrentIndex(ui->advService_v_stream->count() - 1);
		}
		// Note: currentService was created with obs_service_create, needs release if not stored in combobox item
		// QVariant stores it, so it should be managed by Qt or released when combo is cleared.
		// However, explicit release if not added might be safer. For now, assuming it's handled.
		// obs_service_release(currentService); // This might be too soon if findData relies on it.
	} else if (ui->advService_v_stream->count() > 0) {
		ui->advService_v_stream->setCurrentIndex(0); // Default to first if no specific one was set
	}

	obs_service_release(currentService); // Release the temporary service used for finding current

	ui->advService_v_stream->blockSignals(false);

	// Trigger dependent updates
	on_advOutVStreamService_currentIndexChanged();
}


void OBSBasicSettings::UpdateVStreamServerList()
{
	if (!ui->advService_v_stream || !ui->advServer_v_stream) return; // Guard

	OBSService service_v = ui->advService_v_stream->currentData().value<OBSService>();
	if (!service_v) return;

	const char *current_server_v = config_get_string(outputConfig, "AdvOut", "VStream_Server");
	const char *type_v = obs_service_get_type(service_v);
	bool custom_v = (strcmp(type_v, "rtmp_custom") == 0);

	obs_properties_t *props = obs_service_properties(service_v);
	obs_property_t *servers_prop = obs_properties_get(props, "server"); // "server" is the typical property name

	ui->advServer_v_stream->blockSignals(true);
	ui->advServer_v_stream->clear();
	ui->advServer_v_stream->setEditable(custom_v);

	if (custom_v) {
		ui->advServer_v_stream->addItem(current_server_v ? current_server_v : "");
	} else if (servers_prop) {
		obs_combo_format format = obs_property_combo_format(servers_prop);
		size_t count = obs_property_list_item_count(servers_prop);
		for (size_t i = 0; i < count; i++) {
			const char *val = (format == OBS_COMBO_FORMAT_STRING)
						? obs_property_list_item_string(servers_prop, i)
						: obs_property_list_item_name(servers_prop, i);
			ui->advServer_v_stream->addItem(val);
		}
	}

	SetComboByText(ui->advServer_v_stream, current_server_v);
	ui->advServer_v_stream->blockSignals(false);
	obs_properties_destroy(props);
}

void OBSBasicSettings::UpdateVStreamKeyAuthWidgets()
{
	if (!ui->advService_v_stream) return; // Guard

	OBSService service_v = ui->advService_v_stream->currentData().value<OBSService>();
	bool canUseKey = false;
	bool canUseUser = false;
	bool canUsePass = false;
	bool useAuthStored = config_get_bool(outputConfig, "AdvOut", "VStream_UseAuth");

	if (service_v) {
		canUseKey = ServiceCanUseKey(service_v); // Assuming ServiceCanUseKey etc. are generic enough
		canUseUser = ServiceCanUseUsername(service_v);
		canUsePass = ServiceCanUsePassword(service_v);
		if (ui->advAuthPassword_v_stream) SetServicePasswordEcho(service_v, ui->advAuthPassword_v_stream);
	}

	if (ui->advAuthUseStreamKey_v_stream) {
		ui->advAuthUseStreamKey_v_stream->setEnabled(canUseKey && (canUseUser || canUsePass));
		ui->advAuthUseStreamKey_v_stream->setChecked(!useAuthStored); // if checkbox means "use key"
	}

	bool useKeyChecked = ui->advAuthUseStreamKey_v_stream ? ui->advAuthUseStreamKey_v_stream->isChecked() : true;

	if (ui->advKey_v_stream) ui->advKey_v_stream->setEnabled(canUseKey && useKeyChecked);
	if (ui->advAuthUsername_v_stream) ui->advAuthUsername_v_stream->setEnabled(canUseUser && !useKeyChecked);
	if (ui->advAuthPassword_v_stream) ui->advAuthPassword_v_stream->setEnabled(canUsePass && !useKeyChecked);

	// Visibility of connect/disconnect account buttons
	// This logic would mirror UpdateServicePage but for vertical stream service UI elements
	// if (ui->advConnectAccount_v_stream && ui->advDisconnectAccount_v_stream) {
	//     obs_properties_t *props = service_v ? obs_service_properties(service_v) : nullptr;
	//     obs_property_t *connect_button = props ? obs_properties_get(props, "connect_account") : nullptr;
	//     obs_property_t *disconnect_button = props ? obs_properties_get(props, "disconnect_account") : nullptr;
	//     ui->advConnectAccount_v_stream->setVisible(!!connect_button);
	//     ui->advDisconnectAccount_v_stream->setVisible(!!disconnect_button);
	//     if (props) obs_properties_destroy(props);
	// }
}


// Function to load Vertical Advanced Streaming Encoder Properties
void OBSBasicSettings::LoadAdvOutputStreamingEncoderProperties_V()
{
	blog(LOG_INFO, "Loading Vertical Advanced Streaming Encoder Properties.");
	const char *type = config_get_string(outputConfig, "AdvOut", "Encoder_V_Stream");
	curAdvStreamEncoder_v_stream = type;

	delete streamEncoderProps_v_stream;
	streamEncoderProps_v_stream = CreateEncoderPropertyView(type, "streamEncoder_v_stream.json");
	if (ui->advOutEncoderLayout_v && streamEncoderProps_v_stream) {
		streamEncoderProps_v_stream->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
		ui->advOutEncoderLayout_v->addWidget(streamEncoderProps_v_stream);
		// connect(streamEncoderProps_v_stream, &OBSPropertiesView::Changed, this, &OBSBasicSettings::UpdateVerticalStreamDelayEstimate); // TODO: If needed
	}

	// Ensure the encoder combo box reflects the loaded type, even if it was hidden
	if (ui->advOutEncoder_v && !SetComboByData(ui->advOutEncoder_v, type)) { // Changed from SetComboByValue
		uint32_t caps = obs_get_encoder_caps(type);
		if ((caps & ENCODER_HIDE_FLAGS) != 0) {
			QString encName = QT_UTF8(obs_encoder_get_display_name(type));
			if (caps & OBS_ENCODER_CAP_DEPRECATED)
				encName += " (" + QTStr("Deprecated") + ")";
			ui->advOutEncoder_v->insertItem(0, encName, QT_UTF8(type));
			SetComboByData(ui->advOutEncoder_v, type); // Changed from SetComboByValue
		}
	}
	// UpdateVerticalStreamDelayEstimate(); // TODO: If applicable
}

void OBSBasicSettings::on_advOutVStreamService_currentIndexChanged()
{
    if (loading) return;

    // This assumes ui->advService_v_stream is the QComboBox for vertical service selection
    if (!ui->advService_v_stream) return;

    OBSService service_v = ui->advService_v_stream->currentData().value<OBSService>();
    if (!service_v) return; // Should not happen if list is populated

    // Store the selected service ID in a temporary variable or directly update config if appropriate
    // For now, let's assume we are not directly setting it on a global "main vertical service" object
    // like `main->SetService_V(service_v);` as that doesn't exist.
    // The config will be updated during Save.

    const char *type = obs_service_get_type(service_v);
    if (ui->advShowAllServices_v_stream) { // Hypothetical checkbox for "show all" for vertical
        if (strcmp(type, "rtmp_custom") == 0) {
            ui->advShowAllServices_v_stream->setEnabled(true);
        } else {
            ui->advShowAllServices_v_stream->setEnabled(ServiceCanShowAll(service_v));
        }
    }

    if (ui->advServer_v_stream) ui->advServer_v_stream->setEditable(strcmp(type, "rtmp_custom") == 0);

    UpdateVStreamServerList();
    UpdateVStreamKeyAuthWidgets();
    // UpdateVStreamServiceRecommendations(); // If implemented
    // UpdateVStreamBandwidthTestEnable(); // If implemented

    SetWidgetChanged(ui->advService_v_stream);
}

void OBSBasicSettings::on_advOutVStreamUseAuth_clicked()
{
    if (loading) return;
    if (!ui->advAuthUseStreamKey_v_stream) return; // Guard

    UpdateVStreamKeyAuthWidgets();
    SetWidgetChanged(ui->advAuthUseStreamKey_v_stream);
}

void OBSBasicSettings::on_advOutVStreamShowAll_clicked()
{
    if (loading) return;
    if (!ui->advService_v_stream || !ui->advShowAllServices_v_stream) return; // Guard

    const char* current_service_id_v = nullptr;
    OBSService current_selection = ui->advService_v_stream->currentData().value<OBSService>();
    if(current_selection) current_service_id_v = obs_service_get_id(current_selection);

    PopulateVStreamServiceList(ui->advShowAllServices_v_stream->isChecked(), current_service_id_v);
    SetWidgetChanged(ui->advShowAllServices_v_stream);
}


// Function to save Vertical Advanced Streaming Settings
void OBSBasicSettings::SaveAdvOutputStreamingSettings_V()
{
	blog(LOG_INFO, "Saving Vertical Advanced Streaming Settings.");
	// Encoder
	if (WidgetChanged(ui->advOutEncoder_v))
		config_set_string(outputConfig, "AdvOut", "Encoder_V_Stream", GetCurrentEncoder(ui->advOutEncoder_v));

	// Rescale Output
	SaveAdvOutRescale(ui->advOutRescale_v, ui->advOutRescaleFilter_v, "RescaleRes_V_Stream", "RescaleFilter_V_Stream");
	config_set_int(outputConfig, "AdvOut", "RescaleFilter_V_Stream", ui->advOutUseRescale_v->isChecked() ? ui->advOutRescaleFilter_v->currentIndex() : OBS_SCALE_DISABLE);

	// Audio Track
	int trackIndex_v = 0;
	QRadioButton* trackButtons_v[] = {ui->advOutTrack1_v, ui->advOutTrack2_v, ui->advOutTrack3_v, ui->advOutTrack4_v, ui->advOutTrack5_v, ui->advOutTrack6_v};
	bool trackChanged_v = false;
	for(int i=0; i<6; ++i) {
		if (trackButtons_v[i] && WidgetChanged(trackButtons_v[i])) trackChanged_v = true;
		if (trackButtons_v[i] && trackButtons_v[i]->isChecked()) trackIndex_v = i + 1;
	}
	if (trackChanged_v) {
		config_set_int(outputConfig, "AdvOut", "TrackIndex_V_Stream", trackIndex_v);
	}
	// Note: Save logic for multi-track audio mixes (advOutMultiTrackX_v) would go here if those UI elements were added and used.

	// Encoder Properties
	WriteJsonData(streamEncoderProps_v_stream, "streamEncoder_v_stream.json");

	// --- Vertical Advanced Stream Service Settings ---
	// The following code assumes UI elements (e.g., ui->advService_v_stream, ui->advServer_v_stream, ui->advKey_v_stream)
	// for vertical advanced stream service configuration will be added to the UI.
	// Currently, these are placeholders as the UI elements do not exist in advOutputStreamTab_v.

	// Placeholder: Saving Service Type from a QComboBox (e.g., ui->advService_v_stream)
	// if (ui->advService_v_stream && WidgetChanged(ui->advService_v_stream)) {
	//     OBSService service_v = ui->advService_v_stream->currentData().value<OBSService>();
	//     if (service_v) {
	//         config_set_string(outputConfig, "AdvOut", "VStream_ServiceType", obs_service_get_id(service_v));
	//     } else {
	//         config_remove_value(outputConfig, "AdvOut", "VStream_ServiceType");
	//     }
	// }

	// Placeholder: Saving Server URL from a QLineEdit (e.g., ui->advServer_v_stream)
	// if (ui->advServer_v_stream && WidgetChanged(ui->advServer_v_stream)) {
	//     config_set_string(outputConfig, "AdvOut", "VStream_Server", ui->advServer_v_stream->text().toUtf8().constData());
	// }

	// Placeholder: Saving Stream Key from a QLineEdit (e.g., ui->advKey_v_stream)
	// if (ui->advKey_v_stream && WidgetChanged(ui->advKey_v_stream)) {
	//     config_set_string(outputConfig, "AdvOut", "VStream_Key", ui->advKey_v_stream->text().toUtf8().constData());
	// }

	// Placeholder: Saving Authentication settings
	// if (ui->advAuthUseStreamKey_v_stream && WidgetChanged(ui->advAuthUseStreamKey_v_stream)) {
	//     bool useAuth_v = !ui->advAuthUseStreamKey_v_stream->isChecked(); // Assuming checkbox means "use key"
	//     config_set_bool(outputConfig, "AdvOut", "VStream_UseAuth", useAuth_v);
	//     if (useAuth_v) {
	//         if (ui->advAuthUsername_v_stream && WidgetChanged(ui->advAuthUsername_v_stream))
	//             config_set_string(outputConfig, "AdvOut", "VStream_Username", ui->advAuthUsername_v_stream->text().toUtf8().constData());
	//         if (ui->advAuthPassword_v_stream && WidgetChanged(ui->advAuthPassword_v_stream))
	//             config_set_string(outputConfig, "AdvOut", "VStream_Password", ui->advAuthPassword_v_stream->text().toUtf8().constData());
	//     } else {
	//         config_remove_value(outputConfig, "AdvOut", "VStream_Username");
	//         config_remove_value(outputConfig, "AdvOut", "VStream_Password");
	//     }
	// }

	// Placeholder: Saving "Show all services"
	// if (ui->advShowAllServices_v_stream && WidgetChanged(ui->advShowAllServices_v_stream)) {
	//    config_set_bool(outputConfig, "AdvOut", "VStream_ShowAll", ui->advShowAllServices_v_stream->isChecked());
	// }

	// Placeholder: Saving "Ignore recommendations"
	// if (ui->advIgnoreRec_v_stream && WidgetChanged(ui->advIgnoreRec_v_stream)) {
	//    config_set_bool(outputConfig, "AdvOut", "VStream_IgnoreRec", ui->advIgnoreRec_v_stream->isChecked());
	// }


	// --- Save Vertical Advanced Stream Service Settings ---
	// These assume UI elements like ui->advService_v_stream, ui->advServer_v_stream etc. exist and have `WidgetChanged` correctly set.

	if (ui->advService_v_stream && WidgetChanged(ui->advService_v_stream)) {
		OBSService service_v = ui->advService_v_stream->currentData().value<OBSService>();
		if (service_v) {
			config_set_string(outputConfig, "AdvOut", "VStream_ServiceType", obs_service_get_id(service_v));
		} else {
			config_remove_value(outputConfig, "AdvOut", "VStream_ServiceType");
		}
	}

	if (ui->advServer_v_stream && WidgetChanged(ui->advServer_v_stream)) {
		config_set_string(outputConfig, "AdvOut", "VStream_Server", ui->advServer_v_stream->currentText().toUtf8().constData());
	}

	bool useAuth_v = false;
	if (ui->advAuthUseStreamKey_v_stream) { // Check if the checkbox itself exists
	useAuth_v = !ui->advAuthUseStreamKey_v_stream->isChecked(); // True if "Use Authentication" (i.e. user/pass) is selected
	if (WidgetChanged(ui->advAuthUseStreamKey_v_stream)) {
		config_set_bool(outputConfig, "AdvOut", "VStream_UseAuth", useAuth_v);
	}
	}


	if (useAuth_v) {
		if (ui->advAuthUsername_v_stream && WidgetChanged(ui->advAuthUsername_v_stream)) {
			config_set_string(outputConfig, "AdvOut", "VStream_Username", ui->advAuthUsername_v_stream->text().toUtf8().constData());
		}
		if (ui->advAuthPassword_v_stream && WidgetChanged(ui->advAuthPassword_v_stream)) {
			config_set_string(outputConfig, "AdvOut", "VStream_Password", ui->advAuthPassword_v_stream->text().toUtf8().constData());
		}
	} else { // Using stream key or no auth if key is also disabled
		if (ui->advKey_v_stream && WidgetChanged(ui->advKey_v_stream)) {
			config_set_string(outputConfig, "AdvOut", "VStream_Key", ui->advKey_v_stream->text().toUtf8().constData());
		}
		// When switching to stream key, clear username/password from config if they were set for auth
		if (WidgetChanged(ui->advAuthUseStreamKey_v_stream) && !useAuth_v) {
			config_remove_value(outputConfig, "AdvOut", "VStream_Username");
			config_remove_value(outputConfig, "AdvOut", "VStream_Password");
		}
	}


	if (ui->advShowAllServices_v_stream && WidgetChanged(ui->advShowAllServices_v_stream)) {
	   config_set_bool(outputConfig, "AdvOut", "VStream_ShowAll", ui->advShowAllServices_v_stream->isChecked());
	}

	// Example for a hypothetical "Ignore Recommendations" checkbox for vertical stream
	// if (ui->advIgnoreRec_v_stream && WidgetChanged(ui->advIgnoreRec_v_stream)) {
	//    config_set_bool(outputConfig, "AdvOut", "VStream_IgnoreRec", ui->advIgnoreRec_v_stream->isChecked());
	// }

	blog(LOG_INFO, "SaveAdvOutputStreamingSettings_V: Vertical advanced stream settings processed.");
}

// Function to load Vertical Advanced Standard Recording Settings
void OBSBasicSettings::LoadAdvOutputRecordingSettings_V()
{
	blog(LOG_INFO, "Loading Vertical Advanced Standard Recording Settings.");
	// TODO: Check vertical-specific recording/replay active flags if they become independent
	bool recordActive = obs_frontend_recording_active();
	bool replayActive = obs_frontend_replay_buffer_active();

	// Recording Type (Standard, FFmpeg - though FFmpeg is separate tab)
	// Assuming "Standard" type (0) or "Custom Output (FFmpeg)" (1) for advOutRecType_v,
	// but FFmpeg settings are on another tab. Here we focus on "Standard" if type is 0.
	// The type here usually controls if RecEncoder is used or if it's "Same as Stream" etc.
	// For dual output, "Same as Stream" needs context (Horizontal or Vertical Stream).
	// The UI for advOutRecType_v has "Standard" and "Custom Output (FFmpeg)".
	// If "Standard", then RecEncoder_V_Rec is used.
	ui->advOutRecType_v->setCurrentIndex(config_get_int(outputConfig, "AdvOut", "RecType_V_Rec")); // 0 for Standard, 1 for FFmpeg
	UpdateAdvStandardRecording_V(); // This will enable/disable advOutRecEncoder_v based on type

	// Recording Path
	QString defaultRecPath_v = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
	ui->advOutRecPath_v->setText(config_get_string(outputConfig, "AdvOut", "RecFilePath_V_Rec", defaultRecPath_v.toUtf8().constData()));

	// Recording Format
	SetComboByText(ui->advOutRecFormat_v, config_get_string(outputConfig, "AdvOut", "RecFormat_V_Rec"));

	// Recording Encoder (Standard type)
	const char *default_rec_encoder_v = EncoderAvailable("obs_x264") ? "obs_x264" : "jim_x264";
	InitAdvRecEncoders(ui->advOutRecEncoder_v, default_rec_encoder_v); // Populates combobox
	const char *rec_encoder_v = config_get_string(outputConfig, "AdvOut", "RecEncoder_V_Rec");
	if (!SetComboByData(ui->advOutRecEncoder_v, rec_encoder_v)) {
		uint32_t caps = obs_get_encoder_caps(rec_encoder_v);
		if ((caps & ENCODER_HIDE_FLAGS) != 0) {
			QString encName = obs_encoder_get_display_name(rec_encoder_v);
			if (caps & OBS_ENCODER_CAP_DEPRECATED)
				encName += " (" + QTStr("Deprecated") + ")";
			ui->advOutRecEncoder_v->insertItem(0, encName, rec_encoder_v);
			SetComboByData(ui->advOutRecEncoder_v, rec_encoder_v);
		}
	}

	// Audio Tracks for Recording
	int recTracks_v = config_get_int(outputConfig, "AdvOut", "RecTracks_V_Rec");
	ui->advOutRecTrack1_v->setChecked(recTracks_v & (1 << 0));
	ui->advOutRecTrack2_v->setChecked(recTracks_v & (1 << 1));
	ui->advOutRecTrack3_v->setChecked(recTracks_v & (1 << 2));
	ui->advOutRecTrack4_v->setChecked(recTracks_v & (1 << 3));
	ui->advOutRecTrack5_v->setChecked(recTracks_v & (1 << 4));
	ui->advOutRecTrack6_v->setChecked(recTracks_v & (1 << 5));

	// Filename Formatting & Overwrite (assuming these are global or we need _V_Rec keys)
	// For now, let's assume vertical recording uses the same filename formatting and overwrite settings as horizontal.
	// If they need to be separate, new config keys and UI elements would be required.
	// ui->filenameFormatting_v->setText(config_get_string(outputConfig, "Output", "FilenameFormatting_V_Rec"));
	// ui->overwriteIfExists_v->setChecked(config_get_bool(outputConfig, "Output", "OverwriteIfExists_V_Rec"));


	// Disabling UI elements if recording/replay is active
	bool disableRecControls = recordActive || replayActive;
	ui->advOutRecPathBrowse_v->setDisabled(disableRecControls);
	ui->advOutRecPath_v->setDisabled(disableRecControls);
	ui->advOutRecFormat_v->setDisabled(disableRecControls);
	ui->advOutRecType_v->setDisabled(disableRecControls);
	// advOutRecEncoder_v disable state is handled by UpdateAdvStandardRecording_V based on type and disableRecControls
	UpdateAdvStandardRecording_V(); // Call again to ensure encoder state is correct after loading type
}

// Function to load Vertical Advanced Recording Encoder Properties
void OBSBasicSettings::LoadAdvOutputRecordingEncoderProperties_V()
{
	blog(LOG_INFO, "Loading Vertical Advanced Recording Encoder Properties.");
	const char *type = config_get_string(outputConfig, "AdvOut", "RecEncoder_V_Rec"); // Use outputConfig
	curAdvRecEncoder_v_rec = type;

	delete recordEncoderProps_v_rec;
	recordEncoderProps_v_rec = CreateEncoderPropertyView(type, "recordEncoder_v_rec.json");
	if (ui->advOutRecEncoderLayout_v && recordEncoderProps_v_rec) {
		recordEncoderProps_v_rec->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
		ui->advOutRecEncoderLayout_v->addWidget(recordEncoderProps_v_rec);
	}

	if (ui->advOutRecEncoder_v && !SetComboByData(ui->advOutRecEncoder_v, type)) { // Changed from SetComboByValue
        uint32_t caps = obs_get_encoder_caps(type);
        if ((caps & ENCODER_HIDE_FLAGS) != 0) {
            QString encName = QT_UTF8(obs_encoder_get_display_name(type));
            if (caps & OBS_ENCODER_CAP_DEPRECATED)
                encName += " (" + QTStr("Deprecated") + ")";
            ui->advOutRecEncoder_v->insertItem(0, encName, QT_UTF8(type));
            SetComboByData(ui->advOutRecEncoder_v, type); // Changed from SetComboByValue
        }
    }
}

// Function to save Vertical Advanced Standard Recording Settings
void OBSBasicSettings::SaveAdvOutputRecordingSettings_V()
{
	blog(LOG_INFO, "Saving Vertical Advanced Standard Recording Settings.");
	// Recording Type
	if (WidgetChanged(ui->advOutRecType_v))
		config_set_int(outputConfig, "AdvOut", "RecType_V_Rec", ui->advOutRecType_v->currentIndex());

	// Recording Path
	if (WidgetChanged(ui->advOutRecPath_v))
		config_set_string(outputConfig, "AdvOut", "RecFilePath_V_Rec", ui->advOutRecPath_v->text().toUtf8().constData());

	// Recording Format
	if (WidgetChanged(ui->advOutRecFormat_v))
		SaveCombo(ui->advOutRecFormat_v, "AdvOut", "RecFormat_V_Rec");

	// Recording Encoder (Standard type)
	if (WidgetChanged(ui->advOutRecEncoder_v))
		config_set_string(outputConfig, "AdvOut", "RecEncoder_V_Rec", GetCurrentEncoder(ui->advOutRecEncoder_v));

	// Audio Tracks for Recording
	int recTracks_v = 0;
	if (ui->advOutRecTrack1_v->isChecked()) recTracks_v |= (1 << 0);
	if (ui->advOutRecTrack2_v->isChecked()) recTracks_v |= (1 << 1);
	if (ui->advOutRecTrack3_v->isChecked()) recTracks_v |= (1 << 2);
	if (ui->advOutRecTrack4_v->isChecked()) recTracks_v |= (1 << 3);
	if (ui->advOutRecTrack5_v->isChecked()) recTracks_v |= (1 << 4);
	if (ui->advOutRecTrack6_v->isChecked()) recTracks_v |= (1 << 5);

	// Check if any track checkbox changed state
	bool trackChanged = WidgetChanged(ui->advOutRecTrack1_v) || WidgetChanged(ui->advOutRecTrack2_v) ||
	                    WidgetChanged(ui->advOutRecTrack3_v) || WidgetChanged(ui->advOutRecTrack4_v) ||
	                    WidgetChanged(ui->advOutRecTrack5_v) || WidgetChanged(ui->advOutRecTrack6_v);
	if (trackChanged) {
		config_set_int(outputConfig, "AdvOut", "RecTracks_V_Rec", recTracks_v);
	}

	// Encoder Properties
	WriteJsonData(recordEncoderProps_v_rec, "recordEncoder_v_rec.json");

	// Filename Formatting & Overwrite (if these become vertical-specific)
	// if (WidgetChanged(ui->filenameFormatting_v))
	//     config_set_string(outputConfig, "Output", "FilenameFormatting_V_Rec", ui->filenameFormatting_v->text().toUtf8().constData());
	// if (WidgetChanged(ui->overwriteIfExists_v))
	//     config_set_bool(outputConfig, "Output", "OverwriteIfExists_V_Rec", ui->overwriteIfExists_v->isChecked());
}

// Function to load Vertical Advanced FFmpeg Recording Settings
void OBSBasicSettings::LoadAdvOutputFFmpegSettings_V()
{
	blog(LOG_INFO, "Loading Vertical Advanced FFmpeg Recording Settings.");
	// TODO: Check vertical-specific recording/replay active flags
	bool recordActive = obs_frontend_recording_active();
	bool replayActive = obs_frontend_replay_buffer_active();

	// FFmpeg Output Type (File or URL)
	// Assuming ui->advOutFFOutputToFile_v radio button exists (or similar control)
	// bool toFile = config_get_bool(outputConfig, "AdvOut", "FFOutputToFile_V_Rec");
	// ui->advOutFFOutputToFile_v->setChecked(toFile);
	// ui->advOutFFOutputToUrl_v->setChecked(!toFile);
	// UpdateFFmpegPathOrURL_V(toFile); // Helper to enable/disable path vs URL input

	// Recording Path (if to file) / URL
	QString defaultFFRecPath_v = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
	ui->advOutFFRecPath_v->setText(config_get_string(outputConfig, "AdvOut", "FFFilePath_V_Rec", defaultFFRecPath_v.toUtf8().constData()));
	// ui->advOutFFURL_v->setText(config_get_string(outputConfig, "AdvOut", "FFURL_V_Rec"));


	// Container Format
	SetComboByText(ui->advOutFFFormat_v, config_get_string(outputConfig, "AdvOut", "FFFormat_V_Rec"));

	// Video Encoder & Bitrate
	const char *default_venc_v = EncoderAvailable("obs_x264") ? "obs_x264" : "jim_x264"; // Example default
	InitAdvFFVEncoders(ui->advOutFFVEncoder_v, default_venc_v);
	SetComboByData(ui->advOutFFVEncoder_v, config_get_string(outputConfig, "AdvOut", "FFVEncoder_V_Rec"));
	ui->advOutFFVBitrate_v->setValue(config_get_int(outputConfig, "AdvOut", "FFVBitrate_V_Rec"));

	// Audio Encoder & Bitrate
	const char *default_aenc_v = EncoderAvailable("ffmpeg_aac") ? "ffmpeg_aac" : "libfdk_aac"; // Example default
	InitAdvFFAEncoders(ui->advOutFFAEncoder_v, default_aenc_v);
	SetComboByData(ui->advOutFFAEncoder_v, config_get_string(outputConfig, "AdvOut", "FFAEncoder_V_Rec"));
	ui->advOutFFABitrate_v->setValue(config_get_int(outputConfig, "AdvOut", "FFABitrate_V_Rec"));

	// Muxer Settings
	ui->advOutFFMuxer_v->setText(config_get_string(outputConfig, "AdvOut", "FFMCustom_V_Rec"));

	// Rescale Output
	LoadAdvOutRescale(ui->advOutFFRescale_v, ui->advOutFFRescaleFilter_v, "FFRescaleRes_V_Rec", "FFRescaleFilter_V_Rec");
	ui->advOutFFUseRescale_v->setChecked(config_get_int(outputConfig, "AdvOut", "FFRescaleFilter_V_Rec") != OBS_SCALE_DISABLE);
	UpdateAdvFFOut_V(); // Update enabled state

	// Audio Tracks
	int ffTracks_v = config_get_int(outputConfig, "AdvOut", "FFTracks_V_Rec");
	ui->advOutFFTrack1_v->setChecked(ffTracks_v & (1 << 0));
	ui->advOutFFTrack2_v->setChecked(ffTracks_v & (1 << 1));
	ui->advOutFFTrack3_v->setChecked(ffTracks_v & (1 << 2));
	ui->advOutFFTrack4_v->setChecked(ffTracks_v & (1 << 3));
	ui->advOutFFTrack5_v->setChecked(ffTracks_v & (1 << 4));
	ui->advOutFFTrack6_v->setChecked(ffTracks_v & (1 << 5));

	// Disable relevant controls if recording/replay is active
	bool disableRecControls = recordActive || replayActive;
	// ui->advOutFFOutputToFile_v->setDisabled(disableRecControls);
	// ui->advOutFFOutputToUrl_v->setDisabled(disableRecControls);
	ui->advOutFFRecPathBrowse_v->setDisabled(disableRecControls /*|| !toFile*/);
	ui->advOutFFRecPath_v->setDisabled(disableRecControls /*|| !toFile*/);
	// ui->advOutFFURL_v->setDisabled(disableRecControls /*|| toFile*/);
	ui->advOutFFFormat_v->setDisabled(disableRecControls);
	ui->advOutFFVEncoder_v->setDisabled(disableRecControls);
	ui->advOutFFVBitrate_v->setDisabled(disableRecControls);
	ui->advOutFFAEncoder_v->setDisabled(disableRecControls);
	ui->advOutFFABitrate_v->setDisabled(disableRecControls);
	ui->advOutFFMuxer_v->setDisabled(disableRecControls);
	ui->advOutFFUseRescale_v->setDisabled(disableRecControls);
	// Rescale options are updated by UpdateAdvFFOut_V
}

// Function to save Vertical Advanced FFmpeg Recording Settings
void OBSBasicSettings::SaveAdvOutputFFmpegSettings_V()
{
	blog(LOG_INFO, "Saving Vertical Advanced FFmpeg Recording Settings.");

	// FFmpeg Output Type
	// bool toFile = ui->advOutFFOutputToFile_v->isChecked();
	// config_set_bool(outputConfig, "AdvOut", "FFOutputToFile_V_Rec", toFile);
	// if (WidgetChanged(ui->advOutFFOutputToFile_v) || WidgetChanged(ui->advOutFFOutputToUrl_v)) { /* Mark changed */ }


	// Recording Path / URL
	// if (toFile) {
		if (WidgetChanged(ui->advOutFFRecPath_v))
			config_set_string(outputConfig, "AdvOut", "FFFilePath_V_Rec", ui->advOutFFRecPath_v->text().toUtf8().constData());
	// } else {
	//    if (WidgetChanged(ui->advOutFFURL_v))
	//        config_set_string(outputConfig, "AdvOut", "FFURL_V_Rec", ui->advOutFFURL_v->text().toUtf8().constData());
	// }

	// Container Format
	if (WidgetChanged(ui->advOutFFFormat_v))
		SaveCombo(ui->advOutFFFormat_v, "AdvOut", "FFFormat_V_Rec");

	// Video Encoder & Bitrate
	if (WidgetChanged(ui->advOutFFVEncoder_v))
		config_set_string(outputConfig, "AdvOut", "FFVEncoder_V_Rec", GetCurrentEncoder(ui->advOutFFVEncoder_v));
	if (WidgetChanged(ui->advOutFFVBitrate_v))
		config_set_int(outputConfig, "AdvOut", "FFVBitrate_V_Rec", ui->advOutFFVBitrate_v->value());

	// Audio Encoder & Bitrate
	if (WidgetChanged(ui->advOutFFAEncoder_v))
		config_set_string(outputConfig, "AdvOut", "FFAEncoder_V_Rec", GetCurrentEncoder(ui->advOutFFAEncoder_v));
	if (WidgetChanged(ui->advOutFFABitrate_v))
		config_set_int(outputConfig, "AdvOut", "FFABitrate_V_Rec", ui->advOutFFABitrate_v->value());

	// Muxer Settings
	if (WidgetChanged(ui->advOutFFMuxer_v))
		config_set_string(outputConfig, "AdvOut", "FFMCustom_V_Rec", ui->advOutFFMuxer_v->text().toUtf8().constData());

	// Rescale Output
	SaveAdvOutRescale(ui->advOutFFRescale_v, ui->advOutFFRescaleFilter_v, "FFRescaleRes_V_Rec", "FFRescaleFilter_V_Rec");
	config_set_int(outputConfig, "AdvOut", "FFRescaleFilter_V_Rec", ui->advOutFFUseRescale_v->isChecked() ? ui->advOutFFRescaleFilter_v->currentIndex() : OBS_SCALE_DISABLE);

	// Audio Tracks
	int ffTracks_v = 0;
	if (ui->advOutFFTrack1_v->isChecked()) ffTracks_v |= (1 << 0);
	if (ui->advOutFFTrack2_v->isChecked()) ffTracks_v |= (1 << 1);
	if (ui->advOutFFTrack3_v->isChecked()) ffTracks_v |= (1 << 2);
	if (ui->advOutFFTrack4_v->isChecked()) ffTracks_v |= (1 << 3);
	if (ui->advOutFFTrack5_v->isChecked()) ffTracks_v |= (1 << 4);
	if (ui->advOutFFTrack6_v->isChecked()) ffTracks_v |= (1 << 5);

	bool trackChanged = WidgetChanged(ui->advOutFFTrack1_v) || WidgetChanged(ui->advOutFFTrack2_v) ||
	                    WidgetChanged(ui->advOutFFTrack3_v) || WidgetChanged(ui->advOutFFTrack4_v) ||
	                    WidgetChanged(ui->advOutFFTrack5_v) || WidgetChanged(ui->advOutFFTrack6_v);
	if (trackChanged) {
		config_set_int(outputConfig, "AdvOut", "FFTracks_V_Rec", ffTracks_v);
	}
}

OBSPropertiesView *OBSBasicSettings::CreateEncoderPropertyView(
	const char *encoder, const char *path, bool changed)
{
	ConfigOpen scenecol(outputConfig);
	ConfigOpen current(main->Config());
	ConfigOpen defaults(main->Defaults());

	OBSPropertiesView *props = new OBSPropertiesView(
		encoder, defaults, current, scenecol, path, changed);
	props->setParent(this);
	return props;
}

void OBSBasicSettings::LoadAdvOutputStreamingEncoderProperties()
{
	const char *type =
		config_get_string(main->Config(), "AdvOut", "Encoder");
	curAdvStreamEncoder = type;

	delete streamEncoderProps;
	streamEncoderProps =
		CreateEncoderPropertyView(type, "streamEncoder.json");
	if (ui->advOutEncoderLayout && streamEncoderProps) {
		streamEncoderProps->setSizePolicy(QSizePolicy::Preferred,
						  QSizePolicy::Minimum);
		ui->advOutEncoderLayout->addWidget(streamEncoderProps);
		connect(streamEncoderProps, &OBSPropertiesView::Changed, this,
			&OBSBasicSettings::UpdateStreamDelayEstimate);
	}
}

void OBSBasicSettings::LoadAdvOutputRecordingEncoderProperties()
{
	const char *type =
		config_get_string(main->Config(), "AdvOut", "RecEncoder");
	curAdvRecEncoder = type;

	delete recordEncoderProps;
	recordEncoderProps =
		CreateEncoderPropertyView(type, "recordEncoder.json");
	if (ui->advOutRecEncoderLayout && recordEncoderProps) {
		recordEncoderProps->setSizePolicy(QSizePolicy::Preferred,
						  QSizePolicy::Minimum);
		ui->advOutRecEncoderLayout->addWidget(recordEncoderProps);
	}
}

void OBSBasicSettings::LoadAdvOutputRecordingSettings()
{
	bool recordActive = obs_frontend_recording_active();
	bool replayActive = obs_frontend_replay_buffer_active();

	const char *default_encoder = EncoderAvailable("obs_x264")
					      ? "obs_x264"
					      : "jim_x264";
	InitAdvRecEncoders(ui->advOutRecEncoder, default_encoder);

	const char *encoder =
		config_get_string(outputConfig, "AdvOut", "RecEncoder");
	if (!SetComboByData(ui->advOutRecEncoder, encoder)) {
		uint32_t caps = obs_get_encoder_caps(encoder);
		if ((caps & ENCODER_HIDE_FLAGS) != 0) {
			QString encName = obs_encoder_get_display_name(encoder);
			if (caps & OBS_ENCODER_CAP_DEPRECATED)
				encName += " (" + QTStr("Deprecated") + ")";
			ui->advOutRecEncoder->insertItem(0, encName, encoder);
			SetComboByData(ui->advOutRecEncoder, encoder);
		}
	}

	QString 온도니_path =
		QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
	ui->advOutRecPath->setText(config_get_string(
		outputConfig, "AdvOut", "RecFilePath",
		온도니_path.toUtf8().constData()));
	SetComboByText(
		ui->advOutRecFormat,
		config_get_string(outputConfig, "AdvOut", "RecFormat"));
	ui->advOutRecType->setCurrentIndex(
		config_get_int(outputConfig, "AdvOut", "RecType"));

	int recTracks = config_get_int(outputConfig, "AdvOut", "RecTracks");
	ui->advOutRecTrack1->setChecked(recTracks & (1 << 0));
	ui->advOutRecTrack2->setChecked(recTracks & (1 << 1));
	ui->advOutRecTrack3->setChecked(recTracks & (1 << 2));
	ui->advOutRecTrack4->setChecked(recTracks & (1 << 3));
	ui->advOutRecTrack5->setChecked(recTracks & (1 << 4));
	ui->advOutRecTrack6->setChecked(recTracks & (1 << 5));

	ui->filenameFormatting->setText(config_get_string(
		outputConfig, "Output", "FilenameFormatting"));
	ui->overwriteIfExists->setChecked(
		config_get_bool(outputConfig, "Output", "OverwriteIfExists"));

	ui->advOutRecPathBrowse->setDisabled(recordActive || replayActive);
	ui->advOutRecPath->setDisabled(recordActive || replayActive);
	ui->advOutRecFormat->setDisabled(recordActive || replayActive);
	ui->advOutRecEncoder->setDisabled(
		recordActive || replayActive ||
		ui->advOutRecType->currentIndex() == 0);
	ui->advOutRecType->setDisabled(recordActive || replayActive);
	ui->filenameFormatting->setDisabled(recordActive || replayActive);
	ui->overwriteIfExists->setDisabled(recordActive || replayActive);
}

void OBSBasicSettings::LoadAdvOutputFFmpegSettings()
{
	bool recordActive = obs_frontend_recording_active();
	bool replayActive = obs_frontend_replay_buffer_active();

	const char *default_venc = EncoderAvailable("obs_x264")
					   ? "obs_x264"
					   : "jim_x264";
	const char *default_aenc = EncoderAvailable("libfdk_aac")
					   ? "libfdk_aac"
					   : "ffmpeg_aac";

	InitAdvFFVEncoders(ui->advOutFFVEncoder, default_venc);
	InitAdvFFAEncoders(ui->advOutFFAEncoder, default_aenc);

	SetComboByData(
		ui->advOutFFVEncoder,
		config_get_string(outputConfig, "AdvOut", "FFVEncoder"));
	SetComboByData(
		ui->advOutFFAEncoder,
		config_get_string(outputConfig, "AdvOut", "FFAEncoder"));
	SetComboByText(ui->advOutFFFormat,
		       config_get_string(outputConfig, "AdvOut", "FFFormat"));
	QString 온도니_path =
		QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
	ui->advOutFFRecPath->setText(config_get_string(
		outputConfig, "AdvOut", "FFFilePath",
		온도니_path.toUtf8().constData()));
	ui->advOutFFVBitrate->setValue(
		config_get_int(outputConfig, "AdvOut", "FFVBitrate"));
	ui->advOutFFABitrate->setValue(
		config_get_int(outputConfig, "AdvOut", "FFABitrate"));
	ui->advOutFFMuxer->setText(
		config_get_string(outputConfig, "AdvOut", "FFMCustom"));

	LoadAdvOutRescale(ui->advOutFFRescale, ui->advOutFFRescaleFilter,
			  "FFRescaleRes", "FFRescaleFilter");
	ui->advOutFFUseRescale->setChecked(
		config_get_int(outputConfig, "AdvOut", "FFRescaleFilter") !=
		OBS_SCALE_DISABLE);

	int ffTracks = config_get_int(outputConfig, "AdvOut", "FFTracks");
	ui->advOutFFTrack1->setChecked(ffTracks & (1 << 0));
	ui->advOutFFTrack2->setChecked(ffTracks & (1 << 1));
	ui->advOutFFTrack3->setChecked(ffTracks & (1 << 2));
	ui->advOutFFTrack4->setChecked(ffTracks & (1 << 3));
	ui->advOutFFTrack5->setChecked(ffTracks & (1 << 4));
	ui->advOutFFTrack6->setChecked(ffTracks & (1 << 5));

	ui->advOutFFRecPathBrowse->setDisabled(recordActive || replayActive);
	ui->advOutFFRecPath->setDisabled(recordActive || replayActive);
	ui->advOutFFFormat->setDisabled(recordActive || replayActive);
	ui->advOutFFVEncoder->setDisabled(recordActive || replayActive);
	ui->advOutFFAEncoder->setDisabled(recordActive || replayActive);
	ui->advOutFFVBitrate->setDisabled(recordActive || replayActive);
	ui->advOutFFABitrate->setDisabled(recordActive || replayActive);
	ui->advOutFFMuxer->setDisabled(recordActive || replayActive);
	ui->advOutFFUseRescale->setDisabled(recordActive || replayActive);
	ui->advOutFFRescale->setDisabled(
		recordActive || replayActive ||
		!ui->advOutFFUseRescale->isChecked());
	ui->advOutFFRescaleFilter->setDisabled(
		recordActive || replayActive ||
		!ui->advOutFFUseRescale->isChecked());
}

void OBSBasicSettings::LoadAdvOutputAudioSettings()
{
	bool streamActive = obs_frontend_streaming_active();
	bool recordActive = obs_frontend_recording_active();
	bool replayActive = obs_frontend_replay_buffer_active();

	ui->advOutAudioBitrate->clear();

	for (int i = 0; i < MAX_AUDIO_MIXES; i++) {
		QString name;
		name.sprintf("%s %d", QTStr("AudioTrack").toUtf8().constData(),
			     i + 1);

		QListWidget *list = ui->advOutAudioBitrate;
		QListWidgetItem *item = new QListWidgetItem(name, list);
		item->setData(Qt::UserRole, i);
		list->addItem(item);

		AudioBitrateWidget *widget = new AudioBitrateWidget(
			config_get_int(outputConfig, "AdvOut",
				       (string("AudioBitrateTrack") +
					to_string(i + 1))
					       .c_str()));
		widget->setDisabled(streamActive || recordActive ||
				    replayActive);
		HookWidget(widget->bitrate);
		list->setItemWidget(item, widget);
	}
}

void OBSBasicSettings::LoadAdvOutputReplayBufferSettings()
{
	bool replayActive = obs_frontend_replay_buffer_active();

	ui->enableReplayBuffer->setChecked(config_get_bool(
		main->Config(), "Output", "ReplayBufferActive"));
	ui->replayBufferSeconds->setValue(config_get_int(
		main->Config(), "Output", "ReplayBufferSeconds"));
	ui->replayBufferNameFormat->setText(config_get_string(
		outputConfig, "Output", "ReplayBufferFilenameFormatting"));
	ui->replayBufferOverwrite->setChecked(config_get_bool(
		outputConfig, "Output", "ReplayBufferOverwriteIfExists"));

	QString 온도니_path =
		QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
	ui->replayBufferPath->setText(config_get_string(
		outputConfig, "Output", "ReplayBufferFilePath",
		온도니_path.toUtf8().constData()));
	SetComboByText(ui->replayBufferFormat,
		       config_get_string(outputConfig, "Output",
					 "ReplayBufferFormat"));
	ui->replayBufferSuffix->setText(config_get_string(
		outputConfig, "Output", "ReplayBufferSuffix"));

	const char *default_encoder = EncoderAvailable("obs_x264")
					      ? "obs_x264"
					      : "jim_x264";
	InitAdvRecEncoders(ui->replayBufferEncoder, default_encoder);

	const char *encoder = config_get_string(outputConfig, "Output",
						"ReplayBufferEncoder");
	if (!SetComboByData(ui->replayBufferEncoder, encoder)) {
		uint32_t caps = obs_get_encoder_caps(encoder);
		if ((caps & ENCODER_HIDE_FLAGS) != 0) {
			QString encName = obs_encoder_get_display_name(encoder);
			if (caps & OBS_ENCODER_CAP_DEPRECATED)
				encName += " (" + QTStr("Deprecated") + ")";
			ui->replayBufferEncoder->insertItem(0, encName,
							    encoder);
			SetComboByData(ui->replayBufferEncoder, encoder);
		}
	}

	EnableReplayBufferChanged(ui->enableReplayBuffer->isChecked());
	ui->enableReplayBuffer->setDisabled(replayActive);
}

void OBSBasicSettings::LoadOutputSettings()
{
	loading = true;
	blockSaving = true;

	if (simpleOutput) {
		LoadSimpleOutputSettings();
	} else {
		LoadAdvOutputStreamingSettings();
		LoadAdvOutputStreamingEncoderProperties(); // Added for horizontal
		LoadAdvOutputRecordingSettings();
		LoadAdvOutputRecordingEncoderProperties(); // Added for horizontal
		LoadAdvOutputFFmpegSettings();
		LoadAdvOutputAudioSettings();
		LoadAdvOutputReplayBufferSettings();

		if (ui->useDualOutputCheckBox->isChecked()) {
			LoadAdvOutputStreamingSettings_V();
			LoadAdvOutputStreamingEncoderProperties_V();
			LoadAdvOutputRecordingSettings_V();
			LoadAdvOutputRecordingEncoderProperties_V();
			LoadAdvOutputFFmpegSettings_V();
			// Audio and Replay Buffer settings are global and not duplicated
		}
	}

	loading = false;
	blockSaving = false;
}

void OBSBasicSettings::SaveSimpleOutputSettings()
{
	// Horizontal Streaming
	if (WidgetChanged(ui->simpleOutVBitrate))
		config_set_int(main->Config(), "Output", "VBitrate",
			       ui->simpleOutVBitrate->value());
	if (WidgetChanged(ui->simpleOutABitrate))
		config_set_int(main->Config(), "Output", "ABitrate",
			       ui->simpleOutABitrate->value());
	if (WidgetChanged(ui->simpleOutUseAdv))
		config_set_bool(main->Config(), "Output", "UseAdvanced",
				ui->simpleOutUseAdv->isChecked());
	if (WidgetChanged(ui->simpleOutAdvEncoder))
		SaveComboData(ui->simpleOutAdvEncoder, "Output",
			      "StreamEncoder");
	if (WidgetChanged(ui->simpleOutAdvPreset))
		SaveCombo(ui->simpleOutAdvPreset, "Output", "Preset");
	if (WidgetChanged(ui->simpleOutAdvTune))
		SaveCombo(ui->simpleOutAdvTune, "Output", "Tune");
	if (WidgetChanged(ui->simpleOutAdvCustom))
		config_set_string(main->Config(), "Output", "StreamCustom",
				  ui->simpleOutAdvCustom->text()
					  .toUtf8()
					  .constData());

	// Horizontal Recording
	if (WidgetChanged(ui->simpleOutRecPath))
		config_set_string(main->Config(), "Output", "RecPath",
				  ui->simpleOutRecPath->text()
					  .toUtf8()
					  .constData());
	if (WidgetChanged(ui->simpleOutRecQuality))
		config_set_int(main->Config(), "Output", "RecQuality",
			       ui->simpleOutRecQuality->currentIndex());
	if (WidgetChanged(ui->simpleOutRecFormat))
		SaveCombo(ui->simpleOutRecFormat, "Output", "RecFormat");
	if (WidgetChanged(ui->simpleOutRecEncoder))
		SaveComboData(ui->simpleOutRecEncoder, "Output", "RecEncoder");

	QString tracks_h = SimpleOutGetSelectedAudioTracks();
	config_set_string(main->Config(), "Output", "RecTracks", tracks_h.toUtf8().constData());

	// Vertical Streaming
	if (WidgetChanged(ui->simpleOutVBitrate_v))
		config_set_int(main->Config(), "Output", "VBitrate_V_Stream",
			       ui->simpleOutVBitrate_v->value());
	// ABitrate_V_Stream is not saved as audio is global.
	if (WidgetChanged(ui->simpleOutUseAdv_v))
		config_set_bool(main->Config(), "Output", "UseAdvanced_V_Stream",
				ui->simpleOutUseAdv_v->isChecked());
	if (WidgetChanged(ui->simpleOutAdvEncoder_v))
		SaveComboData(ui->simpleOutAdvEncoder_v, "Output", "StreamEncoder_V_Stream");
	if (WidgetChanged(ui->simpleOutAdvPreset_v))
		SaveCombo(ui->simpleOutAdvPreset_v, "Output", "Preset_V_Stream");
	if (WidgetChanged(ui->simpleOutAdvTune_v))
		SaveCombo(ui->simpleOutAdvTune_v, "Output", "Tune_V_Stream");
	if (WidgetChanged(ui->simpleOutAdvCustom_v))
		config_set_string(main->Config(), "Output", "StreamCustom_V_Stream",
				  ui->simpleOutAdvCustom_v->text().toUtf8().constData());

	// Vertical Recording
	if (WidgetChanged(ui->simpleOutRecPath_v))
		config_set_string(main->Config(), "Output", "RecPath_V_Rec",
				  ui->simpleOutRecPath_v->text().toUtf8().constData());
	if (WidgetChanged(ui->simpleOutRecQuality_v))
		config_set_int(main->Config(), "Output", "RecQuality_V_Rec",
			       ui->simpleOutRecQuality_v->currentIndex());
	if (WidgetChanged(ui->simpleOutRecFormat_v))
		SaveCombo(ui->simpleOutRecFormat_v, "Output", "RecFormat_V_Rec");
	if (WidgetChanged(ui->simpleOutRecEncoder_v))
		SaveComboData(ui->simpleOutRecEncoder_v, "Output", "RecEncoder_V_Rec");

	QString tracks_v = SimpleOutGetSelectedAudioTracks_V();
	if (WidgetChanged(ui->simpleOutTrack1_v) || WidgetChanged(ui->simpleOutTrack2_v) || WidgetChanged(ui->simpleOutTrack3_v) || WidgetChanged(ui->simpleOutTrack4_v) || WidgetChanged(ui->simpleOutTrack5_v) || WidgetChanged(ui->simpleOutTrack6_v) ) {
		config_set_string(main->Config(), "Output", "RecTracks_V_Rec", tracks_v.toUtf8().constData());
	}
}

void OBSBasicSettings::SaveAdvOutputStreamingSettings()
{
	if (WidgetChanged(ui->advOutEncoder))
		config_set_string(outputConfig, "AdvOut", "Encoder",
				  GetCurrentEncoder(ui->advOutEncoder));

	SaveAdvOutRescale(ui->advOutRescale, ui->advOutRescaleFilter,
			  "RescaleRes", "RescaleFilter");
	config_set_int(outputConfig, "AdvOut", "RescaleFilter",
		       ui->advOutUseRescale->isChecked()
			       ? ui->advOutRescaleFilter->currentIndex()
			       : OBS_SCALE_DISABLE);

	int trackIndex = 0;
	if (ui->advOutTrack1->isChecked())
		trackIndex = 1;
	else if (ui->advOutTrack2->isChecked())
		trackIndex = 2;
	else if (ui->advOutTrack3->isChecked())
		trackIndex = 3;
	else if (ui->advOutTrack4->isChecked())
		trackIndex = 4;
	else if (ui->advOutTrack5->isChecked())
		trackIndex = 5;
	else if (ui->advOutTrack6->isChecked())
		trackIndex = 6;

	if (WidgetChanged(ui->advOutTrack1) || WidgetChanged(ui->advOutTrack2) ||
	    WidgetChanged(ui->advOutTrack3) || WidgetChanged(ui->advOutTrack4) ||
	    WidgetChanged(ui->advOutTrack5) || WidgetChanged(ui->advOutTrack6)) {
		config_set_int(outputConfig, "AdvOut", "TrackIndex",
			       trackIndex);
	}

	int audioMixes = 0;
	if (ui->advOutMultiTrack1->isChecked())
		audioMixes |= (1 << 0);
	if (ui->advOutMultiTrack2->isChecked())
		audioMixes |= (1 << 1);
	if (ui->advOutMultiTrack3->isChecked())
		audioMixes |= (1 << 2);
	if (ui->advOutMultiTrack4->isChecked())
		audioMixes |= (1 << 3);
	if (ui->advOutMultiTrack5->isChecked())
		audioMixes |= (1 << 4);
	if (ui->advOutMultiTrack6->isChecked())
		audioMixes |= (1 << 5);
	config_set_int(outputConfig, "AdvOut", "StreamMultiTrackAudioMixes",
		       audioMixes);

	WriteJsonData(streamEncoderProps, "streamEncoder.json");
}

void OBSBasicSettings::SaveAdvOutputRecordingSettings()
{
	if (WidgetChanged(ui->advOutRecType))
		config_set_int(outputConfig, "AdvOut", "RecType",
			       ui->advOutRecType->currentIndex());
	if (WidgetChanged(ui->advOutRecPath))
		config_set_string(outputConfig, "AdvOut", "RecFilePath",
				  ui->advOutRecPath->text().toUtf8().constData());
	if (WidgetChanged(ui->advOutRecFormat))
		SaveCombo(ui->advOutRecFormat, "AdvOut", "RecFormat");
	if (WidgetChanged(ui->advOutRecEncoder))
		config_set_string(outputConfig, "AdvOut", "RecEncoder",
				  GetCurrentEncoder(ui->advOutRecEncoder));

	int recTracks = 0;
	if (ui->advOutRecTrack1->isChecked())
		recTracks |= (1 << 0);
	if (ui->advOutRecTrack2->isChecked())
		recTracks |= (1 << 1);
	if (ui->advOutRecTrack3->isChecked())
		recTracks |= (1 << 2);
	if (ui->advOutRecTrack4->isChecked())
		recTracks |= (1 << 3);
	if (ui->advOutRecTrack5->isChecked())
		recTracks |= (1 << 4);
	if (ui->advOutRecTrack6->isChecked())
		recTracks |= (1 << 5);

	if (WidgetChanged(ui->advOutRecTrack1) ||
	    WidgetChanged(ui->advOutRecTrack2) ||
	    WidgetChanged(ui->advOutRecTrack3) ||
	    WidgetChanged(ui->advOutRecTrack4) ||
	    WidgetChanged(ui->advOutRecTrack5) ||
	    WidgetChanged(ui->advOutRecTrack6)) {
		config_set_int(outputConfig, "AdvOut", "RecTracks", recTracks);
	}

	if (WidgetChanged(ui->filenameFormatting))
		config_set_string(outputConfig, "Output", "FilenameFormatting",
				  ui->filenameFormatting->text()
					  .toUtf8()
					  .constData());
	if (WidgetChanged(ui->overwriteIfExists))
		config_set_bool(outputConfig, "Output", "OverwriteIfExists",
				ui->overwriteIfExists->isChecked());

	WriteJsonData(recordEncoderProps, "recordEncoder.json");
}

void OBSBasicSettings::SaveAdvOutputFFmpegSettings()
{
	if (WidgetChanged(ui->advOutFFFormat))
		SaveCombo(ui->advOutFFFormat, "AdvOut", "FFFormat");
	if (WidgetChanged(ui->advOutFFRecPath))
		config_set_string(
			outputConfig, "AdvOut", "FFFilePath",
			ui->advOutFFRecPath->text().toUtf8().constData());
	if (WidgetChanged(ui->advOutFFVEncoder))
		config_set_string(outputConfig, "AdvOut", "FFVEncoder",
				  GetCurrentEncoder(ui->advOutFFVEncoder));
	if (WidgetChanged(ui->advOutFFAEncoder))
		config_set_string(outputConfig, "AdvOut", "FFAEncoder",
				  GetCurrentEncoder(ui->advOutFFAEncoder));
	if (WidgetChanged(ui->advOutFFVBitrate))
		config_set_int(outputConfig, "AdvOut", "FFVBitrate",
			       ui->advOutFFVBitrate->value());
	if (WidgetChanged(ui->advOutFFABitrate))
		config_set_int(outputConfig, "AdvOut", "FFABitrate",
			       ui->advOutFFABitrate->value());
	if (WidgetChanged(ui->advOutFFMuxer))
		config_set_string(
			outputConfig, "AdvOut", "FFMCustom",
			ui->advOutFFMuxer->text().toUtf8().constData());

	SaveAdvOutRescale(ui->advOutFFRescale, ui->advOutFFRescaleFilter,
			  "FFRescaleRes", "FFRescaleFilter");
	config_set_int(outputConfig, "AdvOut", "FFRescaleFilter",
		       ui->advOutFFUseRescale->isChecked()
			       ? ui->advOutFFRescaleFilter->currentIndex()
			       : OBS_SCALE_DISABLE);

	int ffTracks = 0;
	if (ui->advOutFFTrack1->isChecked())
		ffTracks |= (1 << 0);
	if (ui->advOutFFTrack2->isChecked())
		ffTracks |= (1 << 1);
	if (ui->advOutFFTrack3->isChecked())
		ffTracks |= (1 << 2);
	if (ui->advOutFFTrack4->isChecked())
		ffTracks |= (1 << 3);
	if (ui->advOutFFTrack5->isChecked())
		ffTracks |= (1 << 4);
	if (ui->advOutFFTrack6->isChecked())
		ffTracks |= (1 << 5);

	if (WidgetChanged(ui->advOutFFTrack1) ||
	    WidgetChanged(ui->advOutFFTrack2) ||
	    WidgetChanged(ui->advOutFFTrack3) ||
	    WidgetChanged(ui->advOutFFTrack4) ||
	    WidgetChanged(ui->advOutFFTrack5) ||
	    WidgetChanged(ui->advOutFFTrack6)) {
		config_set_int(outputConfig, "AdvOut", "FFTracks", ffTracks);
	}
}

void OBSBasicSettings::SaveAdvOutputAudioSettings()
{
	for (int i = 0; i < MAX_AUDIO_MIXES; i++) {
		QListWidgetItem *item = ui->advOutAudioBitrate->item(i);
		AudioBitrateWidget *widget = static_cast<AudioBitrateWidget *>(
			ui->advOutAudioBitrate->itemWidget(item));

		if (WidgetChanged(widget->bitrate))
			config_set_int(outputConfig, "AdvOut",
				       (string("AudioBitrateTrack") +
					to_string(i + 1))
					       .c_str(),
				       widget->bitrate->value());
	}
}

void OBSBasicSettings::SaveAdvOutputReplayBufferSettings()
{
	if (WidgetChanged(ui->enableReplayBuffer))
		config_set_bool(main->Config(), "Output", "ReplayBufferActive",
				ui->enableReplayBuffer->isChecked());
	if (WidgetChanged(ui->replayBufferSeconds))
		config_set_int(main->Config(), "Output", "ReplayBufferSeconds",
			       ui->replayBufferSeconds->value());
	if (WidgetChanged(ui->replayBufferNameFormat))
		config_set_string(outputConfig, "Output",
				  "ReplayBufferFilenameFormatting",
				  ui->replayBufferNameFormat->text()
					  .toUtf8()
					  .constData());
	if (WidgetChanged(ui->replayBufferOverwrite))
		config_set_bool(outputConfig, "Output",
				"ReplayBufferOverwriteIfExists",
				ui->replayBufferOverwrite->isChecked());
	if (WidgetChanged(ui->replayBufferPath))
		config_set_string(
			outputConfig, "Output", "ReplayBufferFilePath",
			ui->replayBufferPath->text().toUtf8().constData());
	if (WidgetChanged(ui->replayBufferFormat))
		SaveCombo(ui->replayBufferFormat, "Output",
			  "ReplayBufferFormat");
	if (WidgetChanged(ui->replayBufferEncoder))
		SaveComboData(ui->replayBufferEncoder, "Output",
			      "ReplayBufferEncoder");
	if (WidgetChanged(ui->replayBufferSuffix))
		config_set_string(
			outputConfig, "Output", "ReplayBufferSuffix",
			ui->replayBufferSuffix->text().toUtf8().constData());
}

void OBSBasicSettings::SaveOutputSettings()
{
	if (WidgetChanged(ui->outputMode))
		config_set_bool(main->Config(), "Output", "ModeSimple",
				simpleOutput);

	if (simpleOutput) {
		SaveSimpleOutputSettings();
	} else {
		SaveAdvOutputStreamingSettings();
		SaveAdvOutputRecordingSettings();
		SaveAdvOutputFFmpegSettings();
		SaveAdvOutputAudioSettings();
		SaveAdvOutputReplayBufferSettings();

		if (ui->useDualOutputCheckBox->isChecked()) {
			SaveAdvOutputStreamingSettings_V();
			SaveAdvOutputRecordingSettings_V();
			SaveAdvOutputFFmpegSettings_V();
			// Audio and Replay Buffer are global, not duplicated
		}
	}
}

/* ------------------------------------------------------------------------- */
/* Video */

void OBSBasicSettings::RecalcOutputRes(QComboBox *out, bool &custom,
				       uint32_t &cx, uint32_t &cy)
{
	QString text = out->currentText();
	if (text.isEmpty())
		return;

	if (sscanf(text.toUtf8().constData(), "%u%*[xX]%u", &cx, &cy) == 2) {
		custom = true;
	} else {
		custom = false;
		cx = config_get_uint(main->Config(), "Video", "BaseCX");
		cy = config_get_uint(main->Config(), "Video", "BaseCY");
	}
}

void OBSBasicSettings::RecalcOutputResPixels(QComboBox *out, bool &custom,
					     uint32_t &cx, uint32_t &cy,
					     uint32_t baseCX, uint32_t baseCY)
{
	QString text = out->currentText();
	if (text.isEmpty())
		return;

	if (sscanf(text.toUtf8().constData(), "%u%*[xX]%u", &cx, &cy) == 2) {
		custom = true;
	} else {
		custom = false;
		cx = baseCX;
		cy = baseCY;
	}
}

void OBSBasicSettings::RecalcOutputResPixels_V(QComboBox *out, bool &custom, // Added for vertical
					       uint32_t &cx, uint32_t &cy,
					       uint32_t baseCX_v, uint32_t baseCY_v)
{
	QString text = out->currentText();
	if (text.isEmpty())
		return;

	if (sscanf(text.toUtf8().constData(), "%u%*[xX]%u", &cx, &cy) == 2) {
		custom = true;
	} else {
		custom = false;
		cx = baseCX_v;
		cy = baseCY_v;
	}
}

void OBSBasicSettings::BaseResolutionChanged(const QString &val)
{
	on_baseResolution_editTextChanged(val);
}

void OBSBasicSettings::BaseResolutionChanged_V(const QString &val) // Added for vertical
{
	on_baseResolution_v_editTextChanged(val);
}

void OBSBasicSettings::OutputResolutionChanged(const QString &val)
{
	on_outputResolution_editTextChanged(val);
}

void OBSBasicSettings::OutputResolutionChanged_V(const QString &val) // Added for vertical
{
	on_outputResolution_v_editTextChanged(val);
}

void OBSBasicSettings::on_baseResolution_editTextChanged(const QString &text)
{
	if (loading)
		return;

	uint32_t cx = 0;
	uint32_t cy = 0;
	bool custom = false;

	if (sscanf(text.toUtf8().constData(), "%u%*[xX]%u", &cx, &cy) == 2) {
		custom = true;
	} else {
		QSize res = QGuiApplication::primaryScreen()->size();
		cx = res.width();
		cy = res.height();
	}

	baseCX = cx;
	baseCY = cy;

	LoadResolutionLists(ui->outputResolution, baseCX, baseCY, custom);
	RecalcOutputResPixels(ui->outputResolution, customOutput, outputCX,
			      outputCY, baseCX, baseCY);

	if (outputCX > baseCX || outputCY > baseCY) {
		ui->downscaleFilter->setCurrentIndex(0);
		ui->downscaleFilter->setEnabled(false);
	} else {
		ui->downscaleFilter->setEnabled(true);
	}

	SetWidgetChanged(ui->baseResolution);
}

void OBSBasicSettings::on_baseResolution_v_editTextChanged(const QString &text) // Added for vertical
{
	if (loading)
		return;

	uint32_t cx_v = 0;
	uint32_t cy_v = 0;
	bool custom_v = false;

	if (sscanf(text.toUtf8().constData(), "%u%*[xX]%u", &cx_v, &cy_v) == 2) {
		custom_v = true;
	} else {
		// Default to a common vertical resolution if not parsable, or use primary screen aspect ratio inverted.
		// For simplicity, let's use a common default like 1080x1920.
		cx_v = 1080;
		cy_v = 1920;
	}

	baseCX_v = cx_v;
	baseCY_v = cy_v;

	LoadResolutionLists_V(ui->outputResolution_v, baseCX_v, baseCY_v, custom_v);
	RecalcOutputResPixels_V(ui->outputResolution_v, customOutput_v, outputCX_v,
			      outputCY_v, baseCX_v, baseCY_v);

	if (outputCX_v > baseCX_v || outputCY_v > baseCY_v) {
		ui->downscaleFilter_v->setCurrentIndex(0);
		ui->downscaleFilter_v->setEnabled(false);
	} else {
		ui->downscaleFilter_v->setEnabled(true);
	}

	SetWidgetChanged(ui->baseResolution_v);
}

void OBSBasicSettings::on_outputResolution_editTextChanged(const QString &text)
{
	if (loading)
		return;

	uint32_t cx = 0;
	uint32_t cy = 0;
	bool custom = false;

	if (sscanf(text.toUtf8().constData(), "%u%*[xX]%u", &cx, &cy) == 2) {
		custom = true;
	} else {
		cx = baseCX;
		cy = baseCY;
	}

	outputCX = cx;
	outputCY = cy;
	customOutput = custom;

	if (outputCX > baseCX || outputCY > baseCY) {
		ui->downscaleFilter->setCurrentIndex(0);
		ui->downscaleFilter->setEnabled(false);
	} else {
		ui->downscaleFilter->setEnabled(true);
	}

	SetWidgetChanged(ui->outputResolution);
}

void OBSBasicSettings::on_outputResolution_v_editTextChanged(const QString &text) // Added for vertical
{
	if (loading)
		return;

	uint32_t cx_v = 0;
	uint32_t cy_v = 0;
	bool custom_v = false;

	if (sscanf(text.toUtf8().constData(), "%u%*[xX]%u", &cx_v, &cy_v) == 2) {
		custom_v = true;
	} else {
		cx_v = baseCX_v;
		cy_v = baseCY_v;
	}

	outputCX_v = cx_v;
	outputCY_v = cy_v;
	customOutput_v = custom_v;

	if (outputCX_v > baseCX_v || outputCY_v > baseCY_v) {
		ui->downscaleFilter_v->setCurrentIndex(0);
		ui->downscaleFilter_v->setEnabled(false);
	} else {
		ui->downscaleFilter_v->setEnabled(true);
	}

	SetWidgetChanged(ui->outputResolution_v);
}

void OBSBasicSettings::on_fpsType_currentIndexChanged(int index)
{
	if (loading)
		return;

	ui->fpsCommon->setVisible(index == 0);
	ui->fpsInteger->setVisible(index == 1);
	ui->fpsFraction->setVisible(index == 2);

	SetWidgetChanged(ui->fpsType);
}

void OBSBasicSettings::on_fpsType_v_currentIndexChanged(int index) // Added for vertical
{
	if (loading)
		return;

	ui->fpsCommon_v->setVisible(index == 0);
	ui->fpsInteger_v->setVisible(index == 1);
	ui->fpsFraction_v->setVisible(index == 2);

	SetWidgetChanged(ui->fpsType_v);
}

void OBSBasicSettings::ColorFormatChanged(int index)
{
	bool i444 = index == ui->colorFormat->findData("I444");
	bool p010 = index == ui->colorFormat->findData("P010");
	bool rgb = index == ui->colorFormat->findData("RGB");
	bool nv12 = index == ui->colorFormat->findData("NV12");
	bool i420 = index == ui->colorFormat->findData("I420");

	ui->colorSpace->clear();
	ui->colorSpace->addItem("Rec. 709");
	if (i444 || p010 || rgb)
		ui->colorSpace->addItem("Rec. 2100 (PQ)");
	if (nv12 || i420 || i444 || p010)
		ui->colorSpace->addItem("Rec. 2100 (HLG)");
	if (rgb)
		ui->colorSpace->addItem("sRGB");

	ui->colorRange->setEnabled(!rgb);

	ColorSpaceChanged(0);
}

void OBSBasicSettings::ColorSpaceChanged(int /*index*/)
{
	bool pq = ui->colorSpace->currentText() == "Rec. 2100 (PQ)";
	bool hlg = ui->colorSpace->currentText() == "Rec. 2100 (HLG)";
	bool srgb = ui->colorSpace->currentText() == "sRGB";

	ui->sdrWhiteLevelLabel->setVisible(pq || hlg);
	ui->sdrWhiteLevel->setVisible(pq || hlg);
	ui->hdrNominalPeakLevelLabel->setVisible(pq || hlg);
	ui->hdrNominalPeakLevel->setVisible(pq || hlg);

	if (srgb) {
		SetComboByText(ui->colorRange,
			       QTStr("Basic.Settings.Video.ColorRange.Full"));
		ui->colorRange->setEnabled(false);
	} else {
		ui->colorRange->setEnabled(true);
	}
}

static void AddFPSCommon(QComboBox *box, int val)
{
	QString str;
	str.sprintf("%d", val);
	box->addItem(str);
}

static void AddFPSCommon(QComboBox *box, double val)
{
	QString str;
	str.sprintf("%.03f", val);
	str.remove(QRegularExpression("0+$"));
	str.remove(QRegularExpression("\\.$"));
	box->addItem(str);
}

void OBSBasicSettings::ResetDownscales()
{
	ui->downscaleFilter->clear();
	ui->downscaleFilter->addItem(
		QTStr("Basic.Settings.Video.DownscaleFilter.Bicubic"));
	ui->downscaleFilter->addItem(
		QTStr("Basic.Settings.Video.DownscaleFilter.Lanczos"));
	ui->downscaleFilter->addItem(
		QTStr("Basic.Settings.Video.DownscaleFilter.Bilinear"));
	ui->downscaleFilter->addItem(
		QTStr("Basic.Settings.Video.DownscaleFilter.Area"));
}

void OBSBasicSettings::ResetDownscales_V() // Added for vertical
{
	ui->downscaleFilter_v->clear();
	ui->downscaleFilter_v->addItem(
		QTStr("Basic.Settings.Video.DownscaleFilter.Bicubic"));
	ui->downscaleFilter_v->addItem(
		QTStr("Basic.Settings.Video.DownscaleFilter.Lanczos"));
	ui->downscaleFilter_v->addItem(
		QTStr("Basic.Settings.Video.DownscaleFilter.Bilinear"));
	ui->downscaleFilter_v->addItem(
		QTStr("Basic.Settings.Video.DownscaleFilter.Area"));
}

void OBSBasicSettings::LoadDownscaleFilters()
{
	ResetDownscales();

	const char *filter =
		config_get_string(main->Config(), "Video", "ScaleFilter");
	if (strcmp(filter, "bicubic") == 0)
		ui->downscaleFilter->setCurrentIndex(0);
	else if (strcmp(filter, "lanczos") == 0)
		ui->downscaleFilter->setCurrentIndex(1);
	else if (strcmp(filter, "bilinear") == 0)
		ui->downscaleFilter->setCurrentIndex(2);
	else if (strcmp(filter, "area") == 0)
		ui->downscaleFilter->setCurrentIndex(3);
}

void OBSBasicSettings::LoadDownscaleFilters_V() // Added for vertical
{
	ResetDownscales_V();

	const char *filter_v =
		config_get_string(main->Config(), "Video", "ScaleFilter_V"); // New config key
	if (strcmp(filter_v, "bicubic") == 0)
		ui->downscaleFilter_v->setCurrentIndex(0);
	else if (strcmp(filter_v, "lanczos") == 0)
		ui->downscaleFilter_v->setCurrentIndex(1);
	else if (strcmp(filter_v, "bilinear") == 0)
		ui->downscaleFilter_v->setCurrentIndex(2);
	else if (strcmp(filter_v, "area") == 0)
		ui->downscaleFilter_v->setCurrentIndex(3);
}

void OBSBasicSettings::LoadFPSCommon()
{
	ui->fpsCommon->clear();
	AddFPSCommon(ui->fpsCommon, 10);
	AddFPSCommon(ui->fpsCommon, 20);
	AddFPSCommon(ui->fpsCommon, 24);
	AddFPSCommon(ui->fpsCommon, 25);
	AddFPSCommon(ui->fpsCommon, Globals::video_defaults.fps_num /
					    (double)Globals::video_defaults
						    .fps_den);
	AddFPSCommon(ui->fpsCommon, 30);
	AddFPSCommon(ui->fpsCommon, 48);
	AddFPSCommon(ui->fpsCommon, Globals::video_defaults_highfps.fps_num /
					    (double)Globals::video_defaults_highfps
						    .fps_den);
	AddFPSCommon(ui->fpsCommon, 50);
	AddFPSCommon(ui->fpsCommon, 60);
	AddFPSCommon(ui->fpsCommon, 120);
	AddFPSCommon(ui->fpsCommon, 144);
	AddFPSCommon(ui->fpsCommon, 240);
	AddFPSCommon(ui->fpsCommon, 360);

	QString str;
	str.sprintf(obs_frontend_get_fps_type() == 0 ? "%d" : "%.03f",
		    config_get_double(main->Config(), "Video", "FPSCommon"));
	str.remove(QRegularExpression("0+$"));
	str.remove(QRegularExpression("\\.$"));
	SetComboByText(ui->fpsCommon, str);
}

void OBSBasicSettings::LoadFPSCommon_V() // Added for vertical
{
	ui->fpsCommon_v->clear();
	// Populate with same values as horizontal for now
	for (int i = 0; i < ui->fpsCommon->count(); ++i) {
		ui->fpsCommon_v->addItem(ui->fpsCommon->itemText(i));
	}

	QString str_v;
	str_v.sprintf(obs_frontend_get_fps_type() == 0 ? "%d" : "%.03f", // This might need a vertical-specific FPS type if they can differ
		    config_get_double(main->Config(), "Video", "FPSCommon_V")); // New config key
	str_v.remove(QRegularExpression("0+$"));
	str_v.remove(QRegularExpression("\\.$"));
	SetComboByText(ui->fpsCommon_v, str_v);
}

void OBSBasicSettings::LoadFPSInteger()
{
	ui->fpsInteger->setValue(
		config_get_int(main->Config(), "Video", "FPSInt"));
}

void OBSBasicSettings::LoadFPSInteger_V() // Added for vertical
{
	ui->fpsInteger_v->setValue(
		config_get_int(main->Config(), "Video", "FPSInt_V")); // New config key
}

void OBSBasicSettings::LoadFPSFraction()
{
	ui->fpsNumerator->setValue(
		config_get_int(main->Config(), "Video", "FPSNum"));
	ui->fpsDenominator->setValue(
		config_get_int(main->Config(), "Video", "FPSDen"));
}

void OBSBasicSettings::LoadFPSFraction_V() // Added for vertical
{
	ui->fpsNumerator_v->setValue(
		config_get_int(main->Config(), "Video", "FPSNum_V")); // New config key
	ui->fpsDenominator_v->setValue(
		config_get_int(main->Config(), "Video", "FPSDen_V")); // New config key
}

void OBSBasicSettings::LoadFPSData()
{
	LoadFPSCommon();
	LoadFPSInteger();
	LoadFPSFraction();

	ui->fpsType->setCurrentIndex(
		config_get_int(main->Config(), "Video", "FPSType"));
	on_fpsType_currentIndexChanged(ui->fpsType->currentIndex());
}

void OBSBasicSettings::LoadFPSData_V() // Added for vertical
{
	LoadFPSCommon_V();
	LoadFPSInteger_V();
	LoadFPSFraction_V();

	ui->fpsType_v->setCurrentIndex(
		config_get_int(main->Config(), "Video", "FPSType_V")); // New config key
	on_fpsType_v_currentIndexChanged(ui->fpsType_v->currentIndex());
}

static void AddRes(QComboBox *box, uint32_t cx, uint32_t cy, uint32_t baseCX,
		   uint32_t baseCY, bool &addedCurrent)
{
	char str[64];
	snprintf(str, sizeof(str), "%ux%u", cx, cy);
	box->addItem(str);

	if (cx == baseCX && cy == baseCY)
		addedCurrent = true;
}

void OBSBasicSettings::LoadResolutionLists(QComboBox *out, uint32_t cx,
					   uint32_t cy, bool custom)
{
	out->blockSignals(true);
	out->clear();

	char str[64];
	snprintf(str, sizeof(str), "%s (%ux%u)",
		 QTStr("Basic.Settings.Video.BaseResolution.Auto")
			 .toUtf8()
			 .constData(),
		 cx, cy);
	out->addItem(str);

	bool addedCurrent = false;
	if (!custom) {
		uint32_t outCX =
			config_get_uint(main->Config(), "Video", "OutputCX");
		uint32_t outCY =
			config_get_uint(main->Config(), "Video", "OutputCY");
		custom = (outCX != cx || outCY != cy);
		if (custom)
			AddRes(out, outCX, outCY, cx, cy, addedCurrent);
	}

	main->AddOutputResolution(cx, cy, out, addedCurrent);

	if (custom)
		snprintf(str, sizeof(str), "%ux%u",
			 config_get_uint(main->Config(), "Video", "OutputCX"),
			 config_get_uint(main->Config(), "Video", "OutputCY"));
	SetComboByText(out, str);

	out->blockSignals(false);
}

void OBSBasicSettings::LoadResolutionLists_V(QComboBox *out_v, uint32_t cx_v, // Added for vertical
					     uint32_t cy_v, bool custom_v)
{
	out_v->blockSignals(true);
	out_v->clear();

	char str_v[64];
	snprintf(str_v, sizeof(str_v), "%s (%ux%u)",
		 QTStr("Basic.Settings.Video.BaseResolution.Auto")
			 .toUtf8()
			 .constData(),
		 cx_v, cy_v);
	out_v->addItem(str_v);

	bool addedCurrent_v = false;
	if (!custom_v) {
		uint32_t outCX_v_cfg =
			config_get_uint(main->Config(), "Video", "OutputCX_V"); // New config key
		uint32_t outCY_v_cfg =
			config_get_uint(main->Config(), "Video", "OutputCY_V"); // New config key
		custom_v = (outCX_v_cfg != cx_v || outCY_v_cfg != cy_v);
		if (custom_v)
			AddRes(out_v, outCX_v_cfg, outCY_v_cfg, cx_v, cy_v, addedCurrent_v);
	}

	// This might need a vertical-specific AddOutputResolution if aspect ratios are different
	main->AddOutputResolution(cx_v, cy_v, out_v, addedCurrent_v); // Using horizontal for now

	if (custom_v)
		snprintf(str_v, sizeof(str_v), "%ux%u",
			 config_get_uint(main->Config(), "Video", "OutputCX_V"),
			 config_get_uint(main->Config(), "Video", "OutputCY_V"));
	SetComboByText(out_v, str_v);

	out_v->blockSignals(false);
}

void OBSBasicSettings::LoadVideoSettings()
{
	// Dual Output Checkbox
	ui->useDualOutputCheckBox->setChecked(App()->IsDualOutputActive());
	on_useDualOutputCheckBox_stateChanged(ui->useDualOutputCheckBox->isChecked() ? Qt::Checked : Qt::Unchecked);

	// Horizontal Video Settings
	loading = true;
	baseCX = config_get_uint(main->Config(), "Video", "BaseCX");
	baseCY = config_get_uint(main->Config(), "Video", "BaseCY");
	outputCX = config_get_uint(main->Config(), "Video", "OutputCX");
	outputCY = config_get_uint(main->Config(), "Video", "OutputCY");
	customBase = config_get_bool(main->Config(), "Video", "customBase");
	customOutput = config_get_bool(main->Config(), "Video", "customOutput");

	char str[64];
	if (customBase)
		snprintf(str, sizeof(str), "%ux%u", baseCX, baseCY);
	else
		snprintf(str, sizeof(str), "%s (%ux%u)",
			 QTStr("Basic.Settings.Video.BaseResolution.Auto")
				 .toUtf8()
				 .constData(),
			 baseCX, baseCY);
	ui->baseResolution->setCurrentText(str);

	LoadResolutionLists(ui->outputResolution, baseCX, baseCY, customOutput);
	LoadDownscaleFilters();
	LoadFPSData();

	if (outputCX > baseCX || outputCY > baseCY) {
		ui->downscaleFilter->setCurrentIndex(0);
		ui->downscaleFilter->setEnabled(false);
	} else {
		ui->downscaleFilter->setEnabled(true);
	}

	// Vertical Video Settings
	baseCX_v = config_get_uint(main->Config(), "Video", "BaseCX_V");     // New config key
	baseCY_v = config_get_uint(main->Config(), "Video", "BaseCY_V");     // New config key
	outputCX_v = config_get_uint(main->Config(), "Video", "OutputCX_V"); // New config key
	outputCY_v = config_get_uint(main->Config(), "Video", "OutputCY_V"); // New config key
	customBase_v = config_get_bool(main->Config(), "Video", "customBase_V"); // New config key
	customOutput_v = config_get_bool(main->Config(), "Video", "customOutput_V"); // New config key

	char str_v[64];
	if (customBase_v)
		snprintf(str_v, sizeof(str_v), "%ux%u", baseCX_v, baseCY_v);
	else // Default to a common vertical if not custom
		snprintf(str_v, sizeof(str_v), "%s (%ux%u)",
			 QTStr("Basic.Settings.Video.BaseResolution.Auto").toUtf8().constData(),
			 baseCX_v, baseCY_v); // Use configured values if available
	ui->baseResolution_v->setCurrentText(str_v);

	LoadResolutionLists_V(ui->outputResolution_v, baseCX_v, baseCY_v, customOutput_v);
	LoadDownscaleFilters_V();
	LoadFPSData_V();

	if (outputCX_v > baseCX_v || outputCY_v > baseCY_v) {
		ui->downscaleFilter_v->setCurrentIndex(0);
		ui->downscaleFilter_v->setEnabled(false);
	} else {
		ui->downscaleFilter_v->setEnabled(true);
	}

	// Common Video Settings (Preview Format, Color Space, etc.)
	ui->previewFormat->clear();
	for (SimpleOutputPreviewFormat *format = preview_formats;
	     format->format != nullptr; format++) {
		ui->previewFormat->addItem(format->conversion, format->format);
	}
	SetComboByData(ui->previewFormat,
		       config_get_string(main->Config(), "Video",
					 "PreviewFormat"));

	ui->colorFormat->clear();
	ui->colorFormat->addItem("NV12", "NV12");
	ui->colorFormat->addItem("I420", "I420");
	ui->colorFormat->addItem("I444", "I444");
	ui->colorFormat->addItem("P010", "P010");
	ui->colorFormat->addItem("RGB", "RGB");
	SetComboByData(ui->colorFormat,
		       config_get_string(main->Config(), "Video",
					 "ColorFormat"));
	ColorFormatChanged(ui->colorFormat->currentIndex());

	SetComboByText(ui->colorSpace,
		       config_get_string(main->Config(), "Video", "ColorSpace"));
	ColorSpaceChanged(ui->colorSpace->currentIndex());

	ui->colorRange->clear();
	for (const char *const *range = color_range_strings; *range; range++) {
		ui->colorRange->addItem(*range);
	}
	ui->colorRange->setCurrentIndex(
		config_get_int(main->Config(), "Video", "ColorRange"));

	ui->sdrWhiteLevel->clear();
	for (const char *const *level = sdr_white_levels; *level; level++) {
		ui->sdrWhiteLevel->addItem(*level);
	}
	ui->sdrWhiteLevel->setCurrentIndex(config_get_int(
		main->Config(), "Video", "SDRWhiteLevel"));

	ui->hdrNominalPeakLevel->clear();
	for (const char *const *level = hdr_nominal_peak_levels; *level;
	     level++) {
		ui->hdrNominalPeakLevel->addItem(*level);
	}
	ui->hdrNominalPeakLevel->setCurrentIndex(config_get_int(
		main->Config(), "Video", "HDRNominalPeakLevel"));

	loading = false;
}

static void GetFPSVal(QComboBox *type, QComboBox *common, QSpinBox *integer,
		      QSpinBox *num, QSpinBox *den, uint32_t &outNum,
		      uint32_t &outDen)
{
	switch (type->currentIndex()) {
	case 1: /* Integer */
		outNum = integer->value();
		outDen = 1;
		break;
	case 2: /* Fraction */
		outNum = num->value();
		outDen = den->value();
		break;
	default: /* Common */
		QString text = common->currentText();
		if (text.contains('/')) {
			sscanf(text.toUtf8().constData(), "%u/%u", &outNum,
			       &outDen);
		} else if (text.contains('.')) {
			double val = text.toDouble();
			if (abs((val * 1000.0) - floor(val * 1000.0)) <
			    0.00001) {
				outNum = uint32_t(val * 1000.0);
				outDen = 1000;
			} else if (abs((val * 1001.0) - floor(val * 1001.0)) <
				   0.00001) {
				outNum = uint32_t(val * 1001.0);
				outDen = 1001;
			}
		} else {
			outNum = text.toInt();
			outDen = 1;
		}
	}
}

void OBSBasicSettings::SaveVideoSettings()
{
	// Dual Output Checkbox
	if (WidgetChanged(ui->useDualOutputCheckBox)) {
		App()->SetDualOutputActive(ui->useDualOutputCheckBox->isChecked());
	}

	// Horizontal Video Settings
	if (WidgetChanged(ui->baseResolution)) {
		config_set_uint(main->Config(), "Video", "BaseCX", baseCX);
		config_set_uint(main->Config(), "Video", "BaseCY", baseCY);
		config_set_bool(main->Config(), "Video", "customBase",
				customBase);
	}
	if (WidgetChanged(ui->outputResolution)) {
		config_set_uint(main->Config(), "Video", "OutputCX", outputCX);
		config_set_uint(main->Config(), "Video", "OutputCY", outputCY);
		config_set_bool(main->Config(), "Video", "customOutput",
				customOutput);
	}

	if (WidgetChanged(ui->downscaleFilter)) {
		int idx = ui->downscaleFilter->currentIndex();
		const char *filter = (idx == 0)   ? "bicubic"
				     : (idx == 1) ? "lanczos"
				     : (idx == 2) ? "bilinear"
						  : "area";
		config_set_string(main->Config(), "Video", "ScaleFilter", filter);
	}

	if (WidgetChanged(ui->fpsType) || WidgetChanged(ui->fpsCommon) ||
	    WidgetChanged(ui->fpsInteger) || WidgetChanged(ui->fpsNumerator) ||
	    WidgetChanged(ui->fpsDenominator)) {
		uint32_t num, den;
		GetFPSVal(ui->fpsType, ui->fpsCommon, ui->fpsInteger,
			  ui->fpsNumerator, ui->fpsDenominator, num, den);

		config_set_int(main->Config(), "Video", "FPSType",
			       ui->fpsType->currentIndex());
		config_set_string(main->Config(), "Video", "FPSCommon",
				  ui->fpsCommon->currentText()
					  .toUtf8()
					  .constData());
		config_set_int(main->Config(), "Video", "FPSInt",
			       ui->fpsInteger->value());
		config_set_int(main->Config(), "Video", "FPSNum", num);
		config_set_int(main->Config(), "Video", "FPSDen", den);
	}

	// Vertical Video Settings
	if (WidgetChanged(ui->baseResolution_v)) {
		config_set_uint(main->Config(), "Video", "BaseCX_V", baseCX_v); // New config key
		config_set_uint(main->Config(), "Video", "BaseCY_V", baseCY_v); // New config key
		config_set_bool(main->Config(), "Video", "customBase_V", customBase_v); // New config key
	}
	if (WidgetChanged(ui->outputResolution_v)) {
		config_set_uint(main->Config(), "Video", "OutputCX_V", outputCX_v); // New config key
		config_set_uint(main->Config(), "Video", "OutputCY_V", outputCY_v); // New config key
		config_set_bool(main->Config(), "Video", "customOutput_V", customOutput_v); // New config key
	}

	if (WidgetChanged(ui->downscaleFilter_v)) {
		int idx_v = ui->downscaleFilter_v->currentIndex();
		const char *filter_v = (idx_v == 0)   ? "bicubic"
				       : (idx_v == 1) ? "lanczos"
				       : (idx_v == 2) ? "bilinear"
						    : "area";
		config_set_string(main->Config(), "Video", "ScaleFilter_V", filter_v); // New config key
	}

	if (WidgetChanged(ui->fpsType_v) || WidgetChanged(ui->fpsCommon_v) ||
	    WidgetChanged(ui->fpsInteger_v) || WidgetChanged(ui->fpsNumerator_v) ||
	    WidgetChanged(ui->fpsDenominator_v)) {
		uint32_t num_v, den_v;
		GetFPSVal(ui->fpsType_v, ui->fpsCommon_v, ui->fpsInteger_v,
			  ui->fpsNumerator_v, ui->fpsDenominator_v, num_v, den_v);

		config_set_int(main->Config(), "Video", "FPSType_V", ui->fpsType_v->currentIndex()); // New config key
		config_set_string(main->Config(), "Video", "FPSCommon_V", ui->fpsCommon_v->currentText().toUtf8().constData()); // New config key
		config_set_int(main->Config(), "Video", "FPSInt_V", ui->fpsInteger_v->value()); // New config key
		config_set_int(main->Config(), "Video", "FPSNum_V", num_v); // New config key
		config_set_int(main->Config(), "Video", "FPSDen_V", den_v); // New config key
	}

	// Common Video Settings
	if (WidgetChanged(ui->previewFormat))
		SaveComboData(ui->previewFormat, "Video", "PreviewFormat");
	if (WidgetChanged(ui->colorFormat))
		SaveComboData(ui->colorFormat, "Video", "ColorFormat");
	if (WidgetChanged(ui->colorSpace))
		SaveCombo(ui->colorSpace, "Video", "ColorSpace");
	if (WidgetChanged(ui->colorRange))
		config_set_int(main->Config(), "Video", "ColorRange",
			       ui->colorRange->currentIndex());
	if (WidgetChanged(ui->sdrWhiteLevel))
		config_set_int(main->Config(), "Video", "SDRWhiteLevel",
			       ui->sdrWhiteLevel->currentIndex());
	if (WidgetChanged(ui->hdrNominalPeakLevel))
		config_set_int(main->Config(), "Video", "HDRNominalPeakLevel",
			       ui->hdrNominalPeakLevel->currentIndex());

	// Populate and update horizontal_ovi in OBSApp
	obs_video_info h_ovi = {};
	h_ovi.adapter         = 0; // Assuming default adapter
	h_ovi.graphics_module = obs_get_graphics_module(); // Get current graphics module
	h_ovi.base_width      = baseCX;
	h_ovi.base_height     = baseCY;
	h_ovi.output_width    = outputCX;
	h_ovi.output_height   = outputCY;
	GetFPSVal(ui->fpsType, ui->fpsCommon, ui->fpsInteger, ui->fpsNumerator, ui->fpsDenominator, h_ovi.fps_num, h_ovi.fps_den);

	const char *scale_filter_str_h = config_get_string(main->Config(), "Video", "ScaleFilter");
	if (strcmp(scale_filter_str_h, "bicubic") == 0) h_ovi.scale_type = OBS_SCALE_BICUBIC;
	else if (strcmp(scale_filter_str_h, "lanczos") == 0) h_ovi.scale_type = OBS_SCALE_LANCZOS;
	else if (strcmp(scale_filter_str_h, "bilinear") == 0) h_ovi.scale_type = OBS_SCALE_BILINEAR;
	else h_ovi.scale_type = OBS_SCALE_DISABLE; // Or some default like OBS_SCALE_AREA for "area"

	// Populate h_ovi.output_format, h_ovi.colorspace, h_ovi.range from UI/config
	// These settings are read from the UI elements directly.
	// The config string for color format is ui->colorFormat->currentData().toString().toUtf8().constData()
	// The config string for color space is ui->colorSpace->currentText().toUtf8().constData()
	// The config string for color range is ui->colorRange->currentIndex() (0 for Partial, 1 for Full)

	h_ovi.output_format = GetVideoFormatFromName(config_get_string(main->Config(), "Video", "ColorFormat"));
	h_ovi.colorspace    = GetVideoColorSpaceFromName(config_get_string(main->Config(), "Video", "ColorSpace"));
	h_ovi.range         = config_get_int(main->Config(), "Video", "ColorRange") == 1 ? VIDEO_RANGE_FULL : VIDEO_RANGE_PARTIAL;


	App()->UpdateHorizontalVideoInfo(h_ovi);

	if (App()->IsDualOutputActive()) {
		obs_video_info v_ovi = {};
		v_ovi.adapter         = 0;
		v_ovi.graphics_module = obs_get_graphics_module();
		v_ovi.base_width      = baseCX_v;
		v_ovi.base_height     = baseCY_v;
		v_ovi.output_width    = outputCX_v;
		v_ovi.output_height   = outputCY_v;
		GetFPSVal(ui->fpsType_v, ui->fpsCommon_v, ui->fpsInteger_v, ui->fpsNumerator_v, ui->fpsDenominator_v, v_ovi.fps_num, v_ovi.fps_den);

		// For vertical, scale type is from its own dropdown
		v_ovi.scale_type = GetScaleTypeFilter(main->Config(), "ScaleFilter_V");


		// Vertical output uses the same color format, space, and range settings as horizontal by design of the current UI.
		// These values are read from the config which should reflect the UI settings for these common properties.
		v_ovi.output_format = GetVideoFormatFromName(config_get_string(main->Config(), "Video", "ColorFormat"));
		v_ovi.colorspace    = GetVideoColorSpaceFromName(config_get_string(main->Config(), "Video", "ColorSpace"));
		v_ovi.range         = config_get_int(main->Config(), "Video", "ColorRange") == 1 ? VIDEO_RANGE_FULL : VIDEO_RANGE_PARTIAL;

		App()->UpdateVerticalVideoInfo(v_ovi);
	}
}

/* ------------------------------------------------------------------------- */
/* Audio */

void OBSBasicSettings::LoadAudioSettings()
{
	/* ------------------ */
	/* General audio */

	ui->sampleRate->clear();
	ui->sampleRate->addItem("44.1 kHz");
	ui->sampleRate->addItem("48 kHz");

	uint32_t sampleRate =
		config_get_uint(main->Config(), "Audio", "SampleRate");
	if (sampleRate == 44100)
		ui->sampleRate->setCurrentIndex(0);
	else
		ui->sampleRate->setCurrentIndex(1);

	ui->channels->clear();
	ui->channels->addItem(QTStr("Basic.Settings.Audio.Channels.Mono"));
	ui->channels->addItem(QTStr("Basic.Settings.Audio.Channels.Stereo"));
	ui->channels->addItem("2.1");
	ui->channels->addItem("4.0");
	ui->channels->addItem("4.1");
	ui->channels->addItem("5.1");
	ui->channels->addItem("7.1");

	int channels = config_get_int(main->Config(), "Audio", "Channels");
	if (channels <= SPEAKERS_MONO)
		ui->channels->setCurrentIndex(0);
	else if (channels <= SPEAKERS_STEREO)
		ui->channels->setCurrentIndex(1);
	else if (channels <= SPEAKERS_2POINT1)
		ui->channels->setCurrentIndex(2);
	else if (channels <= SPEAKERS_4POINT0)
		ui->channels->setCurrentIndex(3);
	else if (channels <= SPEAKERS_4POINT1)
		ui->channels->setCurrentIndex(4);
	else if (channels <= SPEAKERS_5POINT1)
		ui->channels->setCurrentIndex(5);
	else
		ui->channels->setCurrentIndex(6);

	/* ------------------ */
	/* device ID lists */

	vector<pair<string, string>> list;

	obs_enum_audio_device_types([](void *param, obs_audio_type type) {
		QComboBox *comboBox = reinterpret_cast<QComboBox *>(param);
		if (type == OBS_AUDIO_DEVICE_TYPE_CAPTURE)
			comboBox->addItem(QTStr("Basic.Settings.Audio.Device.DesktopDevice"));
		else if (type == OBS_AUDIO_DEVICE_TYPE_PLAYBACK)
			comboBox->addItem(QTStr("Basic.Settings.Audio.Device.MicDevice"));
		return true;
	},
				    ui->desktopDevice);

	ui->desktopDevice->insertSeparator(ui->desktopDevice->count());
	ui->micDevice1->insertSeparator(ui->micDevice1->count());
	ui->micDevice2->insertSeparator(ui->micDevice2->count());
	ui->micDevice3->insertSeparator(ui->micDevice3->count());
	ui->micDevice4->insertSeparator(ui->micDevice4->count());

	main->GetAudioDeviceList(list, OBS_AUDIO_DEVICE_TYPE_CAPTURE);
	for (auto &iter : list) {
		ui->desktopDevice->addItem(iter.first.c_str(), iter.second.c_str());
	}

	list.clear();
	main->GetAudioDeviceList(list, OBS_AUDIO_DEVICE_TYPE_PLAYBACK);
	for (auto &iter : list) {
		ui->micDevice1->addItem(iter.first.c_str(), iter.second.c_str());
		ui->micDevice2->addItem(iter.first.c_str(), iter.second.c_str());
		ui->micDevice3->addItem(iter.first.c_str(), iter.second.c_str());
		ui->micDevice4->addItem(iter.first.c_str(), iter.second.c_str());
	}

	SetComboByData(ui->desktopDevice,
		       config_get_string(main->Config(), "Audio",
					 "DesktopDeviceID"));
	SetComboByData(ui->micDevice1,
		       config_get_string(main->Config(), "Audio",
					 "MicDeviceID"));
	SetComboByData(ui->micDevice2,
		       config_get_string(main->Config(), "Audio",
					 "AuxDeviceID"));
	SetComboByData(ui->micDevice3,
		       config_get_string(main->Config(), "Audio",
					 "AuxDeviceID2"));
	SetComboByData(ui->micDevice4,
		       config_get_string(main->Config(), "Audio",
					 "AuxDeviceID3"));

	/* ------------------ */
	/* Advanced audio */

	ui->monitoringDevice->clear();
	ui->monitoringDevice->addItem(
		QTStr("Basic.Settings.Audio.MonitoringDevice.Default"));
	ui->monitoringDevice->addItem(
		QTStr("Basic.Settings.Audio.MonitoringDevice.Disabled"));
	ui->monitoringDevice->insertSeparator(ui->monitoringDevice->count());

	list.clear();
	main->GetAudioDeviceList(list, OBS_AUDIO_DEVICE_TYPE_PLAYBACK);
	for (auto &iter : list) {
		ui->monitoringDevice->addItem(iter.first.c_str(),
					      iter.second.c_str());
	}

	SetComboByData(ui->monitoringDevice,
		       config_get_string(main->Config(), "Audio",
					 "MonitoringDeviceID"));

	ui->meterDecayRate->setCurrentIndex(
		config_get_int(main->Config(), "Audio", "MeterDecayRate"));
	ui->peakMeterType->setCurrentIndex(
		config_get_int(main->Config(), "Audio", "PeakMeterType"));
	ui->enablePushToTalk->setChecked(
		config_get_bool(main->Config(), "Audio", "PushToTalk"));
	ui->pushToTalkDelay->setValue(
		config_get_int(main->Config(), "Audio", "PushToTalkDelay"));
	ui->enablePushToMute->setChecked(
		config_get_bool(main->Config(), "Audio", "PushToMute"));
	ui->pushToMuteDelay->setValue(
		config_get_int(main->Config(), "Audio", "PushToMuteDelay"));
	ui->enableDesktopPushToTalk->setChecked(config_get_bool(
		main->Config(), "Audio", "DesktopPushToTalk"));
	ui->desktopPushToTalkDelay->setValue(config_get_int(
		main->Config(), "Audio", "DesktopPushToTalkDelay"));
	ui->enableDesktopPushToMute->setChecked(config_get_bool(
		main->Config(), "Audio", "DesktopPushToMute"));
	ui->desktopPushToMuteDelay->setValue(config_get_int(
		main->Config(), "Audio", "DesktopPushToMuteDelay"));
	ui->hotkeysFocusSources->setChecked(config_get_bool(
		main->Config(), "Audio", "HotkeysFocusSourcesWindow"));
	ui->disableAudioDucking->setChecked(
		config_get_bool(main->Config(), "Audio", "DisableAudioDucking"));

	AudioConfigChanged();
}

void OBSBasicSettings::SaveAudioSettings()
{
	/* ------------------ */
	/* General audio */

	if (WidgetChanged(ui->sampleRate)) {
		int rate = (ui->sampleRate->currentIndex() == 0) ? 44100 : 48000;
		config_set_int(main->Config(), "Audio", "SampleRate", rate);
	}
	if (WidgetChanged(ui->channels)) {
		int channels = 0;
		switch (ui->channels->currentIndex()) {
		case 0:
			channels = SPEAKERS_MONO;
			break;
		case 1:
			channels = SPEAKERS_STEREO;
			break;
		case 2:
			channels = SPEAKERS_2POINT1;
			break;
		case 3:
			channels = SPEAKERS_4POINT0;
			break;
		case 4:
			channels = SPEAKERS_4POINT1;
			break;
		case 5:
			channels = SPEAKERS_5POINT1;
			break;
		case 6:
			channels = SPEAKERS_7POINT1;
			break;
		}
		config_set_int(main->Config(), "Audio", "Channels", channels);
	}

	/* ------------------ */
	/* device ID lists */

	if (WidgetChanged(ui->desktopDevice))
		SaveComboData(ui->desktopDevice, "Audio", "DesktopDeviceID");
	if (WidgetChanged(ui->micDevice1))
		SaveComboData(ui->micDevice1, "Audio", "MicDeviceID");
	if (WidgetChanged(ui->micDevice2))
		SaveComboData(ui->micDevice2, "Audio", "AuxDeviceID");
	if (WidgetChanged(ui->micDevice3))
		SaveComboData(ui->micDevice3, "Audio", "AuxDeviceID2");
	if (WidgetChanged(ui->micDevice4))
		SaveComboData(ui->micDevice4, "Audio", "AuxDeviceID3");

	/* ------------------ */
	/* Advanced audio */

	if (WidgetChanged(ui->monitoringDevice))
		SaveComboData(ui->monitoringDevice, "Audio",
			      "MonitoringDeviceID");
	if (WidgetChanged(ui->meterDecayRate))
		config_set_int(main->Config(), "Audio", "MeterDecayRate",
			       ui->meterDecayRate->currentIndex());
	if (WidgetChanged(ui->peakMeterType))
		config_set_int(main->Config(), "Audio", "PeakMeterType",
			       ui->peakMeterType->currentIndex());
	if (WidgetChanged(ui->enablePushToTalk))
		config_set_bool(main->Config(), "Audio", "PushToTalk",
				ui->enablePushToTalk->isChecked());
	if (WidgetChanged(ui->pushToTalkDelay))
		config_set_int(main->Config(), "Audio", "PushToTalkDelay",
			       ui->pushToTalkDelay->value());
	if (WidgetChanged(ui->enablePushToMute))
		config_set_bool(main->Config(), "Audio", "PushToMute",
				ui->enablePushToMute->isChecked());
	if (WidgetChanged(ui->pushToMuteDelay))
		config_set_int(main->Config(), "Audio", "PushToMuteDelay",
			       ui->pushToMuteDelay->value());
	if (WidgetChanged(ui->enableDesktopPushToTalk))
		config_set_bool(main->Config(), "Audio", "DesktopPushToTalk",
				ui->enableDesktopPushToTalk->isChecked());
	if (WidgetChanged(ui->desktopPushToTalkDelay))
		config_set_int(main->Config(), "Audio",
			       "DesktopPushToTalkDelay",
			       ui->desktopPushToTalkDelay->value());
	if (WidgetChanged(ui->enableDesktopPushToMute))
		config_set_bool(main->Config(), "Audio", "DesktopPushToMute",
				ui->enableDesktopPushToMute->isChecked());
	if (WidgetChanged(ui->desktopPushToMuteDelay))
		config_set_int(main->Config(), "Audio",
			       "DesktopPushToMuteDelay",
			       ui->desktopPushToMuteDelay->value());
	if (WidgetChanged(ui->hotkeysFocusSources))
		config_set_bool(main->Config(), "Audio",
				"HotkeysFocusSourcesWindow",
				ui->hotkeysFocusSources->isChecked());
	if (WidgetChanged(ui->disableAudioDucking))
		config_set_bool(main->Config(), "Audio", "DisableAudioDucking",
				ui->disableAudioDucking->isChecked());

	AudioConfigChanged();
}

void OBSBasicSettings::AudioConfigChanged()
{
	ui->pushToTalkDelay->setEnabled(ui->enablePushToTalk->isChecked());
	ui->pushToMuteDelay->setEnabled(ui->enablePushToMute->isChecked());
	ui->desktopPushToTalkDelay->setEnabled(
		ui->enableDesktopPushToTalk->isChecked());
	ui->desktopPushToMuteDelay->setEnabled(
		ui->enableDesktopPushToMute->isChecked());
	main->AudioConfigChange();
}

/* ------------------------------------------------------------------------- */
/* Hotkeys */

void OBSBasicSettings::LoadHotkeySettings()
{
	ui->hotkeyFilter->setText("");
	ui->hotkeyTree->Load();
}

void OBSBasicSettings::SaveHotkeySettings()
{
	ui->hotkeyTree->Save();
}

void OBSBasicSettings::on_hotkeyFilter_textChanged(const QString &text)
{
	ui->hotkeyTree->Filter(text);
}

/* ------------------------------------------------------------------------- */
/* Advanced */

void OBSBasicSettings::StreamDelayEnableChanged(bool checked)
{
	if (loading)
		return;

	ui->streamDelaySec->setEnabled(checked);
	ui->streamDelayPreserve->setEnabled(checked);
	ui->streamDelayPassword->setEnabled(checked);
	UpdateStreamDelayWarning();
	SetWidgetChanged(ui->streamDelayEnable);
}

void OBSBasicSettings::StreamDelaySecChanged(int /*i*/)
{
	if (loading)
		return;
	SetWidgetChanged(ui->streamDelaySec);
}

void OBSBasicSettings::StreamDelayPreserveChanged(bool /*checked*/)
{
	if (loading)
		return;
	SetWidgetChanged(ui->streamDelayPreserve);
}

void OBSBasicSettings::UpdateStreamDelayEstimate()
{
	OBSService service = main->GetService();
	obs_encoder_t *encoder = obs_frontend_get_streaming_encoder();
	if (!service || !encoder) {
		ui->streamDelayEstimate->setText(QTStr("Unavailable"));
		return;
	}

	obs_data_t *s_settings = obs_encoder_get_settings(encoder);
	obs_data_t *settings = obs_data_create();
	obs_data_apply(settings, s_settings);

	obs_output_set_delay(output, 0, 0);
	int64_t delay_ns = obs_output_get_total_bytes_sent(output) > 0
				   ? obs_output_get_active_delay(output)
				   : 0;
	if (delay_ns == 0) {
		obs_encoder_update(encoder, settings);
		obs_service_apply_encoder_settings(service, settings, encoder);
		delay_ns = obs_encoder_get_active_delay(encoder);
	}

	if (streamEncoderProps)
		streamEncoderProps->UpdateProperties(settings);

	obs_data_release(s_settings);
	obs_data_release(settings);

	QString text;
	text.sprintf("%.1f %s", double(delay_ns) / 1000000000.0,
		     QTStr("seconds").toUtf8().constData());
	ui->streamDelayEstimate->setText(text);
}

void OBSBasicSettings::UpdateRecDelayEstimate()
{
	obs_output_t *fileOutput = obs_frontend_get_recording_output();
	if (!fileOutput) {
		ui->recDelayEstimate->setText(QTStr("Unavailable"));
		return;
	}

	obs_output_set_delay(fileOutput, 0, 0);
	int64_t delay_ns = obs_output_get_total_bytes_sent(fileOutput) > 0
				   ? obs_output_get_active_delay(fileOutput)
				   : 0;

	QString text;
	text.sprintf("%.1f %s", double(delay_ns) / 1000000000.0,
		     QTStr("seconds").toUtf8().constData());
	ui->recDelayEstimate->setText(text);
}

void OBSBasicSettings::UpdateStreamDelayWarning()
{
	bool active = obs_frontend_streaming_active();
	bool enabled = ui->streamDelayEnable->isChecked();
	uint32_t duration = ui->streamDelaySec->value();
	bool preserve = ui->streamDelayPreserve->isChecked();
	bool reconnect = ui->autoReconnectEnable->isChecked();

	if (streamDelayChanging)
		return;

	if (!active && streamDelayActive) {
		streamDelayStopping = true;
		obs_output_set_delay(output, 0, 0);
		streamDelayActive = false;
		streamDelayStopping = false;
	}

	if (enabled && duration > 0) {
		if (!streamDelayActive && !reconnect) {
			streamDelayStarting = true;
			obs_output_set_delay(output, duration,
					     preserve ? OBS_OUTPUT_DELAY_PRESERVE
						      : 0);
			streamDelayActive = true;
			streamDelayStarting = false;
		}
	} else if (streamDelayActive) {
		streamDelayStopping = true;
		obs_output_set_delay(output, 0, 0);
		streamDelayActive = false;
		streamDelayStopping = false;
	}

	if (enabled && duration > 0) {
		if (preserve && !reconnect) {
			ui->streamDelayInfo->setText(QTStr(
				"Basic.Settings.Advanced.StreamDelay.Info.PreserveDelay"));
		} else if (reconnect) {
			ui->streamDelayInfo->setText(QTStr(
				"Basic.Settings.Advanced.StreamDelay.Info.AutoReconnect"));
		} else {
			ui->streamDelayInfo->setText(QTStr(
				"Basic.Settings.Advanced.StreamDelay.Info.DelayOnly"));
		}
		ui->streamDelayInfo->setVisible(true);
	} else {
		ui->streamDelayInfo->setVisible(false);
	}

	UpdateStreamDelayEstimate();
}

void OBSBasicSettings::UpdateRecDelayWarning()
{
	obs_output_t *fileOutput = obs_frontend_get_recording_output();
	if (!fileOutput)
		return;

	bool active = obs_frontend_recording_active();
	bool delayEnabled =
		config_get_bool(main->Config(), "AdvOut", "RecDelayEnable");
	uint32_t duration =
		config_get_uint(main->Config(), "AdvOut", "RecDelaySec");

	if (recDelayChanging)
		return;

	if (!active && recDelayActive) {
		recDelayStopping = true;
		obs_output_set_delay(fileOutput, 0, 0);
		recDelayActive = false;
		recDelayStopping = false;
	}

	if (delayEnabled && duration > 0) {
		if (!recDelayActive) {
			recDelayStarting = true;
			obs_output_set_delay(fileOutput, duration, 0);
			recDelayActive = true;
			recDelayStarting = false;
		}
	} else if (recDelayActive) {
		recDelayStopping = true;
		obs_output_set_delay(fileOutput, 0, 0);
		recDelayActive = false;
		recDelayStopping = false;
	}

	UpdateRecDelayEstimate();
}

void OBSBasicSettings::AutoReconnectEnableChanged(bool checked)
{
	if (loading)
		return;

	ui->retryDelaySec->setEnabled(checked);
	ui->maxRetries->setEnabled(checked);
	SetWidgetChanged(ui->autoReconnectEnable);
	UpdateStreamDelayWarning();
}

void OBSBasicSettings::RetryDelaySecChanged(int /*i*/)
{
	if (loading)
		return;
	SetWidgetChanged(ui->retryDelaySec);
}

void OBSBasicSettings::MaxRetriesChanged(int /*i*/)
{
	if (loading)
		return;
	SetWidgetChanged(ui->maxRetries);
}

void OBSBasicSettings::EnableReplayBufferChanged(bool checked)
{
	if (loading)
		return;

	ui->replayBufferSeconds->setEnabled(checked);
	ui->replayBufferPath->setEnabled(checked);
	ui->replayBufferPathBrowse->setEnabled(checked);
	ui->replayBufferFormat->setEnabled(checked);
	ui->replayBufferEncoder->setEnabled(checked);
	ui->replayBufferNameFormat->setEnabled(checked);
	ui->replayBufferSuffix->setEnabled(checked);
	ui->replayBufferOverwrite->setEnabled(checked);
	SetWidgetChanged(ui->enableReplayBuffer);
}

void OBSBasicSettings::ReplayBufferSecondsChanged(int /*i*/)
{
	if (loading)
		return;
	SetWidgetChanged(ui->replayBufferSeconds);
}

void OBSBasicSettings::BrowseReplayBufferPath()
{
	QString path = QFileDialog::getExistingDirectory(
		this, QTStr("Basic.Settings.Output.ReplayBuffer.Directory"),
		ui->replayBufferPath->text());

	if (!path.isEmpty()) {
		ui->replayBufferPath->setText(path);
		SetWidgetChanged(ui->replayBufferPath);
	}
}

void OBSBasicSettings::LoadAdvancedSettings()
{
	bool streamActive = obs_frontend_streaming_active();
	bool recordActive = obs_frontend_recording_active();
	bool replayActive = obs_frontend_replay_buffer_active();

	obs_output_t *fileOutput = obs_frontend_get_recording_output();
	if (fileOutput)
		recDelayActive = obs_output_get_delay(fileOutput) > 0;

	ui->processPriority->clear();
	ui->processPriority->addItem(QTStr("High"));
	ui->processPriority->addItem(QTStr("AboveNormal"));
	ui->processPriority->addItem(QTStr("Normal"));
	ui->processPriority->addItem(QTStr("BelowNormal"));
	ui->processPriority->addItem(QTStr("Idle"));

	int priority = config_get_int(main->Config(), "General",
				      "ProcessPriority");
	if (priority == OBS_PROCESS_PRIORITY_HIGH)
		ui->processPriority->setCurrentIndex(0);
	else if (priority == OBS_PROCESS_PRIORITY_ABOVE_NORMAL)
		ui->processPriority->setCurrentIndex(1);
	else if (priority == OBS_PROCESS_PRIORITY_NORMAL)
		ui->processPriority->setCurrentIndex(2);
	else if (priority == OBS_PROCESS_PRIORITY_BELOW_NORMAL)
		ui->processPriority->setCurrentIndex(3);
	else
		ui->processPriority->setCurrentIndex(4);

	/* stream delay */
	streamDelayActive = obs_output_get_delay(output) > 0;
	ui->streamDelayEnable->setChecked(config_get_bool(
		main->Config(), "AdvOut", "DelayEnable"));
	ui->streamDelaySec->setValue(
		config_get_int(main->Config(), "AdvOut", "DelaySec"));
	ui->streamDelayPreserve->setChecked(config_get_bool(
		main->Config(), "AdvOut", "DelayPreserve"));
	ui->streamDelayPassword->setText(config_get_string(
		main->Config(), "AdvOut", "DelayPassword"));

	/* recording delay (deprecated) */
	config_set_bool(main->Config(), "AdvOut", "RecDelayEnable", false);
	config_set_uint(main->Config(), "AdvOut", "RecDelaySec", 0);

	/* automatic reconnect */
	ui->autoReconnectEnable->setChecked(config_get_bool(
		main->Config(), "AdvOut", "Reconnect"));
	ui->retryDelaySec->setValue(
		config_get_int(main->Config(), "AdvOut", "RetryDelay"));
	ui->maxRetries->setValue(
		config_get_int(main->Config(), "AdvOut", "MaxRetries"));

	ui->bindToIP->clear();
	ui->bindToIP->addItem(QTStr("Default"));
	ui->bindToIP->insertSeparator(1);

	vector<string> ips;
	GetIPAddresses(ips);
	for (auto &iter : ips)
		ui->bindToIP->addItem(iter.c_str());

	SetComboByText(ui->bindToIP,
		       config_get_string(main->Config(), "AdvOut", "BindIP"));
	ui->enableNewSocketLoop->setChecked(
		config_get_bool(main->Config(), "AdvOut", "NewSocketLoop"));
	ui->enableNetworkOptimizations->setChecked(config_get_bool(
		main->Config(), "AdvOut", "NetworkOptimizations"));
	ui->lowLatencyMode->setChecked(
		config_get_bool(main->Config(), "AdvOut", "LowLatencyMode"));

	ui->processPriority->setDisabled(streamActive || recordActive ||
					 replayActive);
	ui->streamDelayEnable->setDisabled(streamActive);
	ui->streamDelaySec->setDisabled(streamActive ||
					!ui->streamDelayEnable->isChecked());
	ui->streamDelayPreserve->setDisabled(
		streamActive || !ui->streamDelayEnable->isChecked());
	ui->streamDelayPassword->setDisabled(
		streamActive || !ui->streamDelayEnable->isChecked());
	ui->autoReconnectEnable->setDisabled(streamActive);
	ui->retryDelaySec->setDisabled(streamActive ||
				       !ui->autoReconnectEnable->isChecked());
	ui->maxRetries->setDisabled(streamActive ||
				    !ui->autoReconnectEnable->isChecked());
	ui->bindToIP->setDisabled(streamActive || recordActive || replayActive);
	ui->enableNewSocketLoop->setDisabled(streamActive || recordActive ||
					     replayActive);
	ui->enableNetworkOptimizations->setDisabled(streamActive ||
						    recordActive ||
						    replayActive);
	ui->lowLatencyMode->setDisabled(streamActive || recordActive ||
					replayActive);

	StreamDelayEnableChanged(ui->streamDelayEnable->isChecked());
	AutoReconnectEnableChanged(ui->autoReconnectEnable->isChecked());
	UpdateStreamDelayWarning();
	UpdateRecDelayWarning();
}

void OBSBasicSettings::SaveAdvancedSettings()
{
	if (WidgetChanged(ui->processPriority)) {
		int priority = 0;
		switch (ui->processPriority->currentIndex()) {
		case 0:
			priority = OBS_PROCESS_PRIORITY_HIGH;
			break;
		case 1:
			priority = OBS_PROCESS_PRIORITY_ABOVE_NORMAL;
			break;
		case 2:
			priority = OBS_PROCESS_PRIORITY_NORMAL;
			break;
		case 3:
			priority = OBS_PROCESS_PRIORITY_BELOW_NORMAL;
			break;
		case 4:
			priority = OBS_PROCESS_PRIORITY_IDLE;
			break;
		}
		config_set_int(main->Config(), "General", "ProcessPriority",
			       priority);
	}

	/* stream delay */
	if (WidgetChanged(ui->streamDelayEnable))
		config_set_bool(main->Config(), "AdvOut", "DelayEnable",
				ui->streamDelayEnable->isChecked());
	if (WidgetChanged(ui->streamDelaySec))
		config_set_int(main->Config(), "AdvOut", "DelaySec",
			       ui->streamDelaySec->value());
	if (WidgetChanged(ui->streamDelayPreserve))
		config_set_bool(main->Config(), "AdvOut", "DelayPreserve",
				ui->streamDelayPreserve->isChecked());
	if (WidgetChanged(ui->streamDelayPassword))
		config_set_string(main->Config(), "AdvOut", "DelayPassword",
				  ui->streamDelayPassword->text()
					  .toUtf8()
					  .constData());

	/* automatic reconnect */
	if (WidgetChanged(ui->autoReconnectEnable))
		config_set_bool(main->Config(), "AdvOut", "Reconnect",
				ui->autoReconnectEnable->isChecked());
	if (WidgetChanged(ui->retryDelaySec))
		config_set_int(main->Config(), "AdvOut", "RetryDelay",
			       ui->retryDelaySec->value());
	if (WidgetChanged(ui->maxRetries))
		config_set_int(main->Config(), "AdvOut", "MaxRetries",
			       ui->maxRetries->value());

	if (WidgetChanged(ui->bindToIP))
		SaveCombo(ui->bindToIP, "AdvOut", "BindIP");
	if (WidgetChanged(ui->enableNewSocketLoop))
		config_set_bool(main->Config(), "AdvOut", "NewSocketLoop",
				ui->enableNewSocketLoop->isChecked());
	if (WidgetChanged(ui->enableNetworkOptimizations))
		config_set_bool(main->Config(), "AdvOut",
				"NetworkOptimizations",
				ui->enableNetworkOptimizations->isChecked());
	if (WidgetChanged(ui->lowLatencyMode))
		config_set_bool(main->Config(), "AdvOut", "LowLatencyMode",
				ui->lowLatencyMode->isChecked());
}

/* ------------------------------------------------------------------------- */

void OBSBasicSettings::on_useDualOutputCheckBox_stateChanged(int state)
{
	bool checked = (state == Qt::Checked);

	// Video page
	ui->videoSettingsTabWidget->setTabEnabled(1, checked); // Enable/disable Vertical Video tab

	// Output page (Simple)
	ui->simpleStreamingTabs->setTabEnabled(1, checked); // Enable/disable Vertical Streaming (Simple)
	ui->simpleRecordingTabs->setTabEnabled(1, checked); // Enable/disable Vertical Recording (Simple)

	// Output page (Advanced)
	ui->advOutTabs->setTabEnabled(ui->advOutTabs->indexOf(ui->advOutputStreamTab_v), checked); // Enable/disable Vertical Streaming (Advanced)
	ui->advOutTabs->setTabEnabled(ui->advOutTabs->indexOf(ui->advOutputRecordTab_v), checked); // Enable/disable Vertical Recording (Advanced)

	if (!loading) {
		SetWidgetChanged(ui->useDualOutputCheckBox);
		// Potentially trigger a reload/resave of output settings if mode changes affect availability
		if (ui->outputMode->currentIndex() == 1) { // Advanced mode
			blockSaving = true; // Prevent immediate save during load
			LoadOutputSettings(); // Reload to show/hide vertical advanced sections correctly
			blockSaving = false;
		}
	}
}

void OBSBasicSettings::ManifestFileChanged(const std::string &file)
{
	if (file == "streamEncoder.json" || file == "recordEncoder.json" ||
	    file == "streamEncoder_v_stream.json" || file == "recordEncoder_v_rec.json") { // Added vertical JSONs
		if (simpleOutput)
			return;

		ResetEncoders();
	}
}

void OBSBasicSettings::ServiceChanged()
{
	const char *protocol = "";
	OBSService service = main->GetService();
	if (service) {
		obs_data_t *settings = obs_service_get_settings(service);
		protocol = obs_data_get_string(settings, "protocol");
		obs_data_release(settings);
	}

	SwapMultiTrack(protocol);
}

void OBSBasicSettings::StreamDelayStarting()
{
	streamDelayChanging = true;
	ui->streamDelayEnable->setDisabled(true);
	ui->streamDelaySec->setDisabled(true);
	ui->streamDelayPreserve->setDisabled(true);
	ui->streamDelayPassword->setDisabled(true);
	streamDelayChanging = false;
}

void OBSBasicSettings::StreamDelayStopping()
{
	streamDelayChanging = true;
	ui->streamDelayEnable->setDisabled(true);
	ui->streamDelaySec->setDisabled(true);
	ui->streamDelayPreserve->setDisabled(true);
	ui->streamDelayPassword->setDisabled(true);
	streamDelayChanging = false;
}

void OBSBasicSettings::RecDelayStarting()
{
	recDelayChanging = true;
	recDelayChanging = false;
}

void OBSBasicSettings::RecDelayStopping()
{
	recDelayChanging = true;
	recDelayChanging = false;
}
