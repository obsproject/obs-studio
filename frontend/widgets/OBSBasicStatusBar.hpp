#pragma once

#include "StatusBarWidget.hpp"

#include <obs.hpp>

#include <QPointer>
#include <QStatusBar>

class QTimer;

class OBSBasicStatusBar : public QStatusBar {
	Q_OBJECT

private:
	StatusBarWidget *statusWidget = nullptr;

	OBSWeakOutputAutoRelease streamOutput;
	std::vector<OBSSignal> streamSigs;
	OBSWeakOutputAutoRelease recordOutput;
	bool active = false;
	bool overloadedNotify = true;
	bool streamPauseIconToggle = false;
	bool disconnected = false;
	bool firstCongestionUpdate = false;

	std::vector<float> congestionArray;

	int retries = 0;
	int totalStreamSeconds = 0;
	int totalRecordSeconds = 0;

	int reconnectTimeout = 0;

	int delaySecTotal = 0;
	int delaySecStarting = 0;
	int delaySecStopping = 0;

	int startSkippedFrameCount = 0;
	int startTotalFrameCount = 0;
	int lastSkippedFrameCount = 0;

	int seconds = 0;
	uint64_t lastBytesSent = 0;
	uint64_t lastBytesSentTime = 0;

	QPixmap excellentPixmap;
	QPixmap goodPixmap;
	QPixmap mediocrePixmap;
	QPixmap badPixmap;
	QPixmap disconnectedPixmap;
	QPixmap inactivePixmap;

	QPixmap recordingActivePixmap;
	QPixmap recordingPausePixmap;
	QPixmap recordingPauseInactivePixmap;
	QPixmap recordingInactivePixmap;
	QPixmap streamingActivePixmap;
	QPixmap streamingInactivePixmap;

	float lastCongestion = 0.0f;

	QPointer<QTimer> refreshTimer;
	QPointer<QTimer> messageTimer;

	obs_output_t *GetOutput();

	void Activate();
	void Deactivate();

	void UpdateDelayMsg();
	void UpdateBandwidth();
	void UpdateStreamTime();
	void UpdateRecordTime();
	void UpdateRecordTimeLabel();
	void UpdateDroppedFrames();

	static void OBSOutputReconnect(void *data, calldata_t *params);
	static void OBSOutputReconnectSuccess(void *data, calldata_t *params);

public slots:
	void UpdateCPUUsage();

	void clearMessage();
	void showMessage(const QString &message, int timeout = 0);

private slots:
	void Reconnect(int seconds);
	void ReconnectSuccess();
	void UpdateStatusBar();
	void UpdateCurrentFPS();
	void UpdateIcons();

public:
	OBSBasicStatusBar(QWidget *parent);

	void StreamDelayStarting(int sec);
	void StreamDelayStopping(int sec);
	void StreamStarted(obs_output_t *output);
	void StreamStopped();
	void RecordingStarted(obs_output_t *output);
	void RecordingStopped();
	void RecordingPaused();
	void RecordingUnpaused();

	void ReconnectClear();
};
