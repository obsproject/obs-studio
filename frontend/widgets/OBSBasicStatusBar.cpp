#include "OBSBasicStatusBar.hpp"
#include "ui_StatusBarWidget.h"

#include <widgets/OBSBasic.hpp>

#include "moc_OBSBasicStatusBar.cpp"

static constexpr int bitrateUpdateSeconds = 2;
static constexpr int congestionUpdateSeconds = 4;
static constexpr float excellentThreshold = 0.0f;
static constexpr float goodThreshold = 0.3333f;
static constexpr float mediocreThreshold = 0.6667f;
static constexpr float badThreshold = 1.0f;

OBSBasicStatusBar::OBSBasicStatusBar(QWidget *parent)
	: QStatusBar(parent),
	  excellentPixmap(QIcon(":/res/images/network-excellent.svg").pixmap(QSize(16, 16))),
	  goodPixmap(QIcon(":/res/images/network-good.svg").pixmap(QSize(16, 16))),
	  mediocrePixmap(QIcon(":/res/images/network-mediocre.svg").pixmap(QSize(16, 16))),
	  badPixmap(QIcon(":/res/images/network-bad.svg").pixmap(QSize(16, 16))),
	  recordingActivePixmap(QIcon(":/res/images/recording-active.svg").pixmap(QSize(16, 16))),
	  recordingPausePixmap(QIcon(":/res/images/recording-pause.svg").pixmap(QSize(16, 16))),
	  streamingActivePixmap(QIcon(":/res/images/streaming-active.svg").pixmap(QSize(16, 16)))
{
	congestionArray.reserve(congestionUpdateSeconds);

	statusWidget = new StatusBarWidget(this);
	statusWidget->ui->delayInfo->setText("");
	statusWidget->ui->droppedFrames->setText(QTStr("DroppedFrames").arg("0", "0.0"));
	statusWidget->ui->statusIcon->setPixmap(inactivePixmap);
	statusWidget->ui->streamIcon->setPixmap(streamingInactivePixmap);
	statusWidget->ui->streamTime->setDisabled(true);
	statusWidget->ui->recordIcon->setPixmap(recordingInactivePixmap);
	statusWidget->ui->recordTime->setDisabled(true);
	statusWidget->ui->delayFrame->hide();
	statusWidget->ui->issuesFrame->hide();
	statusWidget->ui->kbps->hide();

	// Initialize Vertical Stream UI elements (assuming they exist in StatusBarWidget.ui)
	// Initialize Vertical Stream UI elements
	statusWidget->ui->vStreamTime->setText(QString("V: 00:00:00"));
	statusWidget->ui->vStreamTime->setDisabled(true);
	statusWidget->ui->vStreamIcon->setPixmap(streamingInactivePixmap);
	statusWidget->ui->vStatusIcon->setPixmap(inactivePixmap);
	statusWidget->ui->vKbps->hide();
	statusWidget->ui->vDroppedFrames->hide();
	statusWidget->ui->verticalStreamFrame->hide(); // Hide the whole frame initially

	addPermanentWidget(statusWidget, 1);
	setMinimumHeight(statusWidget->height());

	UpdateIcons(); // This will need to be updated to handle vertical icons if they are different
	connect(App(), &OBSApp::StyleChanged, this, &OBSBasicStatusBar::UpdateIcons);

	// Connect signals for vertical streaming from BasicOutputHandler
	OBSBasic *main = qobject_cast<OBSBasic *>(parent);
	if (main && main->outputHandler) {
		connect(main->outputHandler.get(), &BasicOutputHandler::startVerticalStreaming,
		        this, &OBSBasicStatusBar::VerticalStreamStarted);
		connect(main->outputHandler.get(), &BasicOutputHandler::stopVerticalStreaming,
		        this, &OBSBasicStatusBar::VerticalStreamStopped);
		connect(main->outputHandler.get(), &BasicOutputHandler::verticalStreamDelayStarting,
		        this, &OBSBasicStatusBar::VerticalStreamDelayStarting);
		connect(main->outputHandler.get(), &BasicOutputHandler::verticalStreamStopping,
		        this, &OBSBasicStatusBar::VerticalStreamStopping);
		// TODO: Connect vertical recording signals when implemented
	}


	messageTimer = new QTimer(this);
	messageTimer->setSingleShot(true);
	connect(messageTimer, &QTimer::timeout, this, &OBSBasicStatusBar::clearMessage);

	clearMessage();
}

void OBSBasicStatusBar::Activate()
{
	if (!active) {
		refreshTimer = new QTimer(this);
		connect(refreshTimer, &QTimer::timeout, this, &OBSBasicStatusBar::UpdateStatusBar);

		int skipped = video_output_get_skipped_frames(obs_get_video());
		int total = video_output_get_total_frames(obs_get_video());

		totalStreamSeconds = 0;
		totalRecordSeconds = 0;
		lastSkippedFrameCount = 0;
		startSkippedFrameCount = skipped;
		startTotalFrameCount = total;

		refreshTimer->start(1000);
		active = true;

		if (streamOutput) {
			statusWidget->ui->statusIcon->setPixmap(inactivePixmap);
		}
	}

	if (streamOutput) {
		statusWidget->ui->streamIcon->setPixmap(streamingActivePixmap);
		statusWidget->ui->streamTime->setDisabled(false);
		statusWidget->ui->issuesFrame->show();
		statusWidget->ui->kbps->show();
		firstCongestionUpdate = true;
	}

	if (recordOutput) {
		statusWidget->ui->recordIcon->setPixmap(recordingActivePixmap);
		statusWidget->ui->recordTime->setDisabled(false);
	}

	// Vertical Stream UI activation
	if (verticalStreamOutput_) { // Check if vertical stream output is configured
		statusWidget->ui->verticalStreamFrame->show();
		statusWidget->ui->vStreamIcon->setPixmap(streamingActivePixmap);
		statusWidget->ui->vStreamTime->setDisabled(false);
		statusWidget->ui->vKbps->show();
		statusWidget->ui->vDroppedFrames->show();
		statusWidget->ui->vStatusIcon->setPixmap(inactivePixmap); // Initial status
		vertical_stream_first_congestion_update_ = true;
	}
}

void OBSBasicStatusBar::Deactivate()
{
	OBSBasic *main = qobject_cast<OBSBasic *>(parent());
	if (!main)
		return;

	if (!streamOutput) {
		statusWidget->ui->streamTime->setText(QString("00:00:00"));
		statusWidget->ui->streamTime->setDisabled(true);
		statusWidget->ui->streamIcon->setPixmap(streamingInactivePixmap);
		statusWidget->ui->statusIcon->setPixmap(inactivePixmap);
		statusWidget->ui->delayFrame->hide();
		statusWidget->ui->issuesFrame->hide();
		statusWidget->ui->kbps->hide();
		totalStreamSeconds = 0;
		congestionArray.clear();
		disconnected = false;
		firstCongestionUpdate = false;
	}

	if (!recordOutput) {
		statusWidget->ui->recordTime->setText(QString("00:00:00"));
		statusWidget->ui->recordTime->setDisabled(true);
		statusWidget->ui->recordIcon->setPixmap(recordingInactivePixmap);
		totalRecordSeconds = 0;
	}

	bool anyOutputActive = (streamOutput != nullptr) || (recordOutput != nullptr) || (verticalStreamOutput_ != nullptr);

	if (main->outputHandler && !main->outputHandler->Active() && !anyOutputActive) { // Check if any output is active
		if (refreshTimer) { // Check if timer exists before deleting
			delete refreshTimer;
			refreshTimer = nullptr; // Nullify after delete
		}

		statusWidget->ui->delayInfo->setText("");
		statusWidget->ui->droppedFrames->setText(QTStr("DroppedFrames").arg("0", "0.0"));
		statusWidget->ui->kbps->setText("0 kbps");

		delaySecTotal = 0;
		delaySecStarting = 0;
		delaySecStopping = 0;
		reconnectTimeout = 0;
		active = false;
		overloadedNotify = true;

		statusWidget->ui->statusIcon->setPixmap(inactivePixmap);
	}

	// Vertical Stream UI deactivation
	if (!verticalStreamOutput_) {
		statusWidget->ui->vStreamTime->setText(QString("V: 00:00:00"));
		statusWidget->ui->vStreamTime->setDisabled(true);
		statusWidget->ui->vStreamIcon->setPixmap(streamingInactivePixmap);
		statusWidget->ui->vStatusIcon->setPixmap(inactivePixmap);
		// if (statusWidget->ui->vDelayInfo) statusWidget->ui->vDelayInfo->hide(); // No vDelayInfo yet
		statusWidget->ui->vKbps->hide();
		statusWidget->ui->vDroppedFrames->hide();
		statusWidget->ui->verticalStreamFrame->hide();
		verticalStreamTotalSeconds_ = 0;
		verticalStreamCongestionArray_.clear();
		verticalStreamDisconnected_ = false;
		vertical_stream_first_congestion_update_ = false;
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
			msg = msg.arg(QString::number(delaySecStopping), QString::number(delaySecStarting));
		} else {
			msg = QTStr("Basic.StatusBar.Delay");
			msg = msg.arg(QString::number(delaySecTotal));
		}

		if (!statusWidget->ui->delayFrame->isVisible())
			statusWidget->ui->delayFrame->show();

		statusWidget->ui->delayInfo->setText(msg);
	}
}

void OBSBasicStatusBar::UpdateBandwidth()
{
	if (!streamOutput)
		return;

	if (++seconds < bitrateUpdateSeconds)
		return;

	OBSOutput output = OBSGetStrongRef(streamOutput);
	if (!output)
		return;

	uint64_t bytesSent = obs_output_get_total_bytes(output);
	uint64_t bytesSentTime = os_gettime_ns();

	if (bytesSent < lastBytesSent)
		bytesSent = 0;
	if (bytesSent == 0)
		lastBytesSent = 0;

	uint64_t bitsBetween = (bytesSent - lastBytesSent) * 8;

	double timePassed = double(bytesSentTime - lastBytesSentTime) / 1000000000.0;

	double kbitsPerSec = double(bitsBetween) / timePassed / 1000.0;

	QString text;
	text += QString::number(kbitsPerSec, 'f', 0) + QString(" kbps");

	statusWidget->ui->kbps->setText(text);
	statusWidget->ui->kbps->setMinimumWidth(statusWidget->ui->kbps->width());

	if (!statusWidget->ui->kbps->isVisible())
		statusWidget->ui->kbps->show();

	lastBytesSent = bytesSent;
	lastBytesSentTime = bytesSentTime;
	seconds = 0;
}

void OBSBasicStatusBar::UpdateCPUUsage()
{
	OBSBasic *main = qobject_cast<OBSBasic *>(parent());
	if (!main)
		return;

	QString text;
	text += QString("CPU: ") + QString::number(main->GetCPUUsage(), 'f', 1) + QString("%");

	statusWidget->ui->cpuUsage->setText(text);
	statusWidget->ui->cpuUsage->setMinimumWidth(statusWidget->ui->cpuUsage->width());

	UpdateCurrentFPS();
}

void OBSBasicStatusBar::UpdateCurrentFPS()
{
	struct obs_video_info ovi;
	obs_get_video_info(&ovi);
	float targetFPS = (float)ovi.fps_num / (float)ovi.fps_den;

	QString text = QString::asprintf("%.2f / %.2f FPS", obs_get_active_fps(), targetFPS);

	statusWidget->ui->fpsCurrent->setText(text);
	statusWidget->ui->fpsCurrent->setMinimumWidth(statusWidget->ui->fpsCurrent->width());
}

void OBSBasicStatusBar::UpdateStreamTime()
{
	totalStreamSeconds++;

	int seconds = totalStreamSeconds % 60;
	int totalMinutes = totalStreamSeconds / 60;
	int minutes = totalMinutes % 60;
	int hours = totalMinutes / 60;

	QString text = QString::asprintf("%02d:%02d:%02d", hours, minutes, seconds);
	statusWidget->ui->streamTime->setText(text);
	if (streamOutput && !statusWidget->ui->streamTime->isEnabled())
		statusWidget->ui->streamTime->setDisabled(false);

	if (reconnectTimeout > 0) {
		QString msg = QTStr("Basic.StatusBar.Reconnecting")
				      .arg(QString::number(retries), QString::number(reconnectTimeout));
		showMessage(msg);
		disconnected = true;
		statusWidget->ui->statusIcon->setPixmap(disconnectedPixmap);
		congestionArray.clear();
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

extern volatile bool recording_paused;

void OBSBasicStatusBar::UpdateRecordTime()
{
	bool paused = os_atomic_load_bool(&recording_paused);

	if (!paused) {
		totalRecordSeconds++;

		if (recordOutput && !statusWidget->ui->recordTime->isEnabled())
			statusWidget->ui->recordTime->setDisabled(false);
	} else {
		statusWidget->ui->recordIcon->setPixmap(streamPauseIconToggle ? recordingPauseInactivePixmap
									      : recordingPausePixmap);

		streamPauseIconToggle = !streamPauseIconToggle;
	}

	UpdateRecordTimeLabel();
}

void OBSBasicStatusBar::UpdateRecordTimeLabel()
{
	int seconds = totalRecordSeconds % 60;
	int totalMinutes = totalRecordSeconds / 60;
	int minutes = totalMinutes % 60;
	int hours = totalMinutes / 60;

	QString text = QString::asprintf("%02d:%02d:%02d", hours, minutes, seconds);
	if (os_atomic_load_bool(&recording_paused)) {
		text += QStringLiteral(" (PAUSED)");
	}

	statusWidget->ui->recordTime->setText(text);
}

void OBSBasicStatusBar::UpdateDroppedFrames()
{
	if (!streamOutput)
		return;

	OBSOutput output = OBSGetStrongRef(streamOutput);
	if (!output)
		return;

	int totalDropped = obs_output_get_frames_dropped(output);
	int totalFrames = obs_output_get_total_frames(output);
	double percent = (double)totalDropped / (double)totalFrames * 100.0;

	if (!totalFrames)
		return;

	QString text = QTStr("DroppedFrames");
	text = text.arg(QString::number(totalDropped), QString::number(percent, 'f', 1));
	statusWidget->ui->droppedFrames->setText(text);

	if (!statusWidget->ui->issuesFrame->isVisible())
		statusWidget->ui->issuesFrame->show();

	/* ----------------------------------- *
	 * calculate congestion color          */

	float congestion = obs_output_get_congestion(output);
	float avgCongestion = (congestion + lastCongestion) * 0.5f;
	if (avgCongestion < congestion)
		avgCongestion = congestion;
	if (avgCongestion > 1.0f)
		avgCongestion = 1.0f;

	lastCongestion = congestion;

	if (disconnected)
		return;

	bool update = firstCongestionUpdate;
	float congestionOverTime = avgCongestion;

	if (congestionArray.size() >= congestionUpdateSeconds) {
		congestionOverTime = accumulate(congestionArray.begin(), congestionArray.end(), 0.0f) /
				     (float)congestionArray.size();
		congestionArray.clear();
		update = true;
	} else {
		congestionArray.emplace_back(avgCongestion);
	}

	if (update) {
		if (congestionOverTime <= excellentThreshold + EPSILON)
			statusWidget->ui->statusIcon->setPixmap(excellentPixmap);
		else if (congestionOverTime <= goodThreshold)
			statusWidget->ui->statusIcon->setPixmap(goodPixmap);
		else if (congestionOverTime <= mediocreThreshold)
			statusWidget->ui->statusIcon->setPixmap(mediocrePixmap);
		else if (congestionOverTime <= badThreshold)
			statusWidget->ui->statusIcon->setPixmap(badPixmap);

		firstCongestionUpdate = false;
	}
}

void OBSBasicStatusBar::OBSOutputReconnect(void *data, calldata_t *params)
{
	OBSBasicStatusBar *statusBar = static_cast<OBSBasicStatusBar *>(data);

	int seconds = (int)calldata_int(params, "timeout_sec");
	QMetaObject::invokeMethod(statusBar, "Reconnect", Q_ARG(int, seconds));
}

void OBSBasicStatusBar::OBSOutputReconnectSuccess(void *data, calldata_t *)
{
	OBSBasicStatusBar *statusBar = static_cast<OBSBasicStatusBar *>(data);

	QMetaObject::invokeMethod(statusBar, "ReconnectSuccess");
}

void OBSBasicStatusBar::Reconnect(int seconds)
{
	OBSBasic *main = qobject_cast<OBSBasic *>(parent());

	if (!retries)
		main->SysTrayNotify(QTStr("Basic.SystemTray.Message.Reconnecting"), QSystemTrayIcon::Warning);

	reconnectTimeout = seconds;

	if (streamOutput) {
		OBSOutput output = OBSGetStrongRef(streamOutput);
		if (!output)
			return;

		delaySecTotal = obs_output_get_active_delay(output);
		UpdateDelayMsg();

		retries++;
	}
}

void OBSBasicStatusBar::ReconnectClear()
{
	retries = 0;
	reconnectTimeout = 0;
	seconds = -1;
	lastBytesSent = 0;
	lastBytesSentTime = os_gettime_ns();
	delaySecTotal = 0;
	UpdateDelayMsg();
}

void OBSBasicStatusBar::ReconnectSuccess()
{
	OBSBasic *main = qobject_cast<OBSBasic *>(parent());

	QString msg = QTStr("Basic.StatusBar.ReconnectSuccessful");
	showMessage(msg, 4000);
	main->SysTrayNotify(msg, QSystemTrayIcon::Information);
	ReconnectClear();

	if (streamOutput) {
		OBSOutput output = OBSGetStrongRef(streamOutput);
		if (!output)
			return;

		delaySecTotal = obs_output_get_active_delay(output);
		UpdateDelayMsg();
		disconnected = false;
		firstCongestionUpdate = true;
	}
}

void OBSBasicStatusBar::UpdateStatusBar()
{
	OBSBasic *main = qobject_cast<OBSBasic *>(parent());

	UpdateBandwidth();

	if (streamOutput)
		UpdateStreamTime();

	if (recordOutput)
		UpdateRecordTime();

	UpdateDroppedFrames(); // For horizontal stream

	if (verticalStreamingActive_ && verticalStreamOutput_) {
		UpdateVerticalStreamTime();
		UpdateVerticalBandwidth();
		UpdateVerticalDroppedFrames();
	}

	int skipped = video_output_get_skipped_frames(obs_get_video());
	int total = video_output_get_total_frames(obs_get_video());

	skipped -= startSkippedFrameCount;
	total -= startTotalFrameCount;

	int diff = skipped - lastSkippedFrameCount;
	double percentage = double(skipped) / double(total) * 100.0;

	if (diff > 10 && percentage >= 0.1f) {
		showMessage(QTStr("HighResourceUsage"), 4000);
		if (!main->isVisible() && overloadedNotify) {
			main->SysTrayNotify(QTStr("HighResourceUsage"), QSystemTrayIcon::Warning);
			overloadedNotify = false;
		}
	}

	lastSkippedFrameCount = skipped;
}

void OBSBasicStatusBar::StreamDelayStarting(int sec)
{
	OBSBasic *main = qobject_cast<OBSBasic *>(parent());
	if (!main || !main->outputHandler)
		return;

	OBSOutputAutoRelease output = obs_frontend_get_streaming_output();
	streamOutput = OBSGetWeakRef(output);

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
	streamOutput = OBSGetWeakRef(output);

	streamSigs.emplace_back(obs_output_get_signal_handler(output), "reconnect", OBSOutputReconnect, this);
	streamSigs.emplace_back(obs_output_get_signal_handler(output), "reconnect_success", OBSOutputReconnectSuccess,
				this);

	retries = 0;
	lastBytesSent = 0;
	lastBytesSentTime = os_gettime_ns();
	Activate();
}

void OBSBasicStatusBar::StreamStopped()
{
	if (streamOutput) {
		streamSigs.clear();

		ReconnectClear();
		streamOutput = nullptr;
		clearMessage();
		Deactivate();
	}
}

void OBSBasicStatusBar::RecordingStarted(obs_output_t *output)
{
	recordOutput = OBSGetWeakRef(output);
	Activate();
}

void OBSBasicStatusBar::RecordingStopped()
{
	recordOutput = nullptr;
	Deactivate();
}

void OBSBasicStatusBar::RecordingPaused()
{
	if (recordOutput) {
		statusWidget->ui->recordIcon->setPixmap(recordingPausePixmap);
		streamPauseIconToggle = true;
	}

	UpdateRecordTimeLabel();
}

void OBSBasicStatusBar::RecordingUnpaused()
{
	if (recordOutput) {
		statusWidget->ui->recordIcon->setPixmap(recordingActivePixmap);
	}

	UpdateRecordTimeLabel();
}

static QPixmap GetPixmap(const QString &filename)
{
	QString path = obs_frontend_is_theme_dark() ? "theme:Dark/" : ":/res/images/";
	return QIcon(path + filename).pixmap(QSize(16, 16));
}

void OBSBasicStatusBar::UpdateIcons()
{
	disconnectedPixmap = GetPixmap("network-disconnected.svg");
	inactivePixmap = GetPixmap("network-inactive.svg");

	streamingInactivePixmap = GetPixmap("streaming-inactive.svg");

	recordingInactivePixmap = GetPixmap("recording-inactive.svg");
	recordingPauseInactivePixmap = GetPixmap("recording-pause-inactive.svg");

	bool streaming = obs_frontend_streaming_active();

	if (!streaming) {
		statusWidget->ui->streamIcon->setPixmap(streamingInactivePixmap);
		statusWidget->ui->statusIcon->setPixmap(inactivePixmap);
	} else {
		if (disconnected)
			statusWidget->ui->statusIcon->setPixmap(disconnectedPixmap);
	}

	bool recording = obs_frontend_recording_active();

	if (!recording)
		statusWidget->ui->recordIcon->setPixmap(recordingInactivePixmap);
}

void OBSBasicStatusBar::showMessage(const QString &message, int timeout)
{
	messageTimer->stop();

	statusWidget->ui->message->setText(message);

	if (timeout)
		messageTimer->start(timeout);
}

void OBSBasicStatusBar::clearMessage()
{
	statusWidget->ui->message->setText("");
}

// ----------------------------------------------------------------------------
// Vertical Streaming Slots Implementation

void OBSBasicStatusBar::VerticalStreamStarted(obs_output_t *output)
{
	verticalStreamOutput_ = OBSGetWeakRef(output); // Store a weak ref
	verticalStreamingActive_ = true;
	verticalStreamDisconnected_ = false;
	verticalStreamTotalSeconds_ = 0;
	verticalStreamReconnectTimeout_ = 0;
	verticalStreamRetries_ = 0;
	verticalStreamLastBytesSent_ = 0;
	verticalStreamLastBytesSentTime_ = os_gettime_ns();
	verticalStreamSecondsCounter_ = 0;
	verticalStreamCongestionArray_.clear();
	vertical_stream_first_congestion_update_ = true;


	statusWidget->ui->verticalStreamFrame->show();
	statusWidget->ui->vStreamIcon->setPixmap(streamingActivePixmap);
	statusWidget->ui->vStreamTime->setDisabled(false);
	statusWidget->ui->vStatusIcon->setPixmap(inactivePixmap);
	statusWidget->ui->vKbps->show();
	statusWidget->ui->vDroppedFrames->show();


	// Use the main refresh timer (refreshTimer) if it's not already active,
	// as it handles things like CPU/FPS updates that are global.
	// Activate() will start it if needed.
	Activate();

	// Specific timer for updating vertical stream time if needed, or integrate into main UpdateStatusBar
	// For simplicity, let's assume UpdateStatusBar will be modified to also call UpdateVerticalStreamTime.
	// If a separate timer is preferred:
	/*
	if (!verticalStreamTimer) {
		verticalStreamTimer = new QTimer(this);
		connect(verticalStreamTimer, &QTimer::timeout, this, &OBSBasicStatusBar::UpdateVerticalStreamTime);
	}
	if (!verticalStreamTimer->isActive()) {
		verticalStreamTimer->start(1000);
	}
	*/

	if (output) { // Ensure output is valid before getting signal handler
		verticalStreamSigs.emplace_back(obs_output_get_signal_handler(output), "reconnect", OBSOutputVerticalReconnect, this);
		verticalStreamSigs.emplace_back(obs_output_get_signal_handler(output), "reconnect_success", OBSOutputVerticalReconnectSuccess, this);
	}

	blog(LOG_INFO, "Vertical streaming started (status bar handling).");
}

void OBSBasicStatusBar::VerticalStreamStopped(int code, QString last_error)
{
	verticalStreamingActive_ = false;
	verticalStreamDisconnected_ = false;
	// verticalStreamDelayActive_ = false; // This state would be in BasicOutputHandler, not directly here

	// if(verticalStreamTimer) verticalStreamTimer->stop(); // Stop specific timer if used
	verticalStreamSigs.clear();
	verticalStreamOutput_ = nullptr;

	// TODO (UI): Update UI elements for vertical stream to inactive/stopped state
	// Example:
	// if (statusWidget->ui->vStreamTime) {
	// 	statusWidget->ui->vStreamTime->setText(QString("00:00:00"));
	// 	statusWidget->ui->vStreamTime->setDisabled(true);
	// }
	// if (statusWidget->ui->vStreamIcon) statusWidget->ui->vStreamIcon->setPixmap(streamingInactivePixmap);
	// if (statusWidget->ui->vStatusIcon) statusWidget->ui->vStatusIcon->setPixmap(inactivePixmap);
	statusWidget->ui->vStreamTime->setText(QString("V: 00:00:00"));
	statusWidget->ui->vStreamTime->setDisabled(true);
	statusWidget->ui->vStreamIcon->setPixmap(streamingInactivePixmap);
	statusWidget->ui->vStatusIcon->setPixmap(inactivePixmap);
	// if (statusWidget->ui->vDelayInfo) statusWidget->ui->vDelayInfo->hide(); // No vDelayInfo yet
	statusWidget->ui->vKbps->hide();
	statusWidget->ui->vDroppedFrames->hide();
	statusWidget->ui->verticalStreamFrame->hide();

	Deactivate(); // Call Deactivate to check if the main refreshTimer can be stopped

	blog(LOG_INFO, "Vertical streaming stopped (status bar handling). Code: %d, Error: %s", code, last_error.toUtf8().constData());
	if (code != 0 && !last_error.isEmpty()) {
		// TODO (UI): Consider a different message or way to show vertical stream specific errors if general showMessage is too intrusive.
		// For now, using general showMessage. A dedicated label for vertical stream errors might be better.
		showMessage(QTStr("Output.StreamStoppedPrematurely.Vertical", "Vertical Stream Stopped Prematurely: Code %1, Error: %2").arg(code).arg(last_error), 5000);
	}
}

void OBSBasicStatusBar::UpdateVerticalStreamTime()
{
	if (!verticalStreamingActive_) {
		return;
	}

	verticalStreamTotalSeconds_++;

	int seconds = verticalStreamTotalSeconds_ % 60;
	int totalMinutes = verticalStreamTotalSeconds_ / 60;
	int minutes = totalMinutes % 60;
	int hours = totalMinutes / 60;

	QString text = QString::asprintf("V: %02d:%02d:%02d", hours, minutes, seconds);
	statusWidget->ui->vStreamTime->setText(text);
	if (!statusWidget->ui->vStreamTime->isEnabled()) statusWidget->ui->vStreamTime->setDisabled(false);

	if (verticalStreamReconnectTimeout_ > 0) {
		QString msg = QTStr("Basic.StatusBar.Reconnecting.Vertical", "Vertical Reconnecting: attempt %1 in %2 sec")
				      .arg(QString::number(verticalStreamRetries_), QString::number(verticalStreamReconnectTimeout_));
		// TODO: Show this message appropriately, perhaps in a dedicated vertical status message area or log
		// For now, we can use the main message bar, or just log it.
		// showMessage(msg); // This would overwrite main message
		statusWidget->ui->vStatusIcon->setPixmap(disconnectedPixmap); // Use main disconnected Pixmap
		verticalStreamCongestionArray_.clear();
		verticalStreamReconnectTimeout_--;
		verticalStreamDisconnected_ = true;


	} else if (verticalStreamRetries_ > 0 && !verticalStreamDisconnected_) { // Only show attempting if not already in hard disconnected state
		QString msg = QTStr("Basic.StatusBar.AttemptingReconnect.Vertical", "Vertical Stream: Attempting reconnect %1...")
                                .arg(QString::number(verticalStreamRetries_));
		// showMessage(msg);
	}


	// TODO: Handle vertical delay display if/when implemented
	// if (verticalStreamDelaySecStopping_ > 0 || verticalStreamDelaySecStarting_ > 0) { UpdateVerticalDelayMsg_V(); }
}

void OBSBasicStatusBar::UpdateVerticalBandwidth() {
	if (!verticalStreamOutput_ || !verticalStreamingActive_) return;

	if (++verticalStreamSecondsCounter_ < bitrateUpdateSeconds) return; // Use same bitrateUpdateSeconds

	OBSOutput output = OBSGetStrongRef(verticalStreamOutput_);
	if (!output) return;

	uint64_t bytesSent = obs_output_get_total_bytes(output);
	uint64_t bytesSentTime = os_gettime_ns();

	if (bytesSent < verticalStreamLastBytesSent_) bytesSent = 0; // Reset if byte counter wrapped or output restarted
	if (bytesSent == 0 && verticalStreamLastBytesSent_ != 0) verticalStreamLastBytesSent_ = 0; // If output reset, ensure we don't use old value

	uint64_t bitsBetween = (bytesSent - verticalStreamLastBytesSent_) * 8;
	double timePassed = double(bytesSentTime - verticalStreamLastBytesSentTime_) / 1000000000.0;
	double kbitsPerSec = (timePassed > EPSILON) ? (double(bitsBetween) / timePassed / 1000.0) : 0.0;

	QString text = QString("V: %1 kbps").arg(QString::number(kbitsPerSec, 'f', 0));
	statusWidget->ui->vKbps->setText(text);
	// statusWidget->ui->vKbps->setMinimumWidth(statusWidget->ui->vKbps->width()); // Optional: adjust width

	if (!statusWidget->ui->vKbps->isVisible()) statusWidget->ui->vKbps->show();

	verticalStreamLastBytesSent_ = bytesSent;
	verticalStreamLastBytesSentTime_ = bytesSentTime;
	verticalStreamSecondsCounter_ = 0;
}

void OBSBasicStatusBar::UpdateVerticalDroppedFrames() {
    if (!verticalStreamOutput_ || !verticalStreamingActive_) return;

    OBSOutput output = OBSGetStrongRef(verticalStreamOutput_);
    if (!output) return;

    int totalDropped = obs_output_get_frames_dropped(output);
    int totalFrames = obs_output_get_total_frames(output);
    double percent = (totalFrames > 0) ? ((double)totalDropped / (double)totalFrames * 100.0) : 0.0;

    QString text = QTStr("DroppedFrames.Vertical", "V Dropped: %1 (%2%)")
                           .arg(QString::number(totalDropped))
                           .arg(QString::number(percent, 'f', 1));
    statusWidget->ui->vDroppedFrames->setText(text);

    if (!statusWidget->ui->vDroppedFrames->isVisible()) statusWidget->ui->vDroppedFrames->show();

    float congestion = obs_output_get_congestion(output);
    float avgCongestion = (congestion + verticalStreamLastCongestion_) * 0.5f;
    if (avgCongestion < congestion) avgCongestion = congestion; // Prioritize current higher congestion
    if (avgCongestion > 1.0f) avgCongestion = 1.0f;
    verticalStreamLastCongestion_ = congestion;

    if (verticalStreamDisconnected_) return;

    bool update = vertical_stream_first_congestion_update_;
    float congestionOverTime = avgCongestion;

    if (verticalStreamCongestionArray_.size() >= congestionUpdateSeconds) {
        congestionOverTime = std::accumulate(verticalStreamCongestionArray_.begin(), verticalStreamCongestionArray_.end(), 0.0f) /
                             (float)verticalStreamCongestionArray_.size();
        verticalStreamCongestionArray_.clear();
        update = true;
    } else {
        verticalStreamCongestionArray_.emplace_back(avgCongestion);
    }

    if (update) {
        if (congestionOverTime <= excellentThreshold + EPSILON)
            statusWidget->ui->vStatusIcon->setPixmap(excellentPixmap);
        else if (congestionOverTime <= goodThreshold)
            statusWidget->ui->vStatusIcon->setPixmap(goodPixmap);
        else if (congestionOverTime <= mediocreThreshold)
            statusWidget->ui->vStatusIcon->setPixmap(mediocrePixmap);
        else
            statusWidget->ui->vStatusIcon->setPixmap(badPixmap);
        vertical_stream_first_congestion_update_ = false;
    }
}


void OBSBasicStatusBar::VerticalStreamDelayStarting(int seconds)
{
	// TODO: Implement UI for vertical stream delay if a separate delay indicator is added.
	// For now, just log.
	blog(LOG_INFO, "Vertical stream delay UI update: Starting in %d seconds.", seconds);
	// Example if vDelayInfo existed:
	// if (statusWidget->ui->vDelayInfo) {
	// 	QString msg = QTStr("Basic.StatusBar.DelayStartingIn.Vertical", "V Delay Starting: %1s").arg(QString::number(seconds));
	//	statusWidget->ui->vDelayInfo->setText(msg);
	//	statusWidget->ui->vDelayInfo->show();
	// }
	// verticalStreamDelaySecStarting_ = seconds; // Need member if counting down here
}

void OBSBasicStatusBar::VerticalStreamStopping()
{
	// TODO: Implement UI for vertical stream stopping delay.
	blog(LOG_INFO, "Vertical stream stopping UI update (with delay).");
	// Example if vDelayInfo and verticalStreamDelaySecStopping_ existed:
	// if (statusWidget->ui->vDelayInfo && verticalStreamDelaySecStopping_ > 0) {
	// 	QString msg = QTStr("Basic.StatusBar.DelayStoppingIn.Vertical", "V Delay Stopping: %1s").arg(QString::number(verticalStreamDelaySecStopping_));
	//	statusWidget->ui->vDelayInfo->setText(msg);
	//	statusWidget->ui->vDelayInfo->show();
	// } else if (statusWidget->ui->vDelayInfo) {
	//    statusWidget->ui->vDelayInfo->hide();
	// }
}

// Callbacks for vertical stream reconnect
void OBSBasicStatusBar::OBSOutputVerticalReconnect(void *data, calldata_t *params)
{
	OBSBasicStatusBar *statusBar = static_cast<OBSBasicStatusBar *>(data);
	int seconds = (int)calldata_int(params, "timeout_sec");
	QMetaObject::invokeMethod(statusBar, "VerticalReconnect", Qt::QueuedConnection, Q_ARG(int, seconds));
}

void OBSBasicStatusBar::OBSOutputVerticalReconnectSuccess(void *data, calldata_t *)
{
	OBSBasicStatusBar *statusBar = static_cast<OBSBasicStatusBar *>(data);
	QMetaObject::invokeMethod(statusBar, "VerticalReconnectSuccess", Qt::QueuedConnection);
}

void OBSBasicStatusBar::VerticalReconnect(int seconds)
{
	OBSBasic *main = qobject_cast<OBSBasic *>(parent());
	if (!main) return;

	if (!verticalStreamRetries_) {
		// main->SysTrayNotify(QTStr("Basic.SystemTray.Message.Reconnecting.Vertical", "Vertical Stream: Reconnecting..."), QSystemTrayIcon::Warning);
	}

	verticalStreamReconnectTimeout_ = seconds;
	verticalStreamDisconnected_ = true; // Mark as disconnected during reconnect attempt

	if (verticalStreamOutput_) {
		OBSOutput output = OBSGetStrongRef(verticalStreamOutput_);
		if (output) {
			// verticalStreamDelaySecTotal_ = obs_output_get_active_delay(output); // If vertical delay is implemented
			// UpdateVerticalDelayMsg_V();
		}
		verticalStreamRetries_++;
	}
}

void OBSBasicStatusBar::VerticalReconnectSuccess()
{
	OBSBasic *main = qobject_cast<OBSBasic *>(parent());
	if (!main) return;

	// QString msg = QTStr("Basic.StatusBar.ReconnectSuccessful.Vertical", "Vertical Stream: Reconnect successful!");
	// showMessage(msg, 4000); // This would overwrite main message
	// main->SysTrayNotify(msg, QSystemTrayIcon::Information);

	verticalStreamRetries_ = 0;
	verticalStreamReconnectTimeout_ = 0;
	verticalStreamDisconnected_ = false; // Mark as connected
	// verticalStreamDelaySecTotal_ = 0;
	// UpdateVerticalDelayMsg_V();

	if (verticalStreamOutput_) {
		OBSOutput output = OBSGetStrongRef(verticalStreamOutput_);
		if (output) {
			// verticalStreamDelaySecTotal_ = obs_output_get_active_delay(output);
			// UpdateVerticalDelayMsg_V();
		}
		vertical_stream_first_congestion_update_ = true; // Force congestion update
		UpdateVerticalDroppedFrames(); // Update status immediately
	}
}
