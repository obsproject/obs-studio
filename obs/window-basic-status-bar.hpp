#pragma once

#include <QStatusBar>
#include <QPointer>
#include <QTimer>
#include <util/platform.h>
#include <obs.h>

class QLabel;

class OBSBasicStatusBar : public QStatusBar {
	Q_OBJECT

private:
	QLabel *droppedFrames;
	QLabel *sessionTime;
	QLabel *cpuUsage;
	QLabel *kbps;

	obs_output_t streamOutput = nullptr;
	obs_output_t recordOutput = nullptr;

	int retries = 0;
	int activeRefs = 0;
	int totalSeconds = 0;

	int      bitrateUpdateSeconds = 0;
	uint64_t lastBytesSent = 0;
	uint64_t lastBytesSentTime = 0;

	QPointer<QTimer> refreshTimer;

	void DecRef();
	void IncRef();

	obs_output_t GetOutput();

	void UpdateBandwidth();
	void UpdateSessionTime();
	void UpdateDroppedFrames();

	static void OBSOutputReconnect(void *data, calldata_t params);
	static void OBSOutputReconnectSuccess(void *data, calldata_t params);

private slots:
	void Reconnect();
	void ReconnectSuccess();
	void UpdateStatusBar();
	void UpdateCPUUsage();

public:
	OBSBasicStatusBar(QWidget *parent);

	void StreamStarted(obs_output_t output);
	void StreamStopped();
	void RecordingStarted(obs_output_t output);
	void RecordingStopped();
};
