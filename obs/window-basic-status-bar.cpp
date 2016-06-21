#include <QLabel>
#include "obs-app.hpp"
#include "window-basic-main.hpp"
#include "window-basic-status-bar.hpp"
#include "window-basic-main-outputs.hpp"

OBSBasicStatusBar::OBSBasicStatusBar(QWidget *parent)
	: QStatusBar    (parent),
	  delayInfo     (new QLabel),
	  droppedFrames (new QLabel),
	  sessionTime   (new QLabel),
	  cpuUsage      (new QLabel),
	  kbps          (new QLabel)
{
	sessionTime->setText(QString("00:00:00"));
	cpuUsage->setText(QString("CPU: 0.0%"));

	delayInfo->setAlignment(Qt::AlignRight);
	droppedFrames->setAlignment(Qt::AlignRight);
	sessionTime->setAlignment(Qt::AlignRight);
	cpuUsage->setAlignment(Qt::AlignRight);
	kbps->setAlignment(Qt::AlignRight);

	delayInfo->setIndent(20);
	droppedFrames->setIndent(20);
	sessionTime->setIndent(20);
	cpuUsage->setIndent(20);
	kbps->setIndent(10);

	addPermanentWidget(droppedFrames);
	addPermanentWidget(sessionTime);
	addPermanentWidget(cpuUsage);
	addPermanentWidget(delayInfo);
	addPermanentWidget(kbps);
}

void OBSBasicStatusBar::Activate()
{
	if (!active) {
		refreshTimer = new QTimer(this);
		connect(refreshTimer, SIGNAL(timeout()),
				this, SLOT(UpdateStatusBar()));

		int skipped = video_output_get_skipped_frames(obs_get_video());
		int total   = video_output_get_total_frames(obs_get_video());

		totalSeconds = 0;
		lastSkippedFrameCount = 0;
		startSkippedFrameCount = skipped;
		startTotalFrameCount = total;

		refreshTimer->start(1000);
		active = true;
	}
}

void OBSBasicStatusBar::Deactivate()
{
	OBSBasic *main = qobject_cast<OBSBasic*>(parent());
	if (!main)
		return;

	if (!main->outputHandler->Active()) {
		delete refreshTimer;
		sessionTime->setText(QString("00:00:00"));
		delayInfo->setText("");
		droppedFrames->setText("");
		kbps->setText("");

		delaySecTotal = 0;
		delaySecStarting = 0;
		delaySecStopping = 0;
		reconnectTimeout = 0;
		active = false;
	}
}

void OBSBasicStatusBar::UpdateDelayMsg()
{
	QString msg;

	if (delaySecTotal) {
		if (delaySecStarting && !delaySecStopping) {
			msg = QTStr("Basic.StatusBar.DelayStartingIn");
			msg = msg.arg(QString::number(delaySecStarting));

		} else if (!delaySecStarting && delaySecStopping) {
			msg = QTStr("Basic.StatusBar.DelayStoppingIn");
			msg = msg.arg(QString::number(delaySecStopping));

		} else if (delaySecStarting && delaySecStopping) {
			msg = QTStr("Basic.StatusBar.DelayStartingStoppingIn");
			msg = msg.arg(QString::number(delaySecStopping),
					QString::number(delaySecStarting));
		} else {
			msg = QTStr("Basic.StatusBar.Delay");
			msg = msg.arg(QString::number(delaySecTotal));
		}
	}

	delayInfo->setText(msg);
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

	if (bytesSent < lastBytesSent)
		bytesSent = 0;
	if (bytesSent == 0)
		lastBytesSent = 0;

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

	if (reconnectTimeout > 0) {
		QString msg = QTStr("Basic.StatusBar.Reconnecting");
		showMessage(msg.arg(QString::number(retries),
					QString::number(reconnectTimeout)));
		reconnectTimeout--;

	} else if (retries > 0) {
		QString msg = QTStr("Basic.StatusBar.AttemptingReconnect");
		showMessage(msg.arg(QString::number(retries)));
	}

	if (delaySecStopping > 0 || delaySecStarting > 0) {
		if (delaySecStopping > 0)
			--delaySecStopping;
		if (delaySecStarting > 0)
			--delaySecStarting;
		UpdateDelayMsg();
	}
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

void OBSBasicStatusBar::OBSOutputReconnect(void *data, calldata_t *params)
{
	OBSBasicStatusBar *statusBar =
		reinterpret_cast<OBSBasicStatusBar*>(data);

	int seconds = (int)calldata_int(params, "timeout_sec");
	QMetaObject::invokeMethod(statusBar, "Reconnect", Q_ARG(int, seconds));
	UNUSED_PARAMETER(params);
}

void OBSBasicStatusBar::OBSOutputReconnectSuccess(void *data, calldata_t *params)
{
	OBSBasicStatusBar *statusBar =
		reinterpret_cast<OBSBasicStatusBar*>(data);

	QMetaObject::invokeMethod(statusBar, "ReconnectSuccess");
	UNUSED_PARAMETER(params);
}

void OBSBasicStatusBar::Reconnect(int seconds)
{
	retries++;
	reconnectTimeout = seconds;

	if (streamOutput) {
		delaySecTotal = obs_output_get_active_delay(streamOutput);
		UpdateDelayMsg();
	}
}

void OBSBasicStatusBar::ReconnectClear()
{
	retries              = 0;
	reconnectTimeout     = 0;
	bitrateUpdateSeconds = -1;
	lastBytesSent        = 0;
	lastBytesSentTime    = os_gettime_ns();
	delaySecTotal        = 0;
	UpdateDelayMsg();
}

void OBSBasicStatusBar::ReconnectSuccess()
{
	showMessage(QTStr("Basic.StatusBar.ReconnectSuccessful"), 4000);
	ReconnectClear();

	if (streamOutput) {
		delaySecTotal = obs_output_get_active_delay(streamOutput);
		UpdateDelayMsg();
	}
}

void OBSBasicStatusBar::UpdateStatusBar()
{
	UpdateBandwidth();
	UpdateSessionTime();
	UpdateDroppedFrames();

	int skipped = video_output_get_skipped_frames(obs_get_video());
	int total   = video_output_get_total_frames(obs_get_video());

	skipped -= startSkippedFrameCount;
	total   -= startTotalFrameCount;

	int diff = skipped - lastSkippedFrameCount;
	double percentage = double(skipped) / double(total) * 100.0;

	if (diff > 10 && percentage >= 0.1f)
		showMessage(QTStr("HighResourceUsage"), 4000);

	lastSkippedFrameCount = skipped;
}

void OBSBasicStatusBar::StreamDelayStarting(int sec)
{
	OBSBasic *main = qobject_cast<OBSBasic*>(parent());
	if (!main || !main->outputHandler)
		return;

	streamOutput = main->outputHandler->streamOutput;

	delaySecTotal = delaySecStarting = sec;
	UpdateDelayMsg();
	Activate();
}

void OBSBasicStatusBar::StreamDelayStopping(int sec)
{
	delaySecTotal = delaySecStopping = sec;
	UpdateDelayMsg();
}

void OBSBasicStatusBar::StreamStarted(obs_output_t *output)
{
	streamOutput = output;

	signal_handler_connect(obs_output_get_signal_handler(streamOutput),
			"reconnect", OBSOutputReconnect, this);
	signal_handler_connect(obs_output_get_signal_handler(streamOutput),
			"reconnect_success", OBSOutputReconnectSuccess, this);

	retries           = 0;
	lastBytesSent     = 0;
	lastBytesSentTime = os_gettime_ns();
	Activate();
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

		ReconnectClear();
		streamOutput = nullptr;
		clearMessage();
		Deactivate();
	}
}

void OBSBasicStatusBar::RecordingStarted(obs_output_t *output)
{
	recordOutput = output;
	Activate();
}

void OBSBasicStatusBar::RecordingStopped()
{
	recordOutput = nullptr;
	Deactivate();
}
