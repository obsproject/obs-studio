/******************************************************************************
    Copyright (C) 2013-2014 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include <obs.hpp>
#include <util/util.hpp>
#include <util/lexer.h>
#include <sstream>
#include <QLineEdit>
#include <QMessageBox>
#include <QCloseEvent>
#include <QFileDialog>

#include "obs-app.hpp"
#include "platform.hpp"
#include "properties-view.hpp"
#include "qt-wrappers.hpp"
#include "window-basic-main.hpp"
#include "window-basic-settings.hpp"

#include <util/platform.h>

using namespace std;

/* parses "[width]x[height]", string, i.e. 1024x768 */
static bool ConvertResText(const char *res, uint32_t &cx, uint32_t &cy)
{
	BaseLexer lex;
	base_token token;

	lexer_start(lex, res);

	/* parse width */
	if (!lexer_getbasetoken(lex, &token, IGNORE_WHITESPACE))
		return false;
	if (token.type != BASETOKEN_DIGIT)
		return false;

	cx = std::stoul(token.text.array);

	/* parse 'x' */
	if (!lexer_getbasetoken(lex, &token, IGNORE_WHITESPACE))
		return false;
	if (strref_cmpi(&token.text, "x") != 0)
		return false;

	/* parse height */
	if (!lexer_getbasetoken(lex, &token, IGNORE_WHITESPACE))
		return false;
	if (token.type != BASETOKEN_DIGIT)
		return false;

	cy = std::stoul(token.text.array);

	/* shouldn't be any more tokens after this */
	if (lexer_getbasetoken(lex, &token, IGNORE_WHITESPACE))
		return false;

	return true;
}

static inline bool WidgetChanged(QWidget *widget)
{
	return widget->property("changed").toBool();
}

static inline void SetComboByName(QComboBox *combo, const char *name)
{
	int idx = combo->findText(QT_UTF8(name));
	if (idx != -1)
		combo->setCurrentIndex(idx);
}

static inline void SetComboByValue(QComboBox *combo, const char *name)
{
	int idx = combo->findData(QT_UTF8(name));
	if (idx != -1)
		combo->setCurrentIndex(idx);
}

static inline QString GetComboData(QComboBox *combo)
{
	int idx = combo->currentIndex();
	if (idx == -1)
		return QString();

	return combo->itemData(idx).toString();
}

void OBSBasicSettings::HookWidget(QWidget *widget, const char *signal,
		const char *slot)
{
	QObject::connect(widget, signal, this, slot);
	widget->setProperty("changed", QVariant(false));
}

#define COMBO_CHANGED   SIGNAL(currentIndexChanged(int))
#define EDIT_CHANGED    SIGNAL(textChanged(const QString &))
#define CBEDIT_CHANGED  SIGNAL(editTextChanged(const QString &))
#define CHECK_CHANGED   SIGNAL(clicked(bool))
#define SCROLL_CHANGED  SIGNAL(valueChanged(int))

#define GENERAL_CHANGED SLOT(GeneralChanged())
#define OUTPUTS_CHANGED SLOT(OutputsChanged())
#define AUDIO_RESTART   SLOT(AudioChangedRestart())
#define AUDIO_CHANGED   SLOT(AudioChanged())
#define VIDEO_RESTART   SLOT(VideoChangedRestart())
#define VIDEO_RES       SLOT(VideoChangedResolution())
#define VIDEO_CHANGED   SLOT(VideoChanged())

OBSBasicSettings::OBSBasicSettings(QWidget *parent)
	: QDialog          (parent),
	  main             (qobject_cast<OBSBasic*>(parent)),
	  ui               (new Ui::OBSBasicSettings)
{
	string path;

	ui->setupUi(this);

	HookWidget(ui->language,             COMBO_CHANGED,  GENERAL_CHANGED);
	HookWidget(ui->outputMode,           COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->simpleOutputPath,     EDIT_CHANGED,   OUTPUTS_CHANGED);
	HookWidget(ui->simpleOutputVBitrate, SCROLL_CHANGED, OUTPUTS_CHANGED);
	HookWidget(ui->simpleOutputABitrate, COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->simpleOutReconnect,   CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->simpleOutRetryDelay,  SCROLL_CHANGED, OUTPUTS_CHANGED);
	HookWidget(ui->simpleOutMaxRetries,  SCROLL_CHANGED, OUTPUTS_CHANGED);
	HookWidget(ui->simpleOutAdvanced,    CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->simpleOutUseCBR,      CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->simpleOutPreset,      COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->simpleOutUseBufsize,  CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->simpleOutPreset,      COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->simpleOutVBufsize,    SCROLL_CHANGED, OUTPUTS_CHANGED);
	HookWidget(ui->advOutReconnect,      CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutRetryDelay,     SCROLL_CHANGED, OUTPUTS_CHANGED);
	HookWidget(ui->advOutMaxRetries,     SCROLL_CHANGED, OUTPUTS_CHANGED);
	HookWidget(ui->advOutEncoder,        COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutUseRescale,     CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutRescale,        CBEDIT_CHANGED, OUTPUTS_CHANGED);
	HookWidget(ui->advOutTrack1,         CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutTrack2,         CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutTrack3,         CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutTrack4,         CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutRecType,        COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutRecPath,        EDIT_CHANGED,   OUTPUTS_CHANGED);
	HookWidget(ui->advOutRecEncoder,     COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutRecUseRescale,  CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutRecRescale,     CBEDIT_CHANGED, OUTPUTS_CHANGED);
	HookWidget(ui->advOutRecTrack1,      CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutRecTrack2,      CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutRecTrack3,      CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutRecTrack4,      CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFURL,          EDIT_CHANGED,   OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFVBitrate,     SCROLL_CHANGED, OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFUseRescale,   CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFRescale,      CBEDIT_CHANGED, OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFVEncoder,     EDIT_CHANGED,   OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFVCfg,         EDIT_CHANGED,   OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFABitrate,     SCROLL_CHANGED, OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFTrack1,       CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFTrack2,       CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFTrack3,       CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFTrack4,       CHECK_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFAEncoder,     EDIT_CHANGED,   OUTPUTS_CHANGED);
	HookWidget(ui->advOutFFACfg,         EDIT_CHANGED,   OUTPUTS_CHANGED);
	HookWidget(ui->advOutTrack1Bitrate,  COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutTrack1Name,     EDIT_CHANGED,   OUTPUTS_CHANGED);
	HookWidget(ui->advOutTrack2Bitrate,  COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutTrack2Name,     EDIT_CHANGED,   OUTPUTS_CHANGED);
	HookWidget(ui->advOutTrack3Bitrate,  COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutTrack3Name,     EDIT_CHANGED,   OUTPUTS_CHANGED);
	HookWidget(ui->advOutTrack4Bitrate,  COMBO_CHANGED,  OUTPUTS_CHANGED);
	HookWidget(ui->advOutTrack4Name,     EDIT_CHANGED,   OUTPUTS_CHANGED);
	HookWidget(ui->channelSetup,         COMBO_CHANGED,  AUDIO_RESTART);
	HookWidget(ui->sampleRate,           COMBO_CHANGED,  AUDIO_RESTART);
	HookWidget(ui->desktopAudioDevice1,  COMBO_CHANGED,  AUDIO_CHANGED);
	HookWidget(ui->desktopAudioDevice2,  COMBO_CHANGED,  AUDIO_CHANGED);
	HookWidget(ui->auxAudioDevice1,      COMBO_CHANGED,  AUDIO_CHANGED);
	HookWidget(ui->auxAudioDevice2,      COMBO_CHANGED,  AUDIO_CHANGED);
	HookWidget(ui->auxAudioDevice3,      COMBO_CHANGED,  AUDIO_CHANGED);
	HookWidget(ui->renderer,             COMBO_CHANGED,  VIDEO_RESTART);
	HookWidget(ui->adapter,              COMBO_CHANGED,  VIDEO_RESTART);
	HookWidget(ui->baseResolution,       CBEDIT_CHANGED, VIDEO_RES);
	HookWidget(ui->outputResolution,     CBEDIT_CHANGED, VIDEO_RES);
	HookWidget(ui->downscaleFilter,      COMBO_CHANGED,  VIDEO_CHANGED);
	HookWidget(ui->fpsType,              COMBO_CHANGED,  VIDEO_CHANGED);
	HookWidget(ui->fpsCommon,            COMBO_CHANGED,  VIDEO_CHANGED);
	HookWidget(ui->fpsInteger,           SCROLL_CHANGED, VIDEO_CHANGED);
	HookWidget(ui->fpsInteger,           SCROLL_CHANGED, VIDEO_CHANGED);
	HookWidget(ui->fpsNumerator,         SCROLL_CHANGED, VIDEO_CHANGED);
	HookWidget(ui->fpsDenominator,       SCROLL_CHANGED, VIDEO_CHANGED);

	//Apply button disabled until change.
	EnableApplyButton(false);

	LoadServiceTypes();
	LoadServiceInfo();
	LoadEncoderTypes();
	LoadSettings(false);
}

void OBSBasicSettings::SaveCombo(QComboBox *widget, const char *section,
		const char *value)
{
	if (WidgetChanged(widget))
		config_set_string(main->Config(), section, value,
				QT_TO_UTF8(widget->currentText()));
}

void OBSBasicSettings::SaveComboData(QComboBox *widget, const char *section,
		const char *value)
{
	if (WidgetChanged(widget)) {
		QString str = GetComboData(widget);
		config_set_string(main->Config(), section, value,
				QT_TO_UTF8(str));
	}
}

void OBSBasicSettings::SaveCheckBox(QAbstractButton *widget,
		const char *section, const char *value, bool invert)
{
	if (WidgetChanged(widget)) {
		bool checked = widget->isChecked();
		if (invert) checked = !checked;

		config_set_bool(main->Config(), section, value, checked);
	}
}

void OBSBasicSettings::SaveEdit(QLineEdit *widget, const char *section,
		const char *value)
{
	if (WidgetChanged(widget))
		config_set_string(main->Config(), section, value,
				QT_TO_UTF8(widget->text()));
}

void OBSBasicSettings::SaveSpinBox(QSpinBox *widget, const char *section,
		const char *value)
{
	if (WidgetChanged(widget))
		config_set_int(main->Config(), section, value, widget->value());
}

void OBSBasicSettings::LoadServiceTypes()
{
	const char    *type;
	size_t        idx = 0;

	while (obs_enum_service_types(idx++, &type)) {
		const char *name = obs_service_get_display_name(type);
		QString qName = QT_UTF8(name);
		QString qType = QT_UTF8(type);

		ui->streamType->addItem(qName, qType);
	}

	type = obs_service_gettype(main->GetService());
	SetComboByValue(ui->streamType, type);
}

void OBSBasicSettings::LoadServiceInfo()
{
	QLayout          *layout    = ui->streamContainer->layout();
	obs_service_t    *service    = main->GetService();
	obs_data_t       *settings   = obs_service_get_settings(service);

	delete streamProperties;
	streamProperties = new OBSPropertiesView(settings, service,
			(PropertiesReloadCallback)obs_service_properties,
			(PropertiesUpdateCallback)obs_service_update,
			170);

	layout->addWidget(streamProperties);

	obs_data_release(settings);
}

#define TEXT_USE_STREAM_ENC \
	QTStr("Basic.Settings.Output.Adv.Recording.UseStreamEncoder")

void OBSBasicSettings::LoadEncoderTypes()
{
	const char    *type;
	size_t        idx = 0;

	ui->advOutRecEncoder->addItem(TEXT_USE_STREAM_ENC, "none");

	while (obs_enum_encoder_types(idx++, &type)) {
		const char *name = obs_encoder_get_display_name(type);
		const char *codec = obs_get_encoder_codec(type);

		if (strcmp(codec, "h264") != 0)
			continue;

		QString qName = QT_UTF8(name);
		QString qType = QT_UTF8(type);

		ui->advOutEncoder->addItem(qName, qType);
		ui->advOutRecEncoder->addItem(qName, qType);
	}
}

void OBSBasicSettings::LoadLanguageList()
{
	const char *currentLang = App()->GetLocale();

	ui->language->clear();

	for (const auto &locale : GetLocaleNames()) {
		int idx = ui->language->count();

		ui->language->addItem(QT_UTF8(locale.second.c_str()),
				QT_UTF8(locale.first.c_str()));

		if (locale.first == currentLang)
			ui->language->setCurrentIndex(idx);
	}

	ui->language->model()->sort(0);
}

void OBSBasicSettings::LoadGeneralSettings()
{
	loading = true;

	LoadLanguageList();

	loading = false;
}

void OBSBasicSettings::LoadRendererList()
{
	const char *renderer = config_get_string(GetGlobalConfig(), "Video",
			"Renderer");

#ifdef _WIN32
	ui->renderer->addItem(QT_UTF8("Direct3D 11"));
#endif
	ui->renderer->addItem(QT_UTF8("OpenGL"));

	int idx = ui->renderer->findText(QT_UTF8(renderer));
	if (idx == -1)
		idx = 0;

	ui->renderer->setCurrentIndex(idx);
}

Q_DECLARE_METATYPE(MonitorInfo);

static string ResString(uint32_t cx, uint32_t cy)
{
	stringstream res;
	res << cx << "x" << cy;
	return res.str();
}

/* some nice default output resolution vals */
static const double vals[] =
{
	1.0,
	1.25,
	(1.0/0.75),
	1.5,
	(1.0/0.6),
	1.75,
	2.0,
	2.25,
	2.5,
	2.75,
	3.0
};

static const size_t numVals = sizeof(vals)/sizeof(double);

void OBSBasicSettings::ResetDownscales(uint32_t cx, uint32_t cy)
{
	QString advRescale;
	QString advRecRescale;
	QString advFFRescale;

	advRescale = ui->advOutRescale->lineEdit()->text();
	advRecRescale = ui->advOutRecRescale->lineEdit()->text();
	advFFRescale = ui->advOutFFRescale->lineEdit()->text();

	ui->outputResolution->clear();
	ui->advOutRescale->clear();
	ui->advOutRecRescale->clear();
	ui->advOutFFRescale->clear();

	for (size_t idx = 0; idx < numVals; idx++) {
		uint32_t downscaleCX = uint32_t(double(cx) / vals[idx]);
		uint32_t downscaleCY = uint32_t(double(cy) / vals[idx]);

		string res = ResString(downscaleCX, downscaleCY);
		ui->outputResolution->addItem(res.c_str());
		ui->advOutRescale->addItem(res.c_str());
		ui->advOutRecRescale->addItem(res.c_str());
		ui->advOutFFRescale->addItem(res.c_str());
	}

	string res = ResString(cx, cy);

	ui->outputResolution->lineEdit()->setText(res.c_str());

	if (advRescale.isEmpty())
		advRescale = res.c_str();
	if (advRecRescale.isEmpty())
		advRecRescale = res.c_str();
	if (advFFRescale.isEmpty())
		advFFRescale = res.c_str();

	ui->advOutRescale->lineEdit()->setText(advRescale);
	ui->advOutRecRescale->lineEdit()->setText(advRecRescale);
	ui->advOutFFRescale->lineEdit()->setText(advFFRescale);
}

void OBSBasicSettings::LoadDownscaleFilters()
{
	ui->downscaleFilter->addItem(
			QTStr("Basic.Settings.Video.DownscaleFilter.Bilinear"),
			QT_UTF8("bilinear"));
	ui->downscaleFilter->addItem(
			QTStr("Basic.Settings.Video.DownscaleFilter.Bicubic"),
			QT_UTF8("bicubic"));
	ui->downscaleFilter->addItem(
			QTStr("Basic.Settings.Video.DownscaleFilter.Lanczos"),
			QT_UTF8("lanczos"));

	const char *scaleType = config_get_string(main->Config(),
			"Video", "ScaleType");

	if (astrcmpi(scaleType, "bilinear") == 0)
		ui->downscaleFilter->setCurrentIndex(0);
	else if (astrcmpi(scaleType, "lanczos") == 0)
		ui->downscaleFilter->setCurrentIndex(2);
	else
		ui->downscaleFilter->setCurrentIndex(1);
}

void OBSBasicSettings::LoadResolutionLists()
{
	uint32_t cx = config_get_uint(main->Config(), "Video", "BaseCX");
	uint32_t cy = config_get_uint(main->Config(), "Video", "BaseCY");
	vector<MonitorInfo> monitors;

	ui->baseResolution->clear();

	GetMonitors(monitors);

	for (MonitorInfo &monitor : monitors) {
		string res = ResString(monitor.cx, monitor.cy);
		ui->baseResolution->addItem(res.c_str());
	}

	ResetDownscales(cx, cy);

	ui->baseResolution->lineEdit()->setText(ResString(cx, cy).c_str());

	cx = config_get_uint(main->Config(), "Video", "OutputCX");
	cy = config_get_uint(main->Config(), "Video", "OutputCY");

	ui->outputResolution->lineEdit()->setText(ResString(cx, cy).c_str());
}

static inline void LoadFPSCommon(OBSBasic *main, Ui::OBSBasicSettings *ui)
{
	const char *val = config_get_string(main->Config(), "Video",
			"FPSCommon");

	int idx = ui->fpsCommon->findText(val);
	if (idx == -1) idx = 3;
	ui->fpsCommon->setCurrentIndex(idx);
}

static inline void LoadFPSInteger(OBSBasic *main, Ui::OBSBasicSettings *ui)
{
	uint32_t val = config_get_uint(main->Config(), "Video", "FPSInt");
	ui->fpsInteger->setValue(val);
}

static inline void LoadFPSFraction(OBSBasic *main, Ui::OBSBasicSettings *ui)
{
	uint32_t num = config_get_uint(main->Config(), "Video", "FPSNum");
	uint32_t den = config_get_uint(main->Config(), "Video", "FPSDen");

	ui->fpsNumerator->setValue(num);
	ui->fpsDenominator->setValue(den);
}

void OBSBasicSettings::LoadFPSData()
{
	LoadFPSCommon(main, ui.get());
	LoadFPSInteger(main, ui.get());
	LoadFPSFraction(main, ui.get());

	uint32_t fpsType = config_get_uint(main->Config(), "Video",
			"FPSType");
	if (fpsType > 2) fpsType = 0;

	ui->fpsType->setCurrentIndex(fpsType);
	ui->fpsTypes->setCurrentIndex(fpsType);
}

void OBSBasicSettings::LoadVideoSettings()
{
	loading = true;

	if (video_output_active(obs_get_video())) {
		ui->videoPage->setEnabled(false);
		ui->videoMsg->setText(
				QTStr("Basic.Settings.Video.CurrentlyActive"));
	}

	LoadRendererList();
	LoadResolutionLists();
	LoadFPSData();
	LoadDownscaleFilters();

	loading = false;
}

void OBSBasicSettings::LoadSimpleOutputSettings()
{
	const char *path = config_get_string(main->Config(), "SimpleOutput",
			"FilePath");
	int videoBitrate = config_get_uint(main->Config(), "SimpleOutput",
			"VBitrate");
	int videoBufsize = config_get_uint(main->Config(), "SimpleOutput",
			"Bufsize");
	int audioBitrate = config_get_uint(main->Config(), "SimpleOutput",
			"ABitrate");
	bool reconnect = config_get_bool(main->Config(), "SimpleOutput",
			"Reconnect");
	int retryDelay = config_get_uint(main->Config(), "SimpleOutput",
			"RetryDelay");
	int maxRetries = config_get_uint(main->Config(), "SimpleOutput",
			"MaxRetries");
	bool advanced = config_get_bool(main->Config(), "SimpleOutput",
			"UseAdvanced");
	bool useCBR = config_get_bool(main->Config(), "SimpleOutput",
			"UseCBR");
	bool useBufsize = config_get_bool(main->Config(), "SimpleOutput",
			"UseBufsize");
	const char *preset = config_get_string(main->Config(), "SimpleOutput",
			"Preset");
	const char *custom = config_get_string(main->Config(), "SimpleOutput",
			"x264Settings");

	ui->simpleOutputPath->setText(path);
	ui->simpleOutputVBitrate->setValue(videoBitrate);
	ui->simpleOutUseBufsize->setChecked(useBufsize);
	ui->simpleOutVBufsize->setValue(
			useBufsize ? videoBufsize : videoBitrate);

	SetComboByName(ui->simpleOutputABitrate,
			std::to_string(audioBitrate).c_str());

	ui->simpleOutReconnect->setChecked(reconnect);
	ui->simpleOutRetryDelay->setValue(retryDelay);
	ui->simpleOutMaxRetries->setValue(maxRetries);
	ui->simpleOutAdvanced->setChecked(advanced);
	ui->simpleOutUseCBR->setChecked(useCBR);
	ui->simpleOutPreset->setCurrentText(preset);
	ui->simpleOutCustom->setText(custom);
}

void OBSBasicSettings::LoadAdvOutputStreamingSettings()
{
	bool reconnect = config_get_bool(main->Config(), "AdvOut",
			"Reconnect");
	int retryDelay = config_get_int(main->Config(), "AdvOut",
			"RetryDelay");
	int maxRetries = config_get_int(main->Config(), "AdvOut",
			"MaxRetries");
	bool rescale = config_get_bool(main->Config(), "AdvOut",
			"Rescale");
	const char *rescaleRes = config_get_string(main->Config(), "AdvOut",
			"RescaleRes");
	int trackIndex = config_get_int(main->Config(), "AdvOut",
			"TrackIndex");

	ui->advOutReconnect->setChecked(reconnect);
	ui->advOutRetryDelay->setValue(retryDelay);
	ui->advOutMaxRetries->setValue(maxRetries);
	ui->advOutUseRescale->setChecked(rescale);
	ui->advOutRescale->setCurrentText(rescaleRes);

	switch (trackIndex) {
	case 1: ui->advOutTrack1->setChecked(true); break;
	case 2: ui->advOutTrack2->setChecked(true); break;
	case 3: ui->advOutTrack3->setChecked(true); break;
	case 4: ui->advOutTrack4->setChecked(true); break;
	}
}

OBSPropertiesView *OBSBasicSettings::CreateEncoderPropertyView(
		const char *encoder, const char *path, bool changed)
{
	obs_data_t *settings = obs_encoder_defaults(encoder);
	OBSPropertiesView *view;

	char encoderJsonPath[512];
	int ret = os_get_config_path(encoderJsonPath, sizeof(encoderJsonPath),
			path);
	if (ret > 0) {
		BPtr<char> jsonData = os_quick_read_utf8_file(encoderJsonPath);
		if (!!jsonData) {
			obs_data_t *data = obs_data_create_from_json(jsonData);
			obs_data_apply(settings, data);
			obs_data_release(data);
		}
	}

	view = new OBSPropertiesView(settings, encoder,
			(PropertiesReloadCallback)obs_get_encoder_properties,
			170);
	view->setFrameShape(QFrame::StyledPanel);
	view->setProperty("changed", QVariant(changed));
	QObject::connect(view, SIGNAL(Changed()), this, SLOT(OutputsChanged()));

	obs_data_release(settings);
	return view;
}

void OBSBasicSettings::LoadAdvOutputStreamingEncoderProperties()
{
	const char *encoder = config_get_string(main->Config(), "AdvOut",
			"Encoder");

	delete streamEncoderProps;
	streamEncoderProps = CreateEncoderPropertyView(encoder,
			"obs-studio/basic/streamEncoder.json");
	ui->advOutputStreamTab->layout()->addWidget(streamEncoderProps);

	SetComboByValue(ui->advOutEncoder, encoder);
}

void OBSBasicSettings::LoadAdvOutputRecordingSettings()
{
	const char *type = config_get_string(main->Config(), "AdvOut",
			"RecType");
	const char *path = config_get_string(main->Config(), "AdvOut",
			"RecFilePath");
	bool rescale = config_get_bool(main->Config(), "AdvOut",
			"RecRescale");
	const char *rescaleRes = config_get_string(main->Config(), "AdvOut",
			"RecRescaleRes");
	int trackIndex = config_get_int(main->Config(), "AdvOut",
			"RecTrackIndex");

	int typeIndex = (astrcmpi(type, "FFmpeg") == 0) ? 1 : 0;
	ui->advOutRecType->setCurrentIndex(typeIndex);
	ui->advOutRecPath->setText(path);
	ui->advOutRecUseRescale->setChecked(rescale);
	ui->advOutRecRescale->setCurrentText(rescaleRes);

	switch (trackIndex) {
	case 1: ui->advOutRecTrack1->setChecked(true); break;
	case 2: ui->advOutRecTrack2->setChecked(true); break;
	case 3: ui->advOutRecTrack3->setChecked(true); break;
	case 4: ui->advOutRecTrack4->setChecked(true); break;
	}
}

void OBSBasicSettings::LoadAdvOutputRecordingEncoderProperties()
{
	const char *encoder = config_get_string(main->Config(), "AdvOut",
			"RecEncoder");

	delete recordEncoderProps;
	recordEncoderProps = nullptr;

	if (astrcmpi(encoder, "none") != 0) {
		recordEncoderProps = CreateEncoderPropertyView(encoder,
				"obs-studio/basic/recordEncoder.json");
		ui->advOutRecStandard->layout()->addWidget(recordEncoderProps);
	}

	SetComboByValue(ui->advOutRecEncoder, encoder);
}

void OBSBasicSettings::LoadAdvOutputFFmpegSettings()
{
	const char *url = config_get_string(main->Config(), "AdvOut", "FFURL");
	int videoBitrate = config_get_int(main->Config(), "AdvOut",
			"FFVBitrate");
	bool rescale = config_get_bool(main->Config(), "AdvOut",
			"FFRescale");
	const char *rescaleRes = config_get_string(main->Config(), "AdvOut",
			"FFRescaleRes");
	const char *vEncoder = config_get_string(main->Config(), "AdvOut",
			"FFVEncoder");
	const char *vEncCustom = config_get_string(main->Config(), "AdvOut",
			"FFVCustom");
	int audioBitrate = config_get_int(main->Config(), "AdvOut",
			"FFABitrate");
	int audioTrack = config_get_int(main->Config(), "AdvOut",
			"FFAudioTrack");
	const char *aEncoder = config_get_string(main->Config(), "AdvOut",
			"FFAEncoder");
	const char *aEncCustom = config_get_string(main->Config(), "AdvOut",
			"FFACustom");

	ui->advOutFFURL->setText(url);
	ui->advOutFFVBitrate->setValue(videoBitrate);
	ui->advOutFFUseRescale->setChecked(rescale);
	ui->advOutFFRescale->setCurrentText(rescaleRes);
	ui->advOutFFVEncoder->setText(vEncoder);
	ui->advOutFFVCfg->setText(vEncCustom);
	ui->advOutFFABitrate->setValue(audioBitrate);
	ui->advOutFFAEncoder->setText(aEncoder);
	ui->advOutFFACfg->setText(aEncCustom);

	switch (audioTrack) {
	case 1: ui->advOutFFTrack1->setChecked(true); break;
	case 2: ui->advOutFFTrack2->setChecked(true); break;
	case 3: ui->advOutFFTrack3->setChecked(true); break;
	case 4: ui->advOutFFTrack4->setChecked(true); break;
	}
}

void OBSBasicSettings::LoadAdvOutputAudioSettings()
{
	int track1Bitrate = config_get_uint(main->Config(), "AdvOut",
			"Track1Bitrate");
	int track2Bitrate = config_get_uint(main->Config(), "AdvOut",
			"Track2Bitrate");
	int track3Bitrate = config_get_uint(main->Config(), "AdvOut",
			"Track3Bitrate");
	int track4Bitrate = config_get_uint(main->Config(), "AdvOut",
			"Track4Bitrate");
	const char *name1 = config_get_string(main->Config(), "AdvOut",
			"Track1Name");
	const char *name2 = config_get_string(main->Config(), "AdvOut",
			"Track2Name");
	const char *name3 = config_get_string(main->Config(), "AdvOut",
			"Track3Name");
	const char *name4 = config_get_string(main->Config(), "AdvOut",
			"Track4Name");

	SetComboByName(ui->advOutTrack1Bitrate,
			std::to_string(track1Bitrate).c_str());
	SetComboByName(ui->advOutTrack2Bitrate,
			std::to_string(track2Bitrate).c_str());
	SetComboByName(ui->advOutTrack3Bitrate,
			std::to_string(track3Bitrate).c_str());
	SetComboByName(ui->advOutTrack4Bitrate,
			std::to_string(track4Bitrate).c_str());

	ui->advOutTrack1Name->setText(name1);
	ui->advOutTrack2Name->setText(name2);
	ui->advOutTrack3Name->setText(name3);
	ui->advOutTrack4Name->setText(name4);
}

void OBSBasicSettings::LoadOutputSettings()
{
	loading = true;

	const char *mode = config_get_string(main->Config(), "Output", "Mode");

	int modeIdx = astrcmpi(mode, "Advanced") == 0 ? 1 : 0;
	ui->outputMode->setCurrentIndex(modeIdx);

	LoadSimpleOutputSettings();
	LoadAdvOutputStreamingSettings();
	LoadAdvOutputStreamingEncoderProperties();
	LoadAdvOutputRecordingSettings();
	LoadAdvOutputRecordingEncoderProperties();
	LoadAdvOutputFFmpegSettings();
	LoadAdvOutputAudioSettings();

	if (video_output_active(obs_get_video())) {
		ui->outputMode->setEnabled(false);
		ui->advOutTopContainer->setEnabled(false);
		ui->advOutRecTopContainer->setEnabled(false);
		ui->advOutRecTypeContainer->setEnabled(false);
		ui->advOutputAudioTracksTab->setEnabled(false);
	}

	loading = false;
}

static inline void LoadListValue(QComboBox *widget, const char *text,
		const char *val)
{
	widget->addItem(QT_UTF8(text), QT_UTF8(val));
}

void OBSBasicSettings::LoadListValues(QComboBox *widget, obs_property_t *prop,
		const char *configName)
{
	size_t count = obs_property_list_item_count(prop);
	const char *deviceId = config_get_string(main->Config(), "Audio",
			configName);

	widget->addItem(QTStr("Disabled"), "disabled");

	for (size_t i = 0; i < count; i++) {
		const char *name = obs_property_list_item_name(prop, i);
		const char *val  = obs_property_list_item_string(prop, i);
		LoadListValue(widget, name, val);
	}

	int idx = widget->findData(QVariant(QT_UTF8(deviceId)));
	if (idx == -1) {
		deviceId = config_get_default_string(main->Config(), "Audio",
				configName);
		idx = widget->findData(QVariant(QT_UTF8(deviceId)));
	}

	if (idx != -1)
		widget->setCurrentIndex(idx);
}

void OBSBasicSettings::LoadAudioDevices()
{
	const char *input_id  = App()->InputAudioSource();
	const char *output_id = App()->OutputAudioSource();

	obs_properties_t *input_props = obs_get_source_properties(
			OBS_SOURCE_TYPE_INPUT, input_id);
	obs_properties_t *output_props = obs_get_source_properties(
			OBS_SOURCE_TYPE_INPUT, output_id);

	if (input_props) {
		obs_property_t *inputs  = obs_properties_get(input_props,
				"device_id");
		LoadListValues(ui->auxAudioDevice1, inputs, "AuxDevice1");
		LoadListValues(ui->auxAudioDevice2, inputs, "AuxDevice2");
		LoadListValues(ui->auxAudioDevice3, inputs, "AuxDevice3");
		obs_properties_destroy(input_props);
	}

	if (output_props) {
		obs_property_t *outputs = obs_properties_get(output_props,
				"device_id");
		LoadListValues(ui->desktopAudioDevice1, outputs,
				"DesktopDevice1");
		LoadListValues(ui->desktopAudioDevice2, outputs,
				"DesktopDevice2");
		obs_properties_destroy(output_props);
	}
}

void OBSBasicSettings::LoadAudioSettings()
{
	uint32_t sampleRate = config_get_uint(main->Config(), "Audio",
			"SampleRate");
	const char *speakers = config_get_string(main->Config(), "Audio",
			"ChannelSetup");

	loading = true;

	const char *str;
	if (sampleRate == 22050)
		str = "22.05khz";
	else if (sampleRate == 48000)
		str = "48khz";
	else
		str = "44.1khz";

	int sampleRateIdx = ui->sampleRate->findText(str);
	if (sampleRateIdx != -1)
		ui->sampleRate->setCurrentIndex(sampleRateIdx);

	if (strcmp(speakers, "Mono") == 0)
		ui->channelSetup->setCurrentIndex(0);
	else
		ui->channelSetup->setCurrentIndex(1);

	LoadAudioDevices();

	loading = false;
}

void OBSBasicSettings::LoadSettings(bool changedOnly)
{
	if (!changedOnly || generalChanged)
		LoadGeneralSettings();
	if (!changedOnly || outputsChanged)
		LoadOutputSettings();
	if (!changedOnly || audioChanged)
		LoadAudioSettings();
	if (!changedOnly || videoChanged)
		LoadVideoSettings();
}

void OBSBasicSettings::SaveGeneralSettings()
{
	int languageIndex = ui->language->currentIndex();
	QVariant langData = ui->language->itemData(languageIndex);
	string language = langData.toString().toStdString();

	if (WidgetChanged(ui->language))
		config_set_string(GetGlobalConfig(), "General", "Language",
				language.c_str());
}

void OBSBasicSettings::SaveVideoSettings()
{
	QString baseResolution   = ui->baseResolution->currentText();
	QString outputResolution = ui->outputResolution->currentText();
	int     fpsType          = ui->fpsType->currentIndex();
	uint32_t cx = 0, cy = 0;

	/* ------------------- */

	if (WidgetChanged(ui->renderer))
		config_set_string(App()->GlobalConfig(), "Video", "Renderer",
				QT_TO_UTF8(ui->renderer->currentText()));

	if (WidgetChanged(ui->baseResolution) &&
	    ConvertResText(QT_TO_UTF8(baseResolution), cx, cy)) {
		config_set_uint(main->Config(), "Video", "BaseCX", cx);
		config_set_uint(main->Config(), "Video", "BaseCY", cy);
	}

	if (WidgetChanged(ui->outputResolution) &&
	    ConvertResText(QT_TO_UTF8(outputResolution), cx, cy)) {
		config_set_uint(main->Config(), "Video", "OutputCX", cx);
		config_set_uint(main->Config(), "Video", "OutputCY", cy);
	}

	if (WidgetChanged(ui->fpsType))
		config_set_uint(main->Config(), "Video", "FPSType", fpsType);

	SaveCombo(ui->fpsCommon, "Video", "FPSCommon");
	SaveSpinBox(ui->fpsInteger, "Video", "FPSInt");
	SaveSpinBox(ui->fpsNumerator, "Video", "FPSNum");
	SaveSpinBox(ui->fpsDenominator, "Video", "FPSDen");
	SaveComboData(ui->downscaleFilter, "Video", "ScaleType");

	main->ResetVideo();
}

static inline const char *OutputModeFromIdx(int idx)
{
	if (idx == 1)
		return "Advanced";
	else
		return "Simple";
}

static inline const char *RecTypeFromIdx(int idx)
{
	if (idx == 1)
		return "FFmpeg";
	else
		return "Standard";
}

static void WriteJsonData(OBSPropertiesView *view, const char *path)
{
	char full_path[512];

	if (!view || !WidgetChanged(view))
		return;

	int ret = os_get_config_path(full_path, sizeof(full_path), path);
	if (ret > 0) {
		obs_data_t *settings = view->GetSettings();
		if (settings) {
			const char *json = obs_data_get_json(settings);
			if (json && *json) {
				os_quick_write_utf8_file(full_path, json,
						strlen(json), false);
			}
		}
	}
}

static void SaveTrackIndex(config_t *config, const char *section,
		const char *name,
		QAbstractButton *check1,
		QAbstractButton *check2,
		QAbstractButton *check3,
		QAbstractButton *check4)
{
	if (check1->isChecked()) config_set_int(config, section, name, 1);
	else if (check2->isChecked()) config_set_int(config, section, name, 2);
	else if (check3->isChecked()) config_set_int(config, section, name, 3);
	else if (check4->isChecked()) config_set_int(config, section, name, 4);
}

void OBSBasicSettings::SaveOutputSettings()
{
	config_set_string(main->Config(), "Output", "Mode",
			OutputModeFromIdx(ui->outputMode->currentIndex()));

	SaveSpinBox(ui->simpleOutputVBitrate, "SimpleOutput", "VBitrate");
	SaveCombo(ui->simpleOutputABitrate, "SimpleOutput", "ABitrate");
	SaveEdit(ui->simpleOutputPath, "SimpleOutput", "FilePath");
	SaveCheckBox(ui->simpleOutReconnect, "SimpleOutput", "Reconnect");
	SaveSpinBox(ui->simpleOutRetryDelay, "SimpleOutput", "RetryDelay");
	SaveSpinBox(ui->simpleOutMaxRetries, "SimpleOutput", "MaxRetries");
	SaveCheckBox(ui->simpleOutAdvanced, "SimpleOutput", "UseAdvanced");
	SaveCheckBox(ui->simpleOutUseCBR, "SimpleOutput", "UseCBR");
	SaveCheckBox(ui->simpleOutUseBufsize, "SimpleOutput", "UseBufsize");
	SaveCombo(ui->simpleOutPreset, "SimpleOutput", "Preset");
	SaveEdit(ui->simpleOutCustom, "SimpleOutput", "x264Settings");

	if (ui->simpleOutUseBufsize->isChecked())
		SaveSpinBox(ui->simpleOutVBufsize, "SimpleOutput", "VBufsize");

	SaveCheckBox(ui->advOutReconnect, "AdvOut", "Reconnect");
	SaveSpinBox(ui->advOutRetryDelay, "AdvOut", "RetryDelay");
	SaveSpinBox(ui->advOutMaxRetries, "AdvOut", "MaxRetries");
	SaveComboData(ui->advOutEncoder, "AdvOut", "Encoder");
	SaveCheckBox(ui->advOutUseRescale, "AdvOut", "Rescale");
	SaveCombo(ui->advOutRescale, "AdvOut", "RescaleRes");
	SaveTrackIndex(main->Config(), "AdvOut", "TrackIndex",
			ui->advOutTrack1, ui->advOutTrack2,
			ui->advOutTrack3, ui->advOutTrack4);

	config_set_string(main->Config(), "AdvOut", "RecType",
			RecTypeFromIdx(ui->advOutRecType->currentIndex()));

	SaveEdit(ui->advOutRecPath, "AdvOut", "RecFilePath");
	SaveComboData(ui->advOutRecEncoder, "AdvOut", "RecEncoder");
	SaveCheckBox(ui->advOutRecUseRescale, "AdvOut", "RecRescale");
	SaveCombo(ui->advOutRecRescale, "AdvOut", "RecRescaleRes");
	SaveTrackIndex(main->Config(), "AdvOut", "RecTrackIndex",
			ui->advOutRecTrack1, ui->advOutRecTrack2,
			ui->advOutRecTrack3, ui->advOutRecTrack4);

	SaveEdit(ui->advOutFFURL, "AdvOut", "FFURL");
	SaveSpinBox(ui->advOutFFVBitrate, "AdvOut", "FFVBitrate");
	SaveCheckBox(ui->advOutFFUseRescale, "AdvOut", "FFRescale");
	SaveCombo(ui->advOutFFRescale, "AdvOut", "FFRescaleRes");
	SaveEdit(ui->advOutFFVEncoder, "AdvOut", "FFVEncoder");
	SaveEdit(ui->advOutFFVCfg, "AdvOut", "FFVCustom");
	SaveSpinBox(ui->advOutFFABitrate, "AdvOut", "FFABitrate");
	SaveEdit(ui->advOutFFAEncoder, "AdvOut", "FFAEncoder");
	SaveEdit(ui->advOutFFACfg, "AdvOut", "FFACustom");
	SaveTrackIndex(main->Config(), "AdvOut", "FFAudioTrack",
			ui->advOutFFTrack1, ui->advOutFFTrack2,
			ui->advOutFFTrack3, ui->advOutFFTrack4);

	SaveCombo(ui->advOutTrack1Bitrate, "AdvOut", "Track1Bitrate");
	SaveCombo(ui->advOutTrack2Bitrate, "AdvOut", "Track2Bitrate");
	SaveCombo(ui->advOutTrack3Bitrate, "AdvOut", "Track3Bitrate");
	SaveCombo(ui->advOutTrack4Bitrate, "AdvOut", "Track4Bitrate");
	SaveEdit(ui->advOutTrack1Name, "AdvOut", "Track1Name");
	SaveEdit(ui->advOutTrack2Name, "AdvOut", "Track2Name");
	SaveEdit(ui->advOutTrack3Name, "AdvOut", "Track3Name");
	SaveEdit(ui->advOutTrack4Name, "AdvOut", "Track4Name");

	WriteJsonData(streamEncoderProps,
			"obs-studio/basic/streamEncoder.json");
	WriteJsonData(recordEncoderProps,
			"obs-studio/basic/recordEncoder.json");
	main->ResetOutputs();
}

void OBSBasicSettings::SaveAudioSettings()
{
	QString sampleRateStr  = ui->sampleRate->currentText();
	int channelSetupIdx    = ui->channelSetup->currentIndex();

	const char *channelSetup = (channelSetupIdx == 0) ? "Mono" : "Stereo";

	int sampleRate = 44100;
	if (sampleRateStr == "22.05khz")
		sampleRate = 22050;
	else if (sampleRateStr == "48khz")
		sampleRate = 48000;

	if (WidgetChanged(ui->sampleRate))
		config_set_uint(main->Config(), "Audio", "SampleRate",
				sampleRate);

	if (WidgetChanged(ui->channelSetup))
		config_set_string(main->Config(), "Audio", "ChannelSetup",
				channelSetup);

	SaveComboData(ui->desktopAudioDevice1, "Audio", "DesktopDevice1");
	SaveComboData(ui->desktopAudioDevice2, "Audio", "DesktopDevice2");
	SaveComboData(ui->auxAudioDevice1, "Audio", "AuxDevice1");
	SaveComboData(ui->auxAudioDevice2, "Audio", "AuxDevice2");
	SaveComboData(ui->auxAudioDevice3, "Audio", "AuxDevice3");

	main->ResetAudioDevices();
}

void OBSBasicSettings::SaveSettings()
{
	if (generalChanged)
		SaveGeneralSettings();
	if (outputsChanged)
		SaveOutputSettings();
	if (audioChanged)
		SaveAudioSettings();
	if (videoChanged)
		SaveVideoSettings();

	config_save(main->Config());
	config_save(GetGlobalConfig());
}

bool OBSBasicSettings::QueryChanges()
{
	QMessageBox::StandardButton button;

	button = QMessageBox::question(this,
			QTStr("Basic.Settings.ConfirmTitle"),
			QTStr("Basic.Settings.Confirm"),
			QMessageBox::Yes | QMessageBox::No |
			QMessageBox::Cancel);

	if (button == QMessageBox::Cancel)
		return false;
	else if (button == QMessageBox::Yes)
		SaveSettings();
	else
		LoadSettings(true);

	ClearChanged();
	return true;
}

void OBSBasicSettings::closeEvent(QCloseEvent *event)
{
	if (Changed() && !QueryChanges())
		event->ignore();
}

void OBSBasicSettings::on_simpleOutUseBufsize_toggled(bool checked)
{
	if (!checked)
		ui->simpleOutVBufsize->setValue(
				ui->simpleOutputVBitrate->value());
}

void OBSBasicSettings::on_simpleOutputVBitrate_valueChanged(int val)
{
	if (!ui->simpleOutUseBufsize->isChecked())
		ui->simpleOutVBufsize->setValue(val);
}

void OBSBasicSettings::on_listWidget_itemSelectionChanged()
{
	int row = ui->listWidget->currentRow();

	if (loading || row == pageIndex)
		return;

	pageIndex = row;
}

void OBSBasicSettings::on_buttonBox_clicked(QAbstractButton *button)
{
	QDialogButtonBox::ButtonRole val = ui->buttonBox->buttonRole(button);

	if (val == QDialogButtonBox::ApplyRole ||
	    val == QDialogButtonBox::AcceptRole) {
		SaveSettings();
		ClearChanged();
	}

	if (val == QDialogButtonBox::AcceptRole ||
	    val == QDialogButtonBox::RejectRole) {
		ClearChanged();
		close();
	}
}

void OBSBasicSettings::on_streamType_currentIndexChanged(int idx)
{
	QString val = ui->streamType->itemData(idx).toString();
	obs_service_t *newService;

	if (loading)
		return;

	delete streamProperties;
	streamProperties = nullptr;

	newService = obs_service_create(QT_TO_UTF8(val), nullptr, nullptr);
	if (newService)
		main->SetService(newService);

	LoadServiceInfo();
}

void OBSBasicSettings::on_simpleOutputBrowse_clicked()
{
	QString dir = QFileDialog::getExistingDirectory(this,
			QTStr("Basic.Settings.Output.SelectDirectory"),
			ui->simpleOutputPath->text(),
			QFileDialog::ShowDirsOnly |
			QFileDialog::DontResolveSymlinks);
	if (dir.isEmpty())
		return;

	ui->simpleOutputPath->setText(dir);
}

void OBSBasicSettings::on_advOutRecPathBrowse_clicked()
{
	QString dir = QFileDialog::getExistingDirectory(this,
			QTStr("Basic.Settings.Output.SelectDirectory"),
			ui->advOutRecPath->text(),
			QFileDialog::ShowDirsOnly |
			QFileDialog::DontResolveSymlinks);
	if (dir.isEmpty())
		return;

	ui->advOutRecPath->setText(dir);
}

void OBSBasicSettings::on_advOutFFPathBrowse_clicked()
{
	QString file = QFileDialog::getSaveFileName(this,
			QTStr("Basic.Settings.Output.SelectFile"),
			ui->simpleOutputPath->text(),
			QTStr("Basic.Settings.Output.Adv.FFmpeg.SaveFilter"));
	if (file.isEmpty())
		return;

	ui->advOutFFURL->setText(file);
}

void OBSBasicSettings::on_advOutEncoder_currentIndexChanged(int idx)
{
	QString encoder = GetComboData(ui->advOutEncoder);

	delete streamEncoderProps;
	streamEncoderProps = CreateEncoderPropertyView(QT_TO_UTF8(encoder),
			"obs-studio/basic/streamEncoder.json", true);
	ui->advOutputStreamTab->layout()->addWidget(streamEncoderProps);

	UNUSED_PARAMETER(idx);
}

void OBSBasicSettings::on_advOutRecEncoder_currentIndexChanged(int idx)
{
	ui->advOutRecUseRescale->setEnabled(idx > 0);
	ui->advOutRecRescaleContainer->setEnabled(idx > 0);

	delete recordEncoderProps;
	recordEncoderProps = nullptr;

	if (idx > 0) {
		QString encoder = GetComboData(ui->advOutRecEncoder);

		recordEncoderProps = CreateEncoderPropertyView(
				QT_TO_UTF8(encoder),
				"obs-studio/basic/recordEncoder.json", true);
		ui->advOutRecStandard->layout()->addWidget(recordEncoderProps);
	}
}

static inline bool StreamExists(const char *name)
{
	return obs_get_service_by_name(name) != nullptr;
}

#define INVALID_RES_STR "Basic.Settings.Video.InvalidResolution"

static bool ValidResolutions(Ui::OBSBasicSettings *ui)
{
	QString baseRes   = ui->baseResolution->lineEdit()->text();
	QString outputRes = ui->outputResolution->lineEdit()->text();
	uint32_t cx, cy;

	if (!ConvertResText(QT_TO_UTF8(baseRes), cx, cy) ||
	    !ConvertResText(QT_TO_UTF8(outputRes), cx, cy)) {

		ui->videoMsg->setText(QTStr(INVALID_RES_STR));
		return false;
	}

	ui->videoMsg->setText("");
	return true;
}

void OBSBasicSettings::on_baseResolution_editTextChanged(const QString &text)
{
	if (!loading && ValidResolutions(ui.get())) {
		QString baseResolution = text;
		uint32_t cx, cy;

		ConvertResText(QT_TO_UTF8(baseResolution), cx, cy);
		ResetDownscales(cx, cy);
	}
}

void OBSBasicSettings::GeneralChanged()
{
	if (!loading) {
		generalChanged = true;
		sender()->setProperty("changed", QVariant(true));
		EnableApplyButton(true);
	}
}

void OBSBasicSettings::OutputsChanged()
{
	if (!loading) {
		outputsChanged = true;
		sender()->setProperty("changed", QVariant(true));
		EnableApplyButton(true);
	}
}

void OBSBasicSettings::AudioChanged()
{
	if (!loading) {
		audioChanged = true;
		sender()->setProperty("changed", QVariant(true));
		EnableApplyButton(true);
	}
}

void OBSBasicSettings::AudioChangedRestart()
{
	if (!loading) {
		audioChanged = true;
		ui->audioMsg->setText(QTStr("Basic.Settings.ProgramRestart"));
		sender()->setProperty("changed", QVariant(true));
		EnableApplyButton(true);
	}
}

void OBSBasicSettings::VideoChangedRestart()
{
	if (!loading) {
		videoChanged = true;
		ui->videoMsg->setText(QTStr("Basic.Settings.ProgramRestart"));
		sender()->setProperty("changed", QVariant(true));
		EnableApplyButton(true);
	}
}

void OBSBasicSettings::VideoChangedResolution()
{
	if (!loading && ValidResolutions(ui.get())) {
		videoChanged = true;
		sender()->setProperty("changed", QVariant(true));
		EnableApplyButton(true);
	}
}

void OBSBasicSettings::VideoChanged()
{
	if (!loading) {
		videoChanged = true;
		sender()->setProperty("changed", QVariant(true));
		EnableApplyButton(true);
	}
}
