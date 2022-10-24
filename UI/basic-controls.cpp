#include "basic-controls.hpp"

#include "window-basic-main.hpp"

OBSBasicControls::OBSBasicControls(OBSBasic *main_)
	: QFrame(nullptr), main(main_), ui(new Ui::OBSBasicControls)
{
	ui->setupUi(this);

	// Stream button setup
	connect(ui->streamButton, &QPushButton::clicked, main,
		&OBSBasic::StreamButtonClicked);

	// Broadcast button setup
	ui->broadcastButton->setVisible(false);
	connect(ui->broadcastButton, &QPushButton::clicked, main,
		&OBSBasic::BroadcastButtonClicked);
	connect(main, &OBSBasic::BroadcastReady, this,
		[this]() { this->ui->broadcastButton->setChecked(true); });
	connect(main, &OBSBasic::BroadcastStartAborted, this,
		[this]() { this->ui->broadcastButton->setChecked(false); });
	connect(main, &OBSBasic::BroadcastStopAborted, this,
		[this]() { this->ui->broadcastButton->setChecked(true); });
	connect(main, &OBSBasic::BroadcastActionEnabled, this,
		[this]() { this->ui->broadcastButton->setEnabled(true); });

	// Record button setup
	connect(ui->recordButton, &OBSBasicControlsButton::ResizeEvent, this,
		&OBSBasicControls::ResizePauseButton);
	connect(ui->recordButton, &QPushButton::clicked, main,
		&OBSBasic::RecordButtonClicked);

	// Mode switch button setup
	connect(ui->modeSwitch, &QPushButton::clicked, main,
		&OBSBasic::ModeSwitchClicked);
	connect(main, &OBSBasic::PreviewProgramModeChanged, ui->modeSwitch,
		&QPushButton::setChecked);

	// Settings button setup
	connect(ui->settingsButton, &QPushButton::clicked, main,
		&OBSBasic::on_action_Settings_triggered);

	// Exit button setup
	connect(ui->exitButton, &QPushButton::clicked, main, &OBSBasic::close);

	// Setup streaming connections
	connect(main, &OBSBasic::StreamingStarting, this,
		&OBSBasicControls::SetStartingStreamingState);
	connect(main, &OBSBasic::StreamingStartAborted, this,
		[this]() { this->ui->streamButton->setChecked(false); });
	connect(main, &OBSBasic::StreamingStarted, this,
		&OBSBasicControls::SetStreamingStartedState);
	connect(main, &OBSBasic::StreamingStopping, this,
		&OBSBasicControls::SetStoppingStreamingState);
	connect(main, &OBSBasic::StreamingStopAborted, this,
		[this]() { this->ui->streamButton->setChecked(true); });
	connect(main, &OBSBasic::StreamingStopped, this,
		&OBSBasicControls::SetStreamingStoppedState);

	// Setup broadcast connection
	connect(main, &OBSBasic::BroadcastStreamStarted, this,
		&OBSBasicControls::SetBroadcastStartedState);
	connect(main, &OBSBasic::BroadcastFlowStateChanged, this,
		&OBSBasicControls::SetBroadcastFlowEnabled);

	// Setup recording connections
	connect(main, &OBSBasic::RecordingStartAborted, this,
		[this]() { this->ui->recordButton->setChecked(false); });
	connect(main, &OBSBasic::RecordingStarted, this,
		&OBSBasicControls::SetRecordingStartedState);
	connect(main, &OBSBasic::RecordingStopping, this,
		&OBSBasicControls::SetStoppingRecordingState);
	connect(main, &OBSBasic::RecordingStopAborted, this,
		[this]() { this->ui->recordButton->setChecked(true); });
	connect(main, &OBSBasic::RecordingStopped, this,
		&OBSBasicControls::SetRecordingStoppedState);

	// Setup pause/unpause recording connections
	connect(main, &OBSBasic::RecordingPaused, this,
		&OBSBasicControls::PauseToUnpauseButton);
	connect(main, &OBSBasic::RecordingUnpaused, this,
		&OBSBasicControls::UnpauseToPauseButton);

	// Setup replay buffer connection
	connect(main, &OBSBasic::ReplayBufferEnabled, this,
		[this]() { this->SetupReplayBufferButton(false); });
	connect(main, &OBSBasic::ReplayBufferDisabled, this,
		[this]() { this->SetupReplayBufferButton(true); });
	connect(main, &OBSBasic::ReplayBufferStarted, this,
		&OBSBasicControls::SetReplayBufferStartedState);
	connect(main, &OBSBasic::ReplayBufferStopping2, this,
		&OBSBasicControls::SetReplayBufferStoppingState);
	connect(main, &OBSBasic::ReplayBufferStopped, this,
		&OBSBasicControls::SetReplayBufferStoppedState);

	// Setup virtual camera connections
	connect(main, &OBSBasic::VirtualCamEnabled, this,
		&OBSBasicControls::EnableVCamButtons);
	connect(main, &OBSBasic::VirtualCamStarted, this,
		&OBSBasicControls::SetVCamStartedState);
	connect(main, &OBSBasic::VirtualCamStopped, this,
		&OBSBasicControls::SetVCamStoppedState);
}

static inline void ResizeButton(QPushButton *toResize, int height)
{
	QSize size = toResize->size();

	if (size.height() != height || size.width() != height) {
		toResize->setMinimumSize(height, height);
		toResize->setMaximumSize(height, height);
	}
}

void OBSBasicControls::ResizePauseButton()
{
	if (!pauseButton)
		return;

	int height = ui->recordButton->size().height();
	ResizeButton(pauseButton.data(), height);
}

void OBSBasicControls::ResizeSaveReplayButton()
{
	if (!saveReplayButton || !replayBufferButton)
		return;

	int height = replayBufferButton->size().height();
	ResizeButton(saveReplayButton.data(), height);
}

void OBSBasicControls::ResizeVcamConfigButton()
{
	if (!vcamConfigButton || !vcamButton)
		return;

	int height = vcamButton->size().height();
	ResizeButton(vcamConfigButton, height);
}

void OBSBasicControls::SetStreamingStoppedState(bool add_menu)
{
	ui->streamButton->setText(QTStr("Basic.Main.StartStreaming"));
	ui->streamButton->setEnabled(true);
	ui->streamButton->setChecked(false);

	if (!streamButtonMenu.isNull()) {
		if (!add_menu)
			ui->streamButton->setMenu(nullptr);
		streamButtonMenu->deleteLater();
		if (!add_menu)
			streamButtonMenu = nullptr;
	}

	if (!add_menu)
		return;

	streamButtonMenu = new QMenu();
	streamButtonMenu->addAction(QTStr("Basic.Main.StartStreaming"), main,
				    SLOT(StartStreaming()));
	streamButtonMenu->addAction(QTStr("Basic.Main.ForceStopStreaming"),
				    main, SLOT(ForceStopStreaming()));
	ui->streamButton->setMenu(streamButtonMenu);
}

void OBSBasicControls::SetStoppingStreamingState()
{
	ui->streamButton->setText(QTStr("Basic.Main.StoppingStreaming"));
}

void OBSBasicControls::SetStartingStreamingState(bool broadcast_auto_start)
{
	ui->streamButton->setText(QTStr("Basic.Main.Connecting"));
	ui->streamButton->setEnabled(false);
	ui->streamButton->setChecked(false);

	ui->broadcastButton->setChecked(false);

	if (broadcast_auto_start)
		return;

	ui->broadcastButton->setText(QTStr("Basic.Main.StartBroadcast"));
	ui->broadcastButton->setProperty("broadcastState", "ready");
	ui->broadcastButton->style()->unpolish(ui->broadcastButton);
	ui->broadcastButton->style()->polish(ui->broadcastButton);
	// well, we need to disable button while stream is not active
	ui->broadcastButton->setEnabled(false);
}

void OBSBasicControls::SetStreamingStartedState(bool add_menu)
{
	ui->streamButton->setText(QTStr("Basic.Main.StopStreaming"));
	ui->streamButton->setEnabled(true);
	ui->streamButton->setChecked(true);

	if (!add_menu)
		return;

	if (!streamButtonMenu.isNull())
		streamButtonMenu->deleteLater();

	streamButtonMenu = new QMenu();
	streamButtonMenu->addAction(QTStr("Basic.Main.StopStreaming"), main,
				    SLOT(StopStreaming()));
	streamButtonMenu->addAction(QTStr("Basic.Main.ForceStopStreaming"),
				    main, SLOT(ForceStopStreaming()));
	ui->streamButton->setMenu(streamButtonMenu);
}

void OBSBasicControls::SetBroadcastStartedState(bool auto_stop)
{
	if (!auto_stop)
		ui->broadcastButton->setText(QTStr("Basic.Main.StopBroadcast"));
	else {
		ui->broadcastButton->setText(
			QTStr("Basic.Main.AutoStopEnabled"));
		ui->broadcastButton->setEnabled(false);
	}

	ui->broadcastButton->setProperty("broadcastState", "active");
	ui->broadcastButton->style()->unpolish(ui->broadcastButton);
	ui->broadcastButton->style()->polish(ui->broadcastButton);
}

void OBSBasicControls::SetBroadcastFlowEnabled(bool enabled, bool ready)
{
	ui->broadcastButton->setEnabled(enabled);
	ui->broadcastButton->setVisible(enabled);
	ui->broadcastButton->setChecked(ready);
	ui->broadcastButton->setProperty("broadcastState", "idle");
	ui->broadcastButton->style()->unpolish(ui->broadcastButton);
	ui->broadcastButton->style()->polish(ui->broadcastButton);
	ui->broadcastButton->setText(QTStr("Basic.Main.SetupBroadcast"));
}

void OBSBasicControls::SetRecordingStoppedState()
{
	ui->recordButton->setText(QTStr("Basic.Main.StartRecording"));
	ui->recordButton->setChecked(false);

	pauseButton.reset();
}

void OBSBasicControls::SetStoppingRecordingState()
{
	ui->recordButton->setText(QTStr("Basic.Main.StoppingRecording"));
}

void OBSBasicControls::SetRecordingStartedState(bool add_pause_button)
{
	ui->recordButton->setText(QTStr("Basic.Main.StopRecording"));
	ui->recordButton->setChecked(true);

	if (!add_pause_button)
		return;

	pauseButton.reset(new QPushButton());
	pauseButton->setAccessibleName(QTStr("Basic.Main.PauseRecording"));
	pauseButton->setToolTip(QTStr("Basic.Main.PauseRecording"));
	pauseButton->setCheckable(true);
	pauseButton->setChecked(false);
	pauseButton->setProperty("themeID",
				 QVariant(QStringLiteral("pauseIconSmall")));

	QSizePolicy sp;
	sp.setHeightForWidth(true);
	pauseButton->setSizePolicy(sp);

	connect(pauseButton.data(), &QPushButton::clicked, main,
		&OBSBasic::PauseToggled);

	ui->recordingLayout->addWidget(pauseButton.data());
}

void OBSBasicControls::PauseToUnpauseButton()
{
	if (!pauseButton)
		return;

	pauseButton->setAccessibleName(QTStr("Basic.Main.UnpauseRecording"));
	pauseButton->setToolTip(QTStr("Basic.Main.UnpauseRecording"));
	pauseButton->blockSignals(true);
	pauseButton->setChecked(true);
	pauseButton->blockSignals(false);

	if (saveReplayButton)
		saveReplayButton->setEnabled(false);
}

void OBSBasicControls::UnpauseToPauseButton()
{
	if (!pauseButton)
		return;

	pauseButton->setAccessibleName(QTStr("Basic.Main.PauseRecording"));
	pauseButton->setToolTip(QTStr("Basic.Main.PauseRecording"));
	pauseButton->blockSignals(true);
	pauseButton->setChecked(false);
	pauseButton->blockSignals(false);

	if (saveReplayButton)
		saveReplayButton->setEnabled(true);
}

void OBSBasicControls::SetupReplayBufferButton(bool remove)
{
	delete replayBufferButton;
	delete replayBufferLayout;
	disconnect(main, &OBSBasic::ReplayBufferStartAborted, this, nullptr);

	if (remove)
		return;

	replayBufferButton = new OBSBasicControlsButton(
		QTStr("Basic.Main.StartReplayBuffer"), this);
	replayBufferButton->setCheckable(true);

	replayBufferButton->setSizePolicy(QSizePolicy::Ignored,
					  QSizePolicy::Fixed);

	replayBufferLayout = new QHBoxLayout(this);
	replayBufferLayout->addWidget(replayBufferButton);

	replayBufferLayout->setProperty("themeID", "replayBufferButton");

	ui->buttonsVLayout->insertLayout(2, replayBufferLayout);
	setTabOrder(ui->recordButton, replayBufferButton);
	setTabOrder(replayBufferButton,
		    vcamButton ? vcamButton : ui->modeSwitch);

	connect(replayBufferButton.data(), &OBSBasicControlsButton::ResizeEvent,
		this, &OBSBasicControls::ResizeSaveReplayButton);
	connect(replayBufferButton.data(), &QPushButton::clicked, main,
		&OBSBasic::ReplayBufferClicked);
	connect(main, &OBSBasic::ReplayBufferStartAborted, this,
		[this]() { this->replayBufferButton->setChecked(false); });
}

void OBSBasicControls::SetReplayBufferStartedState()
{
	if (!replayBufferButton || !replayBufferLayout)
		return;

	replayBufferButton->setText(QTStr("Basic.Main.StopReplayBuffer"));
	replayBufferButton->setChecked(true);

	saveReplayButton.reset(new QPushButton());
	saveReplayButton->setAccessibleName(QTStr("Basic.Main.SaveReplay"));
	saveReplayButton->setToolTip(QTStr("Basic.Main.SaveReplay"));
	saveReplayButton->setChecked(false);
	saveReplayButton->setProperty(
		"themeID", QVariant(QStringLiteral("replayIconSmall")));

	QSizePolicy sp;
	sp.setHeightForWidth(true);
	saveReplayButton->setSizePolicy(sp);

	replayBufferLayout->addWidget(saveReplayButton.data());

	setTabOrder(replayBufferButton, saveReplayButton.data());
	setTabOrder(saveReplayButton.data(),
		    vcamButton ? vcamButton : ui->modeSwitch);

	connect(saveReplayButton.data(), &QAbstractButton::clicked, main,
		&OBSBasic::ReplayBufferSave);
}

void OBSBasicControls::SetReplayBufferStoppingState()
{
	if (!replayBufferButton)
		return;

	replayBufferButton->setText(QTStr("Basic.Main.StoppingReplayBuffer"));
}

void OBSBasicControls::SetReplayBufferStoppedState()
{
	if (!replayBufferButton)
		return;

	replayBufferButton->setText(QTStr("Basic.Main.StartReplayBuffer"));
	replayBufferButton->setChecked(false);

	saveReplayButton.reset();
}

void OBSBasicControls::EnableVCamButtons()
{
	vcamButton = new OBSBasicControlsButton(
		QTStr("Basic.Main.StartVirtualCam"), this);
	vcamButton->setCheckable(true);
	vcamButton->setProperty("themeID", "vcamButton");

	vcamButton->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

	vcamButtonsLayout = new QHBoxLayout(this);
	vcamButtonsLayout->addWidget(vcamButton);

	ui->buttonsVLayout->insertLayout(2, vcamButtonsLayout);

	vcamConfigButton = new QPushButton;
	vcamConfigButton->setAccessibleName(
		QTStr("Basic.Main.VirtualCamConfig"));
	vcamConfigButton->setToolTip(QTStr("Basic.Main.VirtualCamConfig"));
	vcamConfigButton->setChecked(false);
	vcamConfigButton->setProperty(
		"themeID", QVariant(QStringLiteral("configIconSmall")));

	QSizePolicy sp;
	sp.setHeightForWidth(true);
	vcamConfigButton->setSizePolicy(sp);

	vcamButtonsLayout->addWidget(vcamConfigButton);

	setTabOrder(replayBufferButton ? replayBufferButton.data()
				       : ui->recordButton,
		    vcamButton);
	setTabOrder(vcamButton, vcamConfigButton);
	setTabOrder(vcamConfigButton, ui->modeSwitch);

	connect(vcamButton.data(), &OBSBasicControlsButton::ResizeEvent, this,
		&OBSBasicControls::ResizeVcamConfigButton);
	connect(vcamButton.data(), &QPushButton::clicked, main,
		&OBSBasic::VCamButtonClicked);
	connect(main, &OBSBasic::VirtualCamStartAborted, this,
		[this]() { vcamButton->setChecked(false); });
	connect(vcamConfigButton, &QPushButton::clicked, main,
		&OBSBasic::VCamConfigButtonClicked);
}

void OBSBasicControls::SetVCamStoppedState()
{
	if (!vcamButton)
		return;

	vcamButton->setText(QTStr("Basic.Main.StartVirtualCam"));
	vcamButton->setChecked(false);
}

void OBSBasicControls::SetVCamStartedState()
{
	if (!vcamButton)
		return;

	vcamButton->setText(QTStr("Basic.Main.StopVirtualCam"));
	vcamButton->setChecked(true);
}
