/******************************************************************************
    Copyright (C) 2025 by Taylor Giampaolo <warchamp7@obsproject.com>

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

#include <Idian/Utils.hpp>

#include "OBSBasic.hpp"
#include "VolControl.hpp"

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QScrollArea>

#include <unordered_set>

class AudioMixer : public QFrame {
	Q_OBJECT

	struct RankedVol {
		VolControl *vol;
		int sortWeight;
	};

public:
	explicit AudioMixer(QWidget *parent = 0);
	~AudioMixer();

	void refreshVolumeColors();

	void setMixerLayoutVertical(bool vertical);
	void showMixerContextMenu();

private:
	idian::Utils *utils;

	std::vector<OBSSignal> signalHandlers;
	static void onFrontendEvent(enum obs_frontend_event event, void *data);
	void handleFrontendEvent(enum obs_frontend_event event);

	std::unordered_map<QString, QPointer<VolControl>> volumeList;
	void addControlForUuid(QString uuid);
	void removeControlForUuid(QString uuid);

	std::unordered_set<QString> globalSources;
	std::unordered_set<QString> previewSources;
	bool mixerVertical = false;

	int hiddenCount = 0;
	bool showHidden = false;
	bool showInactive = false;

	bool keepInactiveRight = false;
	bool keepHiddenRight = false;

	bool showToolbar = true;

	QFrame *mixerFrame = nullptr;
	QVBoxLayout *mainLayout = nullptr;
	QVBoxLayout *mixerLayout = nullptr;

	QStackedWidget *stackedMixerArea = nullptr;
	QToolBar *mixerToolbar = nullptr;
	QAction *layoutButton = nullptr;
	QAction *advAudio = nullptr;
	QPushButton *toggleHiddenButton = nullptr;
	QMenu mixerMenu;

	QVBoxLayout *hMainLayout = nullptr;
	QWidget *hMixerContainer = nullptr;
	QScrollArea *hMixerScrollArea = nullptr;
	QWidget *hVolumeWidgets = nullptr;
	QVBoxLayout *hVolControlLayout = nullptr;

	QHBoxLayout *vMainLayout = nullptr;
	QWidget *vMixerContainer = nullptr;
	QScrollArea *vMixerScrollArea = nullptr;
	QWidget *vVolumeWidgets = nullptr;
	QHBoxLayout *vVolControlLayout = nullptr;

	QBoxLayout *activeLayout() const;

	void reloadVolumeControls();
	bool getMixerVisibilityForControl(VolControl *widget);

	void addPreviewSource(const char *uuid);
	void clearPreviewSources();
	bool isSourcePreviewed(obs_source_t *source);
	bool isSourceGlobal(obs_source_t *source);
	void clearVolumeControls();
	void updateShowInactive();
	void updateKeepInactiveRight();
	void updateShowHidden();
	void updateKeepHiddenRight();
	void updateShowToolbar();

	QTimer updateTimer;
	void updateVolumeLayouts();

	/* OBS Callbacks */
	static void obsSourceActivated(void *data, calldata_t *params);
	static void obsSourceDeactivated(void *data, calldata_t *params);
	static void obsSourceAudioActivated(void *data, calldata_t *params);
	static void obsSourceAudioDeactivated(void *data, calldata_t *params);
	static void obsSourceCreate(void *data, calldata_t *params);
	static void obsSourceRemove(void *data, calldata_t *params);
	static void obsSourceRename(void *data, calldata_t *params);

private slots:
	void sourceCreated(QString uuid);
	void sourceRemoved(QString uuid);
	void updatePreviewSources();
	void updateGlobalSources();
	void unhideAllAudioControls();
	void queueLayoutUpdate();

	VolControl *createVolumeControl(obs_source_t *source);
	void updateControlVisibility(std::string uuid);

	void toggleLayout();
	void toggleShowInactive(bool checked);
	void toggleKeepInactiveRight(bool checked);
	void toggleShowHidden(bool checked);
	void toggleKeepHiddenRight(bool checked);

	void on_actionMixerToolbarMenu_triggered();

	void mixerContextMenuRequested();

signals:
	void advAudioPropertiesClicked();
};
