/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>
                          Philippe Groarke <philippe.groarke@gmail.com>

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

#pragma once

#include <util/util.hpp>
#include <QDialog>
#include <QPointer>
#include <memory>
#include <string>

#include <libff/ff-util.h>

#include <obs.hpp>

#include "auth-base.hpp"
#include <window-control.hpp>

class OBSBasic;
class QAbstractButton;
class QComboBox;
class QCheckBox;
class QLabel;
class OBSPropertiesView;
class OBSControlWidget;
class OBSActionsWidget;
#include "ui_OBSBasicSettings.h"
#include "mapper.hpp"
#include "obs-frontend-api.h"
#define VOLUME_METER_DECAY_FAST 23.53
#define VOLUME_METER_DECAY_MEDIUM 11.76
#define VOLUME_METER_DECAY_SLOW 8.57

class SilentUpdateCheckBox : public QCheckBox {
	Q_OBJECT

public slots:
	void setCheckedSilently(bool checked)
	{
		bool blocked = blockSignals(true);
		setChecked(checked);
		blockSignals(blocked);
	}
};

class SilentUpdateSpinBox : public QSpinBox {
	Q_OBJECT

public slots:
	void setValueSilently(int val)
	{
		bool blocked = blockSignals(true);
		setValue(val);
		blockSignals(blocked);
	}
};

class OBSFFDeleter {
public:
	void operator()(const ff_format_desc *format)
	{
		ff_format_desc_free(format);
	}
	void operator()(const ff_codec_desc *codec)
	{
		ff_codec_desc_free(codec);
	}
};
using OBSFFCodecDesc = std::unique_ptr<const ff_codec_desc, OBSFFDeleter>;
using OBSFFFormatDesc = std::unique_ptr<const ff_format_desc, OBSFFDeleter>;

class OBSBasicSettings : public QDialog {
	Q_OBJECT
	Q_PROPERTY(QIcon generalIcon READ GetGeneralIcon WRITE SetGeneralIcon
			   DESIGNABLE true)
	Q_PROPERTY(QIcon streamIcon READ GetStreamIcon WRITE SetStreamIcon
			   DESIGNABLE true)
	Q_PROPERTY(QIcon outputIcon READ GetOutputIcon WRITE SetOutputIcon
			   DESIGNABLE true)
	Q_PROPERTY(QIcon audioIcon READ GetAudioIcon WRITE SetAudioIcon
			   DESIGNABLE true)
	Q_PROPERTY(QIcon videoIcon READ GetVideoIcon WRITE SetVideoIcon
			   DESIGNABLE true)
	Q_PROPERTY(QIcon advancedIcon READ GetAdvancedIcon WRITE SetAdvancedIcon
			   DESIGNABLE true)
	Q_PROPERTY(QIcon controlsIcon READ GetHotkeysIcon WRITE SetControlsIcon
			   DESIGNABLE true)
private:
	OBSBasic *main;

	std::unique_ptr<Ui::OBSBasicSettings> ui;
	QList<QPointer<OBSControlWidget>> controlsWidgets;

	std::shared_ptr<Auth> auth;
	ControlMapper *mapper;

	bool generalChanged = false;
	bool stream1Changed = false;
	bool outputsChanged = false;
	bool audioChanged = false;
	bool videoChanged = false;
	bool advancedChanged = false;
	int pageIndex = 0;
	bool loading = true;
	bool forceAuthReload = false;
	std::string savedTheme;
	int sampleRateIndex = 0;
	int channelIndex = 0;

	int lastSimpleRecQualityIdx = 0;
	int lastChannelSetupIdx = 0;
	int prevLangIndex;

	OBSFFFormatDesc formats;

	OBSPropertiesView *streamProperties = nullptr;
	OBSPropertiesView *streamEncoderProps = nullptr;
	OBSPropertiesView *recordEncoderProps = nullptr;

	QPointer<QLabel> advOutRecWarning;
	QPointer<QLabel> simpleOutRecWarning;

	QString curPreset;
	QString curQSVPreset;
	QString curNVENCPreset;
	QString curAMDPreset;

	QString curAdvStreamEncoder;
	QString curAdvRecordEncoder;

	using AudioSource_t =
		std::tuple<OBSWeakSource, QPointer<QCheckBox>,
			   QPointer<QSpinBox>, QPointer<QCheckBox>,
			   QPointer<QSpinBox>>;
	std::vector<AudioSource_t> audioSources;
	std::vector<OBSSignal> audioSourceSignals;
	OBSSignal sourceCreated;
	OBSSignal channelChanged;

	uint32_t outputCX = 0;
	uint32_t outputCY = 0;

	void SaveCombo(QComboBox *widget, const char *section,
		       const char *value);
	void SaveComboData(QComboBox *widget, const char *section,
			   const char *value);
	void SaveCheckBox(QAbstractButton *widget, const char *section,
			  const char *value, bool invert = false);
	void SaveEdit(QLineEdit *widget, const char *section,
		      const char *value);
	void SaveSpinBox(QSpinBox *widget, const char *section,
			 const char *value);
	void SaveFormat(QComboBox *combo);
	void SaveEncoder(QComboBox *combo, const char *section,
			 const char *value);

	inline bool Changed() const
	{
		return generalChanged || outputsChanged || stream1Changed ||
		       audioChanged || videoChanged || advancedChanged;
	}

	inline void EnableApplyButton(bool en)
	{
		ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(en);
	}

	inline void ClearChanged()
	{
		generalChanged = false;
		stream1Changed = false;
		outputsChanged = false;
		audioChanged = false;
		videoChanged = false;
		advancedChanged = false;
		EnableApplyButton(false);
	}

#ifdef _WIN32
	bool aeroWasDisabled = false;
	QCheckBox *toggleAero = nullptr;
	void ToggleDisableAero(bool checked);
#endif

	void HookWidget(QWidget *widget, const char *signal, const char *slot);

	bool QueryChanges();

	void LoadEncoderTypes();
	void LoadColorRanges();
	void LoadFormats();
	void ReloadCodecs(const ff_format_desc *formatDesc);

	void LoadGeneralSettings();
	void LoadStream1Settings();
	void LoadOutputSettings();
	void LoadAudioSettings();
	void LoadVideoSettings();
	//void ControlsChanged();
	//void ReloadControls();
	void LoadAdvancedSettings();
	void LoadSettings(bool changedOnly);

	OBSPropertiesView *CreateEncoderPropertyView(const char *encoder,
						     const char *path,
						     bool changed = false);

	/* general */
	void LoadLanguageList();
	void LoadThemeList();

	/* stream */
	void InitStreamPage();
	inline bool IsCustomService() const;
	void LoadServices(bool showAll);
	void OnOAuthStreamKeyConnected();
	void OnAuthConnected();
	QString lastService;

	bool prevBrowserAccel;
	QStackedWidget *tabs;
	QLabel *noHotkeys;
	QWidget *hotkeysFrontend;
	QWidget *hotkeysScenes;
	QWidget *hotkeysSources;
	QWidget *hotkeysFilters;
	QWidget *hotkeysOutputs;
	QWidget *hotkeysEncoders;
	QWidget *hotkeysServices;

	QComboBox *scenesCombo;
	QComboBox *sourcesCombo;
	QComboBox *filtersSourceCombo;
	QComboBox *filtersCombo;
	QComboBox *outputsCombo;
	QComboBox *encodersCombo;
	QComboBox *servicesCombo;

	QLabel *sceneLabel;
	QLabel *sourceLabel;
	QLabel *filtersSourceLabel;
	QLabel *outputLabel;
	QLabel *encoderLabel;
	QLabel *serviceLabel;


private slots:
	void UpdateServerList();
	void UpdateKeyLink();
	void on_show_clicked();
	void on_authPwShow_clicked();
	void on_connectAccount_clicked();
	void on_disconnectAccount_clicked();
	void on_useStreamKey_clicked();
	void on_useAuth_toggled();

private:
	bool startup = true;
	/* output */
	void LoadSimpleOutputSettings();
	void LoadAdvOutputStreamingSettings();
	void LoadAdvOutputStreamingEncoderProperties();
	void LoadAdvOutputRecordingSettings();
	void LoadAdvOutputRecordingEncoderProperties();
	void LoadAdvOutputFFmpegSettings();
	void LoadAdvOutputAudioSettings();
	void SetAdvOutputFFmpegEnablement(ff_codec_type encoderType,
					  bool enabled,
					  bool enableEncode = false);

	/* audio */
	void LoadListValues(QComboBox *widget, obs_property_t *prop, int index);
	void LoadAudioDevices();
	void LoadAudioSources();

	/* video */
	void LoadRendererList();
	void ResetDownscales(uint32_t cx, uint32_t cy);
	void LoadDownscaleFilters();
	void LoadResolutionLists();
	void LoadFPSData();

	void SaveGeneralSettings();
	void SaveStream1Settings();
	void SaveOutputSettings();
	void SaveAudioSettings();
	void SaveVideoSettings();
	void SaveAdvancedSettings();
	void SaveSettings();

	void UpdateSimpleOutStreamDelayEstimate();
	void UpdateAdvOutStreamDelayEstimate();

	void FillSimpleRecordingValues();
	void FillSimpleStreamingValues();
	void FillAudioMonitoringDevices();

	void RecalcOutputResPixels(const char *resText);

	QIcon generalIcon;
	QIcon streamIcon;
	QIcon outputIcon;
	QIcon audioIcon;
	QIcon videoIcon;
	QIcon hotkeysIcon;
	QIcon advancedIcon;
	QIcon controlsIcon;

	QIcon GetGeneralIcon() const;
	QIcon GetStreamIcon() const;
	QIcon GetOutputIcon() const;
	QIcon GetAudioIcon() const;
	QIcon GetVideoIcon() const;
	QIcon GetHotkeysIcon() const;
	QIcon GetAdvancedIcon() const;
	QIcon GetControlsIcon() const;

	int CurrentFLVTrack();

	QStringList ControlsList;


	using encoders_elem_t =
		std::tuple<OBSEncoder, QPointer<QLabel>, QPointer<QWidget>>;
	using outputs_elem_t =
		std::tuple<OBSOutput, QPointer<QLabel>, QPointer<QWidget>>;
	using services_elem_t =
		std::tuple<OBSService, QPointer<QLabel>, QPointer<QWidget>>;
	using sources_elem_t =
		std::tuple<OBSSource, QPointer<QLabel>, QPointer<QWidget>>;
	std::vector<sources_elem_t> filters;

	QStringList encoders;
	QStringList outputs;
	QStringList services;
	QStringList scenes;
	QStringList sources;



private slots:
	void on_theme_activated(int idx);
	void on_treeWidget_itemSelectionChanged();
	void on_buttonBox_clicked(QAbstractButton *button);
	void on_service_currentIndexChanged(int idx);
	void on_simpleOutputBrowse_clicked();
	void on_advOutRecPathBrowse_clicked();
	void on_advOutFFPathBrowse_clicked();
	void on_advOutEncoder_currentIndexChanged(int idx);
	void on_advOutRecEncoder_currentIndexChanged(int idx);
	void on_advOutFFIgnoreCompat_stateChanged(int state);
	void on_advOutFFFormat_currentIndexChanged(int idx);
	void on_advOutFFAEncoder_currentIndexChanged(int idx);
	void on_advOutFFVEncoder_currentIndexChanged(int idx);
	void on_advOutFFType_currentIndexChanged(int idx);

	void on_colorFormat_currentIndexChanged(const QString &text);

	void on_filenameFormatting_textEdited(const QString &text);
	void on_outputResolution_editTextChanged(const QString &text);
	void on_baseResolution_editTextChanged(const QString &text);

	void on_disableOSXVSync_clicked();

	void GeneralChanged();
	void AudioChanged();
	void AudioChangedRestart();
	void ReloadAudioSources();
	void SurroundWarning(int idx);
	void SpeakerLayoutChanged(int idx);
	void OutputsChanged();
	void Stream1Changed();
	void VideoChanged();
	void VideoChangedResolution();
	void VideoChangedRestart();
	void AdvancedChanged();
	void AdvancedChangedRestart();

	void UpdateStreamDelayEstimate();

	void UpdateAutomaticReplayBufferCheckboxes();

	void AdvOutRecCheckWarnings();

	void SimpleRecordingQualityChanged();
	void SimpleRecordingEncoderChanged();
	void SimpleRecordingQualityLosslessWarning(int idx);

	void SimpleReplayBufferChanged();
	void AdvReplayBufferChanged();

	void SimpleStreamingEncoderChanged();

	OBSService SpawnTempService();

	void SetGeneralIcon(const QIcon &icon);
	void SetStreamIcon(const QIcon &icon);
	void SetOutputIcon(const QIcon &icon);
	void SetAudioIcon(const QIcon &icon);
	void SetVideoIcon(const QIcon &icon);
	void SetControlsIcon(const QIcon &icon);
	void SetAdvancedIcon(const QIcon &icon);
	void SettingListSelectionChanged();
public:
	OBSBasicSettings(QWidget *parent);
	~OBSBasicSettings();

protected:
	virtual void closeEvent(QCloseEvent *event);



//*********************Control Mapper*****************************//
public:
	QStringList get_control_list();
	void load_controls();
	obs_data_t *inputaction;
	obs_data_t *outputaction;
private:
	QComboBox *TriggerFilter;
	QComboBox *ActionsFilter;
signals:
	void on_control_change(QString Change);
	void edit_trigger(obs_data_t *trigger);
	void edit_action(obs_data_t *action);
public slots:
	void add_row(obs_data_t *trigger, obs_data_t *action);
	void edit_row(int row, obs_data_t *trigger, obs_data_t *action);
	void delete_row();
	void reset_to_defaults();
	void clear_table();
private slots:
	void filter_control_table(QString filter);
	void control_table_selection(int row, int col);
};
