#include <QLabel>
#include "obs-app.hpp"
#include "window-basic-main.hpp"
#include "window-basic-status-bar.hpp"

OBSBasicStatusBar::OBSBasicStatusBar(QWidget *parent)
	: QStatusBar    (parent),
	  droppedFrames (new QLabel),
	  sessionTime   (new QLabel),
	  cpuUsage      (new QLabel),
	  kbps          (new QLabel)
{
	sessionTime->setText(QString("00:00:00"));
	cpuUsage->setText(QString("CPU: 0.0%"));

	droppedFrames->setAlignment(Qt::AlignRight);
	sessionTime->setAlignment(Qt::AlignRight);
	cpuUsage->setAlignment(Qt::AlignRight);
	kbps->setAlignment(Qt::AlignRight);

	droppedFrames->setIndent(20);
	sessionTime->setIndent(20);
	cpuUsage->setIndent(20);
	kbps->setIndent(10);

	addPermanentWidget(droppedFrames);
	addPermanentWidget(sessionTime);
	addPermanentWidget(cpuUsage);
	addPermanentWidget(kbps);
}

void OBSBasicStatusBar::IncRef()
{
	if (activeRefs++ == 0) {
		refreshTimer = new QTimer(this);
		connect(refreshTimer, SIGNAL(timeout()),
				this, SLOT(UpdateStatusBar()));
		totalSeconds = 0;
		refreshTimer->start(1000);
	}
}

void OBSBasicStatusBar::DecRef()
{
	if (--activeRefs == 0) {
		delete refreshTimer;
		sessionTime->setText(QString("00:00:00"));
		droppedFrames->setText("");
		kbps->setText("");
	}
}

#define BITRATE_UPDATE_SECONDS 2

void OBSBasicStatusBar::UpdateBandwidth()
{
	if (!streamOutput)
		return;

	if (++bitrateUpdateSeconds < BITRATE_UPDATE_SECONDS)
		return;

	uint64_t bytesSent     = obs_output_get_total_bytes(streamOutput);
	uint64_t bytesSentTime = os_gettime_ns();
	uint64_t bitsBetween   = (bytesSent - lastBytesSent) * 8;

	double timePassed = double(bytesSentTime - lastBytesSentTime) /
		1000000000.0;

	double kbitsPerSec = double(bitsBetween) / timePassed / 1000.0;

	QString text;
	text += QString("kb/s: ") +
		QString::number(kbitsPerSec, 'f', 0);
	kbps->setText(text);
	kbps->setMinimumWidth(kbps->width());

	lastBytesSent        = bytesSent;
	lastBytesSentTime    = bytesSentTime;
	bitrateUpdateSeconds = 0;
}

void OBSBasicStatusBar::UpdateCPUUsage()
{
	OBSBasic *main = qobject_cast<OBSBasic*>(parent());
	if (!main)
		return;

	QString text;
	text += QString("CPU: ") +
		QString::number(main->GetCPUUsage(), 'f', 1) + QString("%");
	cpuUsage->setText(text);
	cpuUsage->setMinimumWidth(cpuUsage->width());
}

void OBSBasicStatusBar::UpdateSessionTime()
{
	totalSeconds++;

	int seconds      = totalSeconds % 60;
	int totalMinutes = totalSeconds / 60;
	int minutes      = totalMinutes % 60;
	int hours        = totalMinutes / 60;

	QString text;
	text.sprintf("%02d:%02d:%02d", hours, minutes, seconds);
	sessionTime->setText(text);
	sessionTime->setMinimumWidth(sessionTime->width());
}

void OBSBasicStatusBar::UpdateDroppedFrames()
{
	if (!streamOutput)
		return;

	int totalDropped = obs_output_get_frames_dropped(streamOutput);
	int totalFrames  = obs_output_get_total_frames(streamOutput);
	double percent   = (double)totalDropped / (double)totalFrames * 100.0;

	if (!totalFrames)
		return;

	QString text = QTStr("DroppedFrames");
	text = text.arg(QString::number(totalDropped),
			QString::number(percent, 'f', 1));
	droppedFrames->setText(text);
	droppedFrames->setMinimumWidth(droppedFrames->width());
}

void OBSBasicStatusBar::OBSOutputReconnect(void *data, calldata_t params)
{
	OBSBasicStatusBar *statusBar =
		reinterpret_cast<OBSBasicStatusBar*>(data);

	QMetaObject::invokeMethod(statusBar, "Reconnect");
	UNUSED_PARAMETER(params);
}

void OBSBasicStatusBar::OBSOutputReconnectSuccess(void *data, calldata_t params)
{
	OBSBasicStatusBar *statusBar =
		reinterpret_cast<OBSBasicStatusBar*>(data);

	QMetaObject::invokeMethod(statusBar, "ReconnectSuccess");
	UNUSED_PARAMETER(params);
}

void OBSBasicStatusBar::Reconnect()
{
	retries++;

	QString reconnectMsg = QTStr("Basic.StatusBar.Reconnecting");
	showMessage(reconnectMsg.arg(QString::number(retries)));
}

void OBSBasicStatusBar::ReconnectSuccess()
{
	showMessage(QTStr("Basic.StatusBar.ReconnectSuccessful"), 4000);
	retries              = 0;
	bitrateUpdateSeconds = -1;
	lastBytesSent        = 0;
	lastBytesSentTime    = os_gettime_ns();
}

void OBSBasicStatusBar::UpdateStatusBar()
{
	UpdateBandwidth();
	UpdateSessionTime();
	UpdateDroppedFrames();
}

void OBSBasicStatusBar::StreamStarted(obs_output_t output)
{
	streamOutput = output;

	signal_handler_connect(obs_output_get_signal_handler(streamOutput),
			"reconnect", OBSOutputReconnect, this);
	signal_handler_connect(obs_output_get_signal_handler(streamOutput),
			"reconnect_success", OBSOutputReconnectSuccess, this);

	retries           = 0;
	lastBytesSent     = 0;
	lastBytesSentTime = os_gettime_ns();
	IncRef();
}

void OBSBasicStatusBar::StreamStopped()
{
	if (streamOutput) {
		signal_handler_disconnect(
				obs_output_get_signal_handler(streamOutput),
				"reconnect", OBSOutputReconnect, this);
		signal_handler_disconnect(
				obs_output_get_signal_handler(streamOutput),
				"reconnect_success", OBSOutputReconnectSuccess,
				this);

		streamOutput = nullptr;
		clearMessage();
		DecRef();
	}
}

void OBSBasicStatusBar::RecordingStarted(obs_output_t output)
{
	recordOutput = output;
	IncRef();
}

void OBSBasicStatusBar::RecordingStopped()
{
	recordOutput = nullptr;
	DecRef();
}
