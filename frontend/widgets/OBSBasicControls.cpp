#include "OBSBasicControls.hpp"
#include "OBSBasic.hpp"
#include "OutputScheduleDialog.hpp"
#include "qt-wrappers.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QTimer>
#include <QSet>

#include "moc_OBSBasicControls.cpp"

OBSBasicControls::OBSBasicControls(OBSBasic *main) : QFrame(nullptr), ui(new Ui::OBSBasicControls)
{
	/* Create UI elements */
	ui->setupUi(this);
	ui->buttonsVLayout->setSpacing(8);
	ui->buttonsVLayout->setContentsMargins(12, 12, 12, 12);
	ui->recordStreamButton->setText(QTStr("Nova.StreamRecord"));
	ui->scheduleButton->setText(QTStr("Nova.Scheduler"));
	ui->recordStreamButton->setMinimumHeight(42);
	ui->streamButton->setMinimumHeight(36);
	ui->recordButton->setMinimumHeight(36);
	ui->buttonsVLayout->removeWidget(ui->scheduleButton);
	ui->buttonsVLayout->insertWidget(ui->buttonsVLayout->indexOf(ui->modeSwitch), ui->scheduleButton);

	auto *session = new QFrame(this);
	session->setObjectName("novaSession");
	auto *sessionLayout = new QVBoxLayout(session);
	sessionLayout->setContentsMargins(14, 16, 14, 16);
	sessionLayout->setSpacing(10);
	auto *caption = new QLabel(QTStr("Nova.Session"), session);
	caption->setObjectName("novaEyebrow");
	sessionTime = new QLabel("00:00:00", session);
	sessionTime->setObjectName("novaSessionTime");
	sessionStatus = new QLabel(QTStr("Nova.Ready"), session);
	sessionStatus->setWordWrap(true);
	sessionLayout->addWidget(caption);
	sessionLayout->addWidget(sessionTime);
	sessionLayout->addWidget(sessionStatus);
	ui->buttonsVLayout->insertWidget(0, session);
	auto *sessionTimer = new QTimer(this);
	connect(sessionTimer, &QTimer::timeout, this, [this] {
		if (!sessionClock.isValid() || (!sessionStreaming && !sessionRecording))
			return;
		const qint64 seconds = sessionClock.elapsed() / 1000;
		sessionTime->setText(QString("%1:%2:%3")
			.arg(seconds / 3600, 2, 10, QLatin1Char('0'))
			.arg((seconds / 60) % 60, 2, 10, QLatin1Char('0'))
			.arg(seconds % 60, 2, 10, QLatin1Char('0')));
	});
	sessionTimer->start(250);
	connect(main, &OBSBasic::StreamingStarted, this, [this](bool withDelay) {
		if (!withDelay) {
			sessionStreaming = true;
			UpdateSession();
		}
	});
	connect(main, &OBSBasic::StreamingStopped, this, [this](bool withDelay) {
		if (!withDelay) {
			sessionStreaming = false;
			UpdateSession();
		}
	});
	connect(main, &OBSBasic::RecordingStarted, this, [this] {
		sessionRecording = true;
		UpdateSession();
	});
	connect(main, &OBSBasic::RecordingStopped, this, [this] {
		sessionRecording = false;
		UpdateSession();
	});
	connect(ui->recordStreamButton, &QPushButton::clicked, this, &OBSBasicControls::RecordStreamButtonClicked);
	connect(ui->scheduleButton, &QPushButton::clicked, this, &OBSBasicControls::OpenSchedule);
	const char *saved = config_get_string(App()->GetUserConfig(), "OutputSchedule", "EntriesV2");
	if (saved) {
		const auto entries = QJsonDocument::fromJson(QByteArray(saved)).array();
		QSet<QString> ids;
		for (const auto &value : entries) {
			const auto entry = OutputSchedule::FromJson(value.toObject());
			if (entry.IsValid() && !ids.contains(entry.id)) {
				schedules.append(entry);
				ids.insert(entry.id);
			}
		}
	} else {
		// Migrate the original UTC one-time starts without changing their instant.
		const char *legacy = config_get_string(App()->GetUserConfig(), "OutputSchedule", "Starts");
		const auto entries = QJsonDocument::fromJson(legacy ? QByteArray(legacy) : QByteArray()).array();
		for (const auto &value : entries) {
			const auto date = QDateTime::fromString(value.toString(), Qt::ISODateWithMs).toLocalTime();
			if (!date.isValid() || date <= lastScheduleCheck)
				continue;
			OutputSchedule entry;
			entry.name = QTStr("Scheduler.Migrated");
			entry.firstDate = date.date();
			entry.time = date.time();
			schedules.append(entry);
		}
	}
	auto *scheduleTimer = new QTimer(this);
	connect(scheduleTimer, &QTimer::timeout, this, &OBSBasicControls::CheckSchedule);
	scheduleTimer->start(1000);

	streamButtonMenu.reset(new QMenu());
	startStreamAction = streamButtonMenu->addAction(QTStr("Basic.Main.StartStreaming"));
	stopStreamAction = streamButtonMenu->addAction(QTStr("Basic.Main.StopStreaming"));
	QAction *forceStopStreamAction = streamButtonMenu->addAction(QTStr("Basic.Main.ForceStopStreaming"));

	/* Transfer buttons signals as OBSBasicControls signals */
	connect(
		ui->streamButton, &QPushButton::clicked, this, [this]() { emit this->StreamButtonClicked(); },
		Qt::DirectConnection);
	connect(
		ui->broadcastButton, &QPushButton::clicked, this, [this]() { emit this->BroadcastButtonClicked(); },
		Qt::DirectConnection);
	connect(
		ui->recordButton, &QPushButton::clicked, this, [this]() { emit this->RecordButtonClicked(); },
		Qt::DirectConnection);
	connect(
		ui->pauseRecordButton, &QPushButton::clicked, this, [this]() { emit this->PauseRecordButtonClicked(); },
		Qt::DirectConnection);
	connect(
		ui->replayBufferButton, &QPushButton::clicked, this,
		[this]() { emit this->ReplayBufferButtonClicked(); }, Qt::DirectConnection);
	connect(
		ui->saveReplayButton, &QPushButton::clicked, this,
		[this]() { emit this->SaveReplayBufferButtonClicked(); }, Qt::DirectConnection);
	connect(
		ui->virtualCamButton, &QPushButton::clicked, this, [this]() { emit this->VirtualCamButtonClicked(); },
		Qt::DirectConnection);
	connect(
		ui->virtualCamConfigButton, &QPushButton::clicked, this,
		[this]() { emit this->VirtualCamConfigButtonClicked(); }, Qt::DirectConnection);
	connect(
		ui->modeSwitch, &QPushButton::clicked, this, [this]() { emit this->StudioModeButtonClicked(); },
		Qt::DirectConnection);
	connect(
		ui->settingsButton, &QPushButton::clicked, this, [this]() { emit this->SettingsButtonClicked(); },
		Qt::DirectConnection);

	/* Transfer menu actions signals as OBSBasicControls signals */
	connect(
		startStreamAction.get(), &QAction::triggered, this,
		[this]() { emit this->StartStreamMenuActionClicked(); }, Qt::DirectConnection);
	connect(
		stopStreamAction.get(), &QAction::triggered, this,
		[this]() { emit this->StopStreamMenuActionClicked(); }, Qt::DirectConnection);
	connect(
		forceStopStreamAction, &QAction::triggered, this,
		[this]() { emit this->ForceStopStreamMenuActionClicked(); }, Qt::DirectConnection);

	/* Set up default visibility */
	ui->broadcastButton->setVisible(false);
	ui->pauseRecordButton->setVisible(false);
	ui->replayBufferButton->setVisible(false);
	ui->saveReplayButton->setVisible(false);
	ui->virtualCamButton->setVisible(false);
	ui->virtualCamConfigButton->setVisible(false);

	/* Set up state update connections */
	connect(main, &OBSBasic::StreamingPreparing, this, &OBSBasicControls::StreamingPreparing);
	connect(main, &OBSBasic::StreamingStarting, this, &OBSBasicControls::StreamingStarting);
	connect(main, &OBSBasic::StreamingStarted, this, &OBSBasicControls::StreamingStarted);
	connect(main, &OBSBasic::StreamingStopping, this, &OBSBasicControls::StreamingStopping);
	connect(main, &OBSBasic::StreamingStopped, this, &OBSBasicControls::StreamingStopped);

	connect(main, &OBSBasic::BroadcastStreamReady, this, &OBSBasicControls::BroadcastStreamReady);
	connect(main, &OBSBasic::BroadcastStreamActive, this, &OBSBasicControls::BroadcastStreamActive);
	connect(main, &OBSBasic::BroadcastStreamStarted, this, &OBSBasicControls::BroadcastStreamStarted);

	connect(main, &OBSBasic::RecordingStarted, this, &OBSBasicControls::RecordingStarted);
	connect(main, &OBSBasic::RecordingPaused, this, &OBSBasicControls::RecordingPaused);
	connect(main, &OBSBasic::RecordingUnpaused, this, &OBSBasicControls::RecordingUnpaused);
	connect(main, &OBSBasic::RecordingStopping, this, &OBSBasicControls::RecordingStopping);
	connect(main, &OBSBasic::RecordingStopped, this, &OBSBasicControls::RecordingStopped);

	connect(main, &OBSBasic::ReplayBufStarted, this, &OBSBasicControls::ReplayBufferStarted);
	connect(main, &OBSBasic::ReplayBufStopping, this, &OBSBasicControls::ReplayBufferStopping);
	connect(main, &OBSBasic::ReplayBufStopped, this, &OBSBasicControls::ReplayBufferStopped);

	connect(main, &OBSBasic::VirtualCamStarted, this, &OBSBasicControls::VirtualCamStarted);
	connect(main, &OBSBasic::VirtualCamStopped, this, &OBSBasicControls::VirtualCamStopped);

	connect(main, &OBSBasic::PreviewProgramModeChanged, this, &OBSBasicControls::UpdateStudioModeState);

	/* Set up enablement connection */
	connect(main, &OBSBasic::BroadcastFlowEnabled, this, &OBSBasicControls::EnableBroadcastFlow);
	connect(main, &OBSBasic::ReplayBufEnabled, this, &OBSBasicControls::EnableReplayBufferButtons);
	connect(main, &OBSBasic::VirtualCamEnabled, this, &OBSBasicControls::EnableVirtualCamButtons);
}

void OBSBasicControls::StreamingPreparing()
{
	ui->recordStreamButton->setEnabled(false);
	ui->streamButton->setEnabled(false);
	ui->streamButton->setText(QTStr("Basic.Main.PreparingStream"));
}

void OBSBasicControls::StreamingStarting(bool broadcastAutoStart)
{
	ui->streamButton->setText(QTStr("Basic.Main.Connecting"));

	if (!broadcastAutoStart) {
		// well, we need to disable button while stream is not active
		ui->broadcastButton->setEnabled(false);

		ui->broadcastButton->setText(QTStr("Basic.Main.StartBroadcast"));

		ui->broadcastButton->setProperty("broadcastState", "ready");
		ui->broadcastButton->style()->unpolish(ui->broadcastButton);
		ui->broadcastButton->style()->polish(ui->broadcastButton);
	}
}

void OBSBasicControls::StreamingStarted(bool withDelay)
{
	ui->recordStreamButton->setEnabled(true);
	ui->streamButton->setEnabled(true);
	setClasses(ui->streamButton, "state-active");
	ui->streamButton->setText(QTStr("Basic.Main.StopStreaming"));

	if (withDelay) {
		ui->streamButton->setMenu(streamButtonMenu.get());
		startStreamAction->setVisible(false);
		stopStreamAction->setVisible(true);
	}
}

void OBSBasicControls::StreamingStopping()
{
	ui->recordStreamButton->setEnabled(false);
	ui->streamButton->setText(QTStr("Basic.Main.StoppingStreaming"));
}

void OBSBasicControls::StreamingStopped(bool withDelay)
{
	ui->recordStreamButton->setEnabled(true);
	ui->streamButton->setEnabled(true);
	setClasses(ui->streamButton, "");
	ui->streamButton->setText(QTStr("Basic.Main.StartStreaming"));

	if (withDelay) {
		if (!ui->streamButton->menu()) {
			ui->streamButton->setMenu(streamButtonMenu.get());
		}

		startStreamAction->setVisible(true);
		stopStreamAction->setVisible(false);
	} else {
		ui->streamButton->setMenu(nullptr);
	}
}

void OBSBasicControls::BroadcastStreamReady(bool ready)
{
	setClasses(ui->broadcastButton, ready ? "state-active" : "");
}

void OBSBasicControls::BroadcastStreamActive()
{
	ui->broadcastButton->setEnabled(true);
}

void OBSBasicControls::BroadcastStreamStarted(bool autoStop)
{
	ui->broadcastButton->setText(QTStr(autoStop ? "Basic.Main.AutoStopEnabled" : "Basic.Main.StopBroadcast"));
	if (autoStop) {
		ui->broadcastButton->setEnabled(false);
	}

	ui->broadcastButton->setProperty("broadcastState", "active");
	ui->broadcastButton->style()->unpolish(ui->broadcastButton);
	ui->broadcastButton->style()->polish(ui->broadcastButton);
}

void OBSBasicControls::RecordingStarted(bool pausable)
{
	setClasses(ui->recordButton, "state-active");
	ui->recordButton->setText(QTStr("Basic.Main.StopRecording"));

	if (pausable) {
		ui->pauseRecordButton->setVisible(pausable);
		RecordingUnpaused();
	}
}

void OBSBasicControls::RecordingPaused()
{
	QString text = QTStr("Basic.Main.UnpauseRecording");

	setClasses(ui->pauseRecordButton, "icon-media-pause state-active");
	ui->pauseRecordButton->setAccessibleName(text);
	ui->pauseRecordButton->setToolTip(text);

	ui->saveReplayButton->setEnabled(false);
}

void OBSBasicControls::RecordingUnpaused()
{
	QString text = QTStr("Basic.Main.PauseRecording");

	setClasses(ui->pauseRecordButton, "icon-media-pause");
	ui->pauseRecordButton->setAccessibleName(text);
	ui->pauseRecordButton->setToolTip(text);

	ui->saveReplayButton->setEnabled(true);
}

void OBSBasicControls::RecordingStopping()
{
	ui->recordStreamButton->setEnabled(false);
	ui->recordButton->setText(QTStr("Basic.Main.StoppingRecording"));
}

void OBSBasicControls::RecordingStopped()
{
	ui->recordStreamButton->setEnabled(true);
	setClasses(ui->recordButton, "");
	ui->recordButton->setText(QTStr("Basic.Main.StartRecording"));

	ui->pauseRecordButton->setVisible(false);
}

void OBSBasicControls::ReplayBufferStarted()
{
	setClasses(ui->replayBufferButton, "state-active");
	ui->replayBufferButton->setText(QTStr("Basic.Main.StopReplayBuffer"));

	ui->saveReplayButton->setVisible(true);
}

void OBSBasicControls::ReplayBufferStopping()
{
	ui->replayBufferButton->setText(QTStr("Basic.Main.StoppingReplayBuffer"));
}

void OBSBasicControls::ReplayBufferStopped()
{
	setClasses(ui->replayBufferButton, "");
	ui->replayBufferButton->setText(QTStr("Basic.Main.StartReplayBuffer"));

	ui->saveReplayButton->setVisible(false);
}

void OBSBasicControls::VirtualCamStarted()
{
	setClasses(ui->virtualCamButton, "state-active");
	ui->virtualCamButton->setText(QTStr("Basic.Main.StopVirtualCam"));
}

void OBSBasicControls::VirtualCamStopped()
{
	setClasses(ui->virtualCamButton, "");
	ui->virtualCamButton->setText(QTStr("Basic.Main.StartVirtualCam"));
}

void OBSBasicControls::UpdateStudioModeState(bool enabled)
{
	setClasses(ui->modeSwitch, enabled ? "state-active" : "");
}

void OBSBasicControls::EnableBroadcastFlow(bool enabled)
{
	ui->broadcastButton->setVisible(enabled);
	ui->broadcastButton->setEnabled(enabled);

	ui->broadcastButton->setText(QTStr("Basic.Main.SetupBroadcast"));

	ui->broadcastButton->setProperty("broadcastState", "idle");
	ui->broadcastButton->style()->unpolish(ui->broadcastButton);
	ui->broadcastButton->style()->polish(ui->broadcastButton);
}

void OBSBasicControls::EnableReplayBufferButtons(bool enabled)
{
	ui->replayBufferButton->setVisible(enabled);
}

void OBSBasicControls::EnableVirtualCamButtons()
{
	ui->virtualCamButton->setVisible(true);
	ui->virtualCamConfigButton->setVisible(true);
}

bool OBSBasicControls::SaveSchedule()
{
	QJsonArray entries;
	for (const auto &entry : schedules)
		entries.append(entry.ToJson());
	const auto json = QJsonDocument(entries).toJson(QJsonDocument::Compact);
	const char *oldValue = config_get_string(App()->GetUserConfig(), "OutputSchedule", "EntriesV2");
	const bool hadValue = oldValue != nullptr;
	const QByteArray previous = oldValue ? QByteArray(oldValue) : QByteArray();
	config_set_string(App()->GetUserConfig(), "OutputSchedule", "EntriesV2", json.constData());
	if (config_save_safe(App()->GetUserConfig(), "tmp", nullptr) != 0) {
		if (hadValue)
			config_set_string(App()->GetUserConfig(), "OutputSchedule", "EntriesV2", previous.constData());
		else
			config_remove_value(App()->GetUserConfig(), "OutputSchedule", "EntriesV2");
		blog(LOG_ERROR, "Unable to save output schedule");
		return false;
	}
	return true;
}

void OBSBasicControls::CheckSchedule()
{
	const auto now = QDateTime::currentDateTimeUtc();
	// Only dispatch recent occurrences; do not replay days missed while asleep.
	const bool due = ConsumeDueOutputSchedules(schedules, lastScheduleCheck, now);
	lastScheduleCheck = now;
	// Save all simultaneous occurrences before dispatch, including modal reentry.
	if (due && SaveSchedule())
		emit ScheduledStart();
}

void OBSBasicControls::OpenSchedule()
{
	ShowOutputScheduleDialog(this, schedules, [this](const QList<OutputSchedule> &updated) {
		const auto previous = schedules;
		schedules = updated;
		if (!SaveSchedule()) {
			schedules = previous;
			return false;
		}
		return true;
	}, [](const char *key) { return QTStr(key); });
}

void OBSBasicControls::UpdateSession()
{
	const bool active = sessionStreaming || sessionRecording;
	if (active && !sessionClock.isValid()) {
		sessionClock.start();
		sessionTime->setText("00:00:00");
	} else if (!active) {
		sessionClock.invalidate();
	}
	sessionStatus->setText(QTStr(sessionStreaming && sessionRecording ? "Nova.LiveRecording"
				   : sessionStreaming ? "Nova.Live"
				   : sessionRecording ? "Nova.Recording" : "Nova.Ready"));
}
