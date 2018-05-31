#pragma once

#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QSlider>
#include <QMouseEvent>
#include <QLabel>
#include <QTimer>
#include <obs.hpp>

class OBSBasic;

class MediaSlider : public QSlider {
	Q_OBJECT

signals:
	void mediaSliderClicked();
	void mediaSliderHovered();

public:
	QPoint mousePos;

protected:
	void mousePressEvent(QMouseEvent *event)
	{
		if (event->button() == Qt::LeftButton)
			mousePos = event->pos();
			emit mediaSliderClicked();

		QSlider::mousePressEvent(event);
	}

	void mouseMoveEvent(QMouseEvent *event)
	{
		mousePos = event->pos();
		emit mediaSliderHovered();

		QSlider::mouseMoveEvent(event);
	}
};

class OBSBasicMediaControls : public QWidget {
	Q_OBJECT

private:
	OBSBasic      *main;
	MediaSlider   *slider;
	QLabel        *timerLabel;
	QLabel        *durationLabel;
	QTimer        *timer;
	QPushButton   *playPauseButton;
	bool          isPlaying;
	OBSSignal     selectSignal;
	OBSSignal     removeSignal;
	OBSSignal     channelChangedSignal;

	QString FormatSeconds(int totalSeconds);
	void SetPlayPauseIcon();
	uint32_t GetMediaTime();
	uint32_t GetMediaDuration();
	void SetSliderPosition();
	void SetTimerLabel();
	void SetDurationLabel();
	void MediaSeekTo(int value);
	void StartTimer();
	void StopTimer();
	void ResetControls();

	static void OBSChannelChanged(void *param, calldata_t *data);
	static void OBSSceneItemSelect(void *param, calldata_t *data);
	static void OBSSceneItemRemoved(void *param, calldata_t *data);

private slots:
	void MediaPlayPause();
	void MediaRestart();
	void MediaStop();
	void MediaNext();
	void MediaPrevious();

	void MediaSliderChanged(int value);
	void MediaSliderReleased();
	void MediaSliderClicked();
	void MediaSliderHovered();
	void Update();

public:
	OBSSource     source;

	OBSBasicMediaControls(QWidget *parent, OBSSource source_);
	~OBSBasicMediaControls();

	void SetScene(OBSScene scene);
	void SetSource(OBSSource newSource);
};
