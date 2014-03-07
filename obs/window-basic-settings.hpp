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

class OBSBasic;
class QAbstractButton;

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

	inline bool Changed() const
	{
		return generalChanged || outputsChanged || audioChanged ||
			videoChanged;
	}

	inline void ClearChanged()
	{
		generalChanged = false;
		outputsChanged = false;
		audioChanged   = false;
		videoChanged   = false;
	}

	bool QueryChanges();

	void LoadGeneralSettings();
	//void LoadOutputSettings();
	void LoadAudioSettings();
	void LoadVideoSettings();
	void LoadSettings(bool changedOnly);

	/* general */
	void LoadLanguageList();

	/* audio */
	void LoadAudioDevices();

	/* video */
	void LoadRendererList();
	void ResetDownscales(uint32_t cx, uint32_t cy);
	void LoadResolutionLists();
	void LoadFPSData();

	void SaveGeneralSettings();
	//void SaveOutputSettings();
	void SaveAudioSettings();
	void SaveVideoSettings();
	void SaveSettings();

private slots:
	void on_listWidget_itemSelectionChanged();
	void on_buttonBox_clicked(QAbstractButton *button);

	void on_language_currentIndexChanged(int index);

	void on_sampleRate_currentIndexChanged(int index);
	void on_channelSetup_currentIndexChanged(int index);
	void on_audioBufferingTime_valueChanged(int index);

	void on_renderer_currentIndexChanged(int index);
	void on_fpsType_currentIndexChanged(int index);
	void on_baseResolution_editTextChanged(const QString &text);
	void on_outputResolution_editTextChanged(const QString &text);
	void on_fpsCommon_currentIndexChanged(int index);
	void on_fpsInteger_valueChanged(int value);
	void on_fpsNumerator_valueChanged(int value);
	void on_fpsDenominator_valueChanged(int value);

protected:
	virtual void closeEvent(QCloseEvent *event);

public:
	OBSBasicSettings(QWidget *parent);
};
