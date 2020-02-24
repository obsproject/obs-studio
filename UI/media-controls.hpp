#pragma once

#include <QTimer>
#include <obs.hpp>
#include "window-basic-main.hpp"
#include "qt-wrappers.hpp"
#include "obs-frontend-api/obs-frontend-api.h"

#include "ui_MediaControls.h"

class MediaControls : public QWidget {
	Q_OBJECT

private:
	OBSSource source = nullptr;
	OBSBasic *main;
	QTimer *timer;
	bool prevPaused = false;

	OBSSignal selectSignal;
	OBSSignal removeSignal;
	OBSSignal channelChangedSignal;

	QString FormatSeconds(int totalSeconds);
	void StartTimer();
	void StopTimer();
	void RefreshControls();
	void SetScene(OBSScene scene);

	static void OBSMediaStopped(void *data, calldata_t *calldata);
	static void OBSMediaPlay(void *data, calldata_t *calldata);
	static void OBSMediaPause(void *data, calldata_t *calldata);
	static void OBSMediaStarted(void *data, calldata_t *calldata);
	static void OBSSceneItemRemoved(void *param, calldata_t *data);
	static void OBSSceneItemSelect(void *param, calldata_t *data);

	static void OBSFrontendEvent(enum obs_frontend_event event, void *ptr);

	std::unique_ptr<Ui::MediaControls> ui;

private slots:
	void on_playPauseButton_clicked();
	void on_stopButton_clicked();
	void on_nextButton_clicked();
	void on_previousButton_clicked();
	void SliderClicked();
	void SliderReleased(int val);
	void SliderHovered(int val);
	void SetSliderPosition();
	void SetPlayingState();
	void SetPausedState();
	void SetRestartState();

public:
	MediaControls(QWidget *parent = nullptr);
	~MediaControls();

	OBSSource GetSource();
	void SetSource(OBSSource newSource);
};
