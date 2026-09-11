#include "OBSBasicControls.hpp"
#include "OBSBasic.hpp"
#include "qt-wrappers.hpp"

#include <QDateTimeEdit>
#include <QDialogButtonBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QListWidget>
#include <QTimer>
#include <algorithm>

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
	const char *saved = config_get_string(App()->GetUserConfig(), "OutputSchedule", "Starts");
	const auto entries = QJsonDocument::fromJson(saved ? QByteArray(saved) : QByteArray()).array();
	for (const auto &entry : entries) {
		const auto time = QDateTime::fromString(entry.toString(), Qt::ISODateWithMs);
		if (time.isValid() && time > lastScheduleCheck && !scheduledStarts.contains(time))
			scheduledStarts.append(time.toUTC());
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
	std::sort(scheduledStarts.begin(), scheduledStarts.end());
	for (const auto &time : scheduledStarts)
		entries.append(time.toUTC().toString(Qt::ISODateWithMs));
	const auto json = QJsonDocument(entries).toJson(QJsonDocument::Compact);
	const char *oldValue = config_get_string(App()->GetUserConfig(), "OutputSchedule", "Starts");
	const QByteArray previous = oldValue ? QByteArray(oldValue) : QByteArray();
	config_set_string(App()->GetUserConfig(), "OutputSchedule", "Starts", json.constData());
	if (config_save_safe(App()->GetUserConfig(), "tmp", nullptr) != 0) {
		config_set_string(App()->GetUserConfig(), "OutputSchedule", "Starts", previous.constData());
		blog(LOG_ERROR, "Unable to save output schedule");
		return false;
	}
	return true;
}

void OBSBasicControls::CheckSchedule()
{
	const auto now = QDateTime::currentDateTimeUtc();
	bool changed = false;
	bool due = false;
	for (auto it = scheduledStarts.begin(); it != scheduledStarts.end();) {
		if (*it <= now) {
			// Expired starts are consumed, never replayed after a long sleep.
			due |= *it > lastScheduleCheck && it->secsTo(now) < 60;
			it = scheduledStarts.erase(it);
			changed = true;
		} else {
			++it;
		}
	}
	lastScheduleCheck = now;
	// Persist consumption before dispatch: modal output errors may reenter the event loop.
	if (changed && SaveSchedule() && due)
		emit ScheduledStart();
}

void OBSBasicControls::OpenSchedule()
{
	QDialog dialog(this);
	dialog.setWindowTitle(QTStr("Basic.Main.Schedule"));
	auto *layout = new QVBoxLayout(&dialog);
	auto *help = new QLabel(QTStr("Basic.Main.Schedule.Help"), &dialog);
	help->setWordWrap(true);
	layout->addWidget(help);
	auto *list = new QListWidget(&dialog);
	layout->addWidget(list);
	auto refresh = [this, list] {
		list->clear();
		for (const auto &time : scheduledStarts) {
			auto *item = new QListWidgetItem(time.toLocalTime().toString("yyyy-MM-dd HH:mm:ss t"), list);
			item->setData(Qt::UserRole, time);
		}
	};
	refresh();
	auto *date = new QDateTimeEdit(QDateTime::currentDateTime().addSecs(300), &dialog);
	date->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
	date->setCalendarPopup(true);
	layout->addWidget(date);
	auto *add = new QPushButton(QTStr("Basic.Main.Schedule.Add"), &dialog);
	auto *remove = new QPushButton(QTStr("Basic.Main.Schedule.Remove"), &dialog);
	layout->addWidget(add);
	layout->addWidget(remove);
	connect(add, &QPushButton::clicked, &dialog, [&, this] {
		const auto time = date->dateTime().toUTC();
		if (!time.isValid() || time <= QDateTime::currentDateTimeUtc() || scheduledStarts.contains(time)) {
			QMessageBox::warning(&dialog, dialog.windowTitle(), QTStr("Basic.Main.Schedule.Invalid"));
			return;
		}
		const auto previous = scheduledStarts;
		scheduledStarts.append(time);
		if (!SaveSchedule()) {
			scheduledStarts = previous;
			QMessageBox::warning(&dialog, dialog.windowTitle(), QTStr("Basic.Main.Schedule.SaveFailed"));
		}
		refresh();
	});
	connect(remove, &QPushButton::clicked, &dialog, [&, this] {
		if (auto *item = list->currentItem()) {
			const auto previous = scheduledStarts;
			scheduledStarts.removeAll(item->data(Qt::UserRole).toDateTime());
			if (!SaveSchedule()) {
				scheduledStarts = previous;
				QMessageBox::warning(&dialog, dialog.windowTitle(), QTStr("Basic.Main.Schedule.SaveFailed"));
			}
			refresh();
		}
	});
	QTimer refreshTimer;
	connect(&refreshTimer, &QTimer::timeout, &dialog, [refresh, list, this] {
		if (list->count() != scheduledStarts.size())
			refresh();
	});
	refreshTimer.start(1000);
	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
	connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
	layout->addWidget(buttons);
	dialog.resize(560, 420);
	dialog.exec();
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
