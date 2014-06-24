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

#pragma once

#include <util/util.hpp>
#include <QDialog>
#include <memory>

#include <obs.h>

class OBSBasic;
class QAbstractButton;
class QComboBox;
class OBSPropertiesView;

#include "ui_OBSBasicSettings.h"

class OBSBasicSettings : public QDialog {
	Q_OBJECT

private:
	OBSBasic *main;

	std::unique_ptr<Ui::OBSBasicSettings> ui;
	ConfigFile localeIni;
	bool generalChanged;
	bool outputsChanged;
	bool audioChanged;
	bool videoChanged;
	int  pageIndex;
	bool loading;

	OBSPropertiesView *streamProperties;

	void SaveCombo(QComboBox *widget, const char *section,
			const char *value);
	void SaveComboData(QComboBox *widget, const char *section,
			const char *value);
	void SaveEdit(QLineEdit *widget, const char *section,
			const char *value);
	void SaveSpinBox(QSpinBox *widget, const char *section,
			const char *value);

	inline bool Changed() const
	{
		return generalChanged || outputsChanged ||
			audioChanged || videoChanged;
	}

	inline void EnableApplyButton(bool en)
	{
		ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(en);
	}

	inline void ClearChanged()
	{
		generalChanged = false;
		outputsChanged = false;
		audioChanged   = false;
		videoChanged   = false;
		EnableApplyButton(false);
	}

	void HookWidget(QWidget *widget, const char *signal, const char *slot);

	bool QueryChanges();

	void LoadServiceTypes();
	void LoadServiceInfo();

	void LoadGeneralSettings();
	void LoadOutputSettings();
	void LoadAudioSettings();
	void LoadVideoSettings();
	void LoadSettings(bool changedOnly);

	/* general */
	void LoadLanguageList();

	/* output */
	void LoadSimpleOutputSettings();

	/* audio */
	void LoadListValues(QComboBox *widget, obs_property_t prop,
		const char *configName);
	void LoadAudioDevices();

	/* video */
	void LoadRendererList();
	void ResetDownscales(uint32_t cx, uint32_t cy);
	void LoadResolutionLists();
	void LoadFPSData();

	void SaveGeneralSettings();
	void SaveOutputSettings();
	void SaveAudioSettings();
	void SaveVideoSettings();
	void SaveSettings();

private slots:
	void on_listWidget_itemSelectionChanged();
	void on_buttonBox_clicked(QAbstractButton *button);

	void on_streamType_currentIndexChanged(int idx);
	void on_simpleOutputBrowse_clicked();

	void on_baseResolution_editTextChanged(const QString &text);

	void GeneralChanged();
	void AudioChanged();
	void AudioChangedRestart();
	void OutputsChanged();
	void VideoChanged();
	void VideoChangedResolution();
	void VideoChangedRestart();

protected:
	virtual void closeEvent(QCloseEvent *event);

public:
	OBSBasicSettings(QWidget *parent);
};
