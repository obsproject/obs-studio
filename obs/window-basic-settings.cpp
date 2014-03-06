/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

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

#include "obs-app.hpp"
#include "platform.hpp"
#include "qt-wrappers.hpp"
#include "window-basic-main.hpp"
#include "window-basic-settings.hpp"

#include <util/platform.h>

using namespace std;

struct BaseLexer {
	lexer lex;
public:
	inline BaseLexer() {lexer_init(&lex);}
	inline ~BaseLexer() {lexer_free(&lex);}
	operator lexer*() {return &lex;}
};

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

OBSBasicSettings::OBSBasicSettings(QWidget *parent)
	: QDialog        (parent),
	  ui             (new Ui::OBSBasicSettings),
	  generalChanged (false),
	  outputsChanged (false),
	  audioChanged   (false),
	  videoChanged   (false),
	  pageIndex      (0),
	  loading        (true)
{
	string path;

	ui->setupUi(this);

	if (!GetDataFilePath("locale/locale.ini", path))
		throw "Could not find locale/locale.ini path";
	if (localeIni.Open(path.c_str(), CONFIG_OPEN_EXISTING) != 0)
		throw "Could not open locale.ini";

	LoadSettings(false);
}

void OBSBasicSettings::LoadLanguageList()
{
	const char *currentLang = config_get_string(GetGlobalConfig(),
			"General", "Language");

	ui->language->clear();

	size_t numSections = config_num_sections(localeIni);

	for (size_t i = 0; i < numSections; i++) {
		const char *tag = config_get_section(localeIni, i);
		const char *name = config_get_string(localeIni, tag, "Name");
		int idx = ui->language->count();

		ui->language->addItem(QT_UTF8(name), QT_UTF8(tag));

		if (strcmp(tag, currentLang) == 0)
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
	ui->outputResolution->clear();

	for (size_t idx = 0; idx < numVals; idx++) {
		uint32_t downscaleCX = uint32_t(double(cx) / vals[idx]);
		uint32_t downscaleCY = uint32_t(double(cy) / vals[idx]);

		string res = ResString(downscaleCX, downscaleCY);
		ui->outputResolution->addItem(res.c_str());
	}

	ui->outputResolution->lineEdit()->setText(ResString(cx, cy).c_str());
}

void OBSBasicSettings::LoadResolutionLists()
{
	uint32_t cx = config_get_uint(GetGlobalConfig(), "Video", "BaseCX");
	uint32_t cy = config_get_uint(GetGlobalConfig(), "Video", "BaseCY");
	vector<MonitorInfo> monitors;

	ui->baseResolution->clear();

	GetMonitors(monitors);

	for (MonitorInfo &monitor : monitors) {
		string res = ResString(monitor.cx, monitor.cy);
		ui->baseResolution->addItem(res.c_str());
	}

	ResetDownscales(cx, cy);

	ui->baseResolution->lineEdit()->setText(ResString(cx, cy).c_str());

	cx = config_get_uint(GetGlobalConfig(), "Video", "OutputCX");
	cy = config_get_uint(GetGlobalConfig(), "Video", "OutputCY");

	ui->outputResolution->lineEdit()->setText(ResString(cx, cy).c_str());
}

static inline void LoadFPSCommon(Ui::OBSBasicSettings *ui)
{
	const char *val = config_get_string(GetGlobalConfig(), "Video",
			"FPSCommon");

	int idx = ui->fpsCommon->findText(val);
	if (idx == -1) idx = 3;
	ui->fpsCommon->setCurrentIndex(idx);
}

static inline void LoadFPSInteger(Ui::OBSBasicSettings *ui)
{
	uint32_t val = config_get_uint(GetGlobalConfig(), "Video", "FPSInt");
	ui->fpsInteger->setValue(val);
}

static inline void LoadFPSFraction(Ui::OBSBasicSettings *ui)
{
	uint32_t num = config_get_uint(GetGlobalConfig(), "Video", "FPSNum");
	uint32_t den = config_get_uint(GetGlobalConfig(), "Video", "FPSDen");

	ui->fpsNumerator->setValue(num);
	ui->fpsDenominator->setValue(den);
}

void OBSBasicSettings::LoadFPSData()
{
	LoadFPSCommon(ui.get());
	LoadFPSInteger(ui.get());
	LoadFPSFraction(ui.get());

	uint32_t fpsType = config_get_uint(GetGlobalConfig(), "Video",
			"FPSType");
	if (fpsType > 2) fpsType = 0;

	ui->fpsType->setCurrentIndex(fpsType);
	ui->fpsTypes->setCurrentIndex(fpsType);
}

void OBSBasicSettings::LoadVideoSettings()
{
	loading = true;

	if (video_output_active(obs_video())) {
		ui->videoPage->setEnabled(false);
		ui->videoMsg->setText(QTStr("Settings.Video.CurrentlyActive"));
	}

	LoadRendererList();
	LoadResolutionLists();
	LoadFPSData();

	loading = false;
}

static inline void LoadListValue(QComboBox *widget, const char *text,
		const char *val)
{
	widget->addItem(QT_UTF8(text), QT_UTF8(val));
}

static void LoadListValues(QComboBox *widget, obs_property_t prop)
{
	size_t count = obs_property_list_item_count(prop);

	widget->addItem(QTStr("Disabled"), "disabled");

	for (size_t i = 0; i < count; i++) {
		const char *name = obs_property_list_item_name(prop, i);
		const char *val  = obs_property_list_item_value(prop, i);
		LoadListValue(widget, name, val);
	}
}

void OBSBasicSettings::LoadAudioDevices()
{
#ifdef __APPLE__
	const char *input_id  = "coreaudio_input_capture";
	const char *output_id = "coreaudio_output_capture";
#elif _WIN32
	const char *input_id  = "wasapi_input_capture";
	const char *output_id = "wasapi_output_capture";
#else
	return;
#endif

	obs_properties_t input_props = obs_source_properties(
			OBS_SOURCE_TYPE_INPUT, input_id, App()->GetLocale());
	obs_properties_t output_props = obs_source_properties(
			OBS_SOURCE_TYPE_INPUT, output_id, App()->GetLocale());

	obs_property_t inputs = obs_properties_get(input_props, "device_id");
	obs_property_t outputs = obs_properties_get(output_props, "device_id");

	LoadListValues(ui->desktopAudioDevice1, outputs);
	LoadListValues(ui->desktopAudioDevice2, outputs);
	LoadListValues(ui->auxAudioDevice1,     inputs);
	LoadListValues(ui->auxAudioDevice2,     inputs);
	LoadListValues(ui->auxAudioDevice3,     inputs);

	obs_properties_destroy(input_props);
	obs_properties_destroy(output_props);
}

void OBSBasicSettings::LoadAudioSettings()
{
	uint32_t sampleRate = config_get_uint(GetGlobalConfig(), "Audio",
			"SampleRate");
	const char *speakers = config_get_string(GetGlobalConfig(), "Audio",
			"ChannelSetup");
	uint32_t bufferingTime = config_get_uint(GetGlobalConfig(), "Audio",
			"BufferingTime");

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

	ui->audioBufferingTime->setValue(bufferingTime);

	LoadAudioDevices();

	loading = false;
}

void OBSBasicSettings::LoadSettings(bool changedOnly)
{
	if (!changedOnly || generalChanged)
		LoadGeneralSettings();
	//if (!changedOnly || outputChanged)
	//	LoadOutputSettings();
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

	config_set_string(GetGlobalConfig(), "General", "Language",
			language.c_str());
}

void OBSBasicSettings::SaveVideoSettings()
{
	QString renderer         = ui->renderer->currentText();
	QString baseResolution   = ui->baseResolution->currentText();
	QString outputResolution = ui->outputResolution->currentText();
	int     fpsType          = ui->fpsType->currentIndex();
	QString fpsCommon        = ui->fpsCommon->currentText();
	int     fpsInteger       = ui->fpsInteger->value();
	int     fpsNumerator     = ui->fpsNumerator->value();
	int     fpsDenominator   = ui->fpsDenominator->value();
	uint32_t cx, cy;

	/* ------------------- */

	config_set_string(GetGlobalConfig(), "Video", "Renderer",
			QT_TO_UTF8(renderer));

	if (ConvertResText(QT_TO_UTF8(baseResolution), cx, cy)) {
		config_set_uint(GetGlobalConfig(), "Video", "BaseCX", cx);
		config_set_uint(GetGlobalConfig(), "Video", "BaseCY", cy);
	}

	if (ConvertResText(QT_TO_UTF8(outputResolution), cx, cy)) {
		config_set_uint(GetGlobalConfig(), "Video", "OutputCX", cx);
		config_set_uint(GetGlobalConfig(), "Video", "OutputCY", cy);
	}

	config_set_uint(GetGlobalConfig(), "Video", "FPSType", fpsType);
	config_set_string(GetGlobalConfig(), "Video", "FPSCommon",
			QT_TO_UTF8(fpsCommon));
	config_set_uint(GetGlobalConfig(), "Video", "FPSInt", fpsInteger);
	config_set_uint(GetGlobalConfig(), "Video", "FPSNum", fpsNumerator);
	config_set_uint(GetGlobalConfig(), "Video", "FPSDen", fpsDenominator);

	OBSBasic *window = qobject_cast<OBSBasic*>(parent());
	if (window)
		window->ResetVideo();
}

void OBSBasicSettings::SaveAudioSettings()
{
	QString sampleRateStr = ui->sampleRate->currentText();
	int channelSetupIdx = ui->channelSetup->currentIndex();
	int bufferingTime = ui->audioBufferingTime->value();

	const char *channelSetup;
	if (channelSetupIdx == 0)
		channelSetup = "Mono";
	else
		channelSetup = "Stereo";

	int sampleRate = 44100;
	if (sampleRateStr == "22.05khz")
		sampleRate = 22050;
	else if (sampleRateStr == "48khz")
		sampleRate = 48000;

	config_set_uint(GetGlobalConfig(), "Audio", "SampleRate", sampleRate);
	config_set_string(GetGlobalConfig(), "Audio", "ChannelSetup",
			channelSetup);
	config_set_uint(GetGlobalConfig(), "Audio", "BufferingTime",
			bufferingTime);
}

void OBSBasicSettings::SaveSettings()
{
	if (generalChanged)
		SaveGeneralSettings();
	//if (outputChanged)
	//	SaveOutputSettings();
	if (audioChanged)
		SaveAudioSettings();
	if (videoChanged)
		SaveVideoSettings();

	config_save(GetGlobalConfig());
}

bool OBSBasicSettings::QueryChanges()
{
	QMessageBox::StandardButton button;

	button = QMessageBox::question(this,
			QTStr("Settings.ConfirmTitle"),
			QTStr("Settings.Confirm"),
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

void OBSBasicSettings::on_listWidget_itemSelectionChanged()
{
	int row = ui->listWidget->currentRow();

	if (loading || row == pageIndex)
		return;

	if (Changed() && !QueryChanges()) {
		ui->listWidget->setCurrentRow(pageIndex);
		return;
	}

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

static bool ValidResolutions(Ui::OBSBasicSettings *ui)
{
	QString baseRes   = ui->baseResolution->lineEdit()->text();
	QString outputRes = ui->outputResolution->lineEdit()->text();
	uint32_t cx, cy;

	if (!ConvertResText(QT_TO_UTF8(baseRes), cx, cy) ||
	    !ConvertResText(QT_TO_UTF8(outputRes), cx, cy)) {

		ui->videoMsg->setText(
				QTStr("Settings.Video.InvalidResolution"));
		return false;
	}

	ui->videoMsg->setText("");
	return true;
}

void OBSBasicSettings::on_language_currentIndexChanged(int index)
{
	if (!loading)
		generalChanged = true;

	UNUSED_PARAMETER(index);
}

void OBSBasicSettings::on_sampleRate_currentIndexChanged(int index)
{
	if (!loading) {
		audioChanged = true;
		ui->audioMsg->setText(QTStr("Settings.ProgramRestart"));
	}

	UNUSED_PARAMETER(index);
}

void OBSBasicSettings::on_channelSetup_currentIndexChanged(int index)
{
	if (!loading) {
		audioChanged = true;
		ui->audioMsg->setText(QTStr("Settings.ProgramRestart"));
	}

	UNUSED_PARAMETER(index);
}

void OBSBasicSettings::on_audioBufferingTime_valueChanged(int value)
{
	if (!loading) {
		audioChanged = true;
		ui->audioMsg->setText(QTStr("Settings.ProgramRestart"));
	}

	UNUSED_PARAMETER(value);
}

void OBSBasicSettings::on_renderer_currentIndexChanged(int index)
{
	if (!loading) {
		videoChanged = true;
		ui->videoMsg->setText(QTStr("Settings.ProgramRestart"));
	}

	UNUSED_PARAMETER(index);
}

void OBSBasicSettings::on_fpsType_currentIndexChanged(int index)
{
	if (!loading)
		videoChanged = true;

	UNUSED_PARAMETER(index);
}

void OBSBasicSettings::on_baseResolution_editTextChanged(const QString &text)
{
	if (!loading && ValidResolutions(ui.get())) {
		QString baseResolution = ui->baseResolution->currentText();
		uint32_t cx, cy;

		ConvertResText(QT_TO_UTF8(baseResolution), cx, cy);
		ResetDownscales(cx, cy);
		videoChanged = true;
	}

	UNUSED_PARAMETER(text);
}

void OBSBasicSettings::on_outputResolution_editTextChanged(const QString &text)
{
	if (!loading && ValidResolutions(ui.get()))
		videoChanged = true;

	UNUSED_PARAMETER(text);
}

void OBSBasicSettings::on_fpsCommon_currentIndexChanged(int index)
{
	if (!loading)
		videoChanged = true;

	UNUSED_PARAMETER(index);
}

void OBSBasicSettings::on_fpsInteger_valueChanged(int value)
{
	if (!loading)
		videoChanged = true;

	UNUSED_PARAMETER(value);
}

void OBSBasicSettings::on_fpsNumerator_valueChanged(int value)
{
	if (!loading)
		videoChanged = true;

	UNUSED_PARAMETER(value);
}

void OBSBasicSettings::on_fpsDenominator_valueChanged(int value)
{
	if (!loading)
		videoChanged = true;

	UNUSED_PARAMETER(value);
}
