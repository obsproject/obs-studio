#include "system-tray-icon.hpp"
#include "window-basic-main.hpp"

#include <QAction>

OBSSystemTrayIcon::OBSSystemTrayIcon(OBSBasic *main) : QSystemTrayIcon(nullptr)
{
#ifdef __APPLE__
	QIcon trayIconFile = QIcon(":/res/images/obs_macos.svg");
	trayIconFile.setIsMask(true);
#else
	QIcon trayIconFile = QIcon(":/res/images/obs.png");
#endif
	setIcon(QIcon::fromTheme("obs-tray", trayIconFile));
	setToolTip("OBS Studio");

	menu.reset(new QMenu());

	showHide = menu->addAction(
		main->isVisible() ? QTStr("Basic.SystemTray.Hide")
				  : QTStr("Basic.SystemTray.Show"),
		this, [this]() { emit VisibilityActionTriggered(); });
	menu->addSeparator();

	previewProjector.reset(new QMenu(QTStr("PreviewProjector")));
	studioProgramProjector.reset(
		new QMenu(QTStr("StudioProgramProjector")));

	main->AddProjectorMenuMonitors(previewProjector.data(), main,
				       &OBSBasic::OpenPreviewProjector);
	main->AddProjectorMenuMonitors(studioProgramProjector.data(), main,
				       &OBSBasic::OpenStudioProgramProjector);
	menu->addMenu(previewProjector.data());
	menu->addMenu(studioProgramProjector.data());
	menu->addSeparator();

	stream = menu->addAction(main->StreamingActive()
					 ? QTStr("Basic.Main.StopStreaming")
					 : QTStr("Basic.Main.StartStreaming"),
				 main,
				 [this]() { emit StreamActionTriggered(); });
	record = menu->addAction(main->RecordingActive()
					 ? QTStr("Basic.Main.StopRecording")
					 : QTStr("Basic.Main.StartRecording"),
				 main,
				 [this]() { emit RecordActionTriggered(); });
	replay = menu->addAction(
		main->ReplayBufferActive()
			? QTStr("Basic.Main.StopReplayBuffer")
			: QTStr("Basic.Main.StartReplayBuffer"),
		main, [this]() { emit ReplayBufferActionTriggered(); });
	virtualCam = menu->addAction(
		main->VirtualCamActive() ? QTStr("Basic.Main.StopVirtualCam")
					 : QTStr("Basic.Main.StartVirtualCam"),
		main, [this]() { emit VirtualCamActionTriggered(); });
	menu->addSeparator();
	menu->addAction(QTStr("Exit"), main, &OBSBasic::close);

	setContextMenu(menu.data());

	if (main->Active())
		OnActivate(true);

	connect(this, &QSystemTrayIcon::activated, this,
		&OBSSystemTrayIcon::IconActivated);

	/* Set up state update connections */
	connect(main, &OBSBasic::VisibilityChanged, this,
		&OBSSystemTrayIcon::VisibilityChanged);

	connect(main, &OBSBasic::StreamingPreparing, this,
		&OBSSystemTrayIcon::StreamingPreparing);
	connect(main, &OBSBasic::StreamingStarting, this,
		&OBSSystemTrayIcon::StreamingStarting);
	connect(main, &OBSBasic::StreamingStarted, this,
		&OBSSystemTrayIcon::StreamingStarted);
	connect(main, &OBSBasic::StreamingStopping, this,
		&OBSSystemTrayIcon::StreamingStopping);
	connect(main, &OBSBasic::StreamingStopped, this,
		&OBSSystemTrayIcon::StreamingStopped);

	connect(main, &OBSBasic::RecordingStarted, this,
		&OBSSystemTrayIcon::RecordingStarted);
	connect(main, &OBSBasic::RecordingPaused, this,
		&OBSSystemTrayIcon::RecordingPaused);
	connect(main, &OBSBasic::RecordingUnpaused, this,
		&OBSSystemTrayIcon::RecordingUnpaused);
	connect(main, &OBSBasic::RecordingStopping, this,
		&OBSSystemTrayIcon::RecordingStopping);
	connect(main, &OBSBasic::RecordingStopped, this,
		&OBSSystemTrayIcon::RecordingStopped);

	connect(main, &OBSBasic::ReplayBufStarted, this,
		&OBSSystemTrayIcon::ReplayBufferStarted);
	connect(main, &OBSBasic::ReplayBufferStopping, this,
		&OBSSystemTrayIcon::ReplayBufferStopping);
	connect(main, &OBSBasic::ReplayBufStopped, this,
		&OBSSystemTrayIcon::ReplayBufferStopped);

	connect(main, &OBSBasic::VirtualCamStarted, this,
		&OBSSystemTrayIcon::VirtualCamStarted);
	connect(main, &OBSBasic::VirtualCamStopped, this,
		&OBSSystemTrayIcon::VirtualCamStopped);

	/* Set up enablement connection */
	connect(main, &OBSBasic::ReplayBufEnabled, this,
		&OBSSystemTrayIcon::UpdateReplayBuffer);
	connect(main, &OBSBasic::VirtualCamEnabled, this,
		&OBSSystemTrayIcon::EnableVirtualCam);

	show();
}

OBSSystemTrayIcon::~OBSSystemTrayIcon() {}

void OBSSystemTrayIcon::OnActivate(bool force)
{
	if (!force && OBSBasic::Get()->Active())
		return;

#ifdef __APPLE__
	QIcon trayIconFile = QIcon(":/res/images/tray_active_macos.svg");
	trayIconFile.setIsMask(true);
#else
	QIcon trayIconFile = QIcon(":/res/images/tray_active.png");
#endif
	setIcon(QIcon::fromTheme("obs-tray-active", trayIconFile));
}

void OBSSystemTrayIcon::OnDeactivate()
{
	if (!OBSBasic::Get()->Active())
		return;

#ifdef __APPLE__
	QIcon trayIconFile = QIcon(":/res/images/obs_macos.svg");
	trayIconFile.setIsMask(true);
#else
	QIcon trayIconFile = QIcon(":/res/images/obs.png");
#endif
	setIcon(QIcon::fromTheme("obs-tray", trayIconFile));
}

// Visibility
void OBSSystemTrayIcon::VisibilityChanged(bool visible)
{
	showHide->setText(visible ? QTStr("Basic.SystemTray.Hide")
				  : QTStr("Basic.SystemTray.Show"));
}

// Streaming
void OBSSystemTrayIcon::StreamingPreparing()
{
	stream->setEnabled(false);
	stream->setText(QTStr("Basic.Main.PreparingStream"));
}

void OBSSystemTrayIcon::StreamingStarting(bool)
{
	stream->setText(QTStr("Basic.Main.Connecting"));
}

void OBSSystemTrayIcon::StreamingStarted(bool)
{
	stream->setEnabled(true);
	stream->setText(QTStr("Basic.Main.StopStreaming"));
	OnActivate();
}

void OBSSystemTrayIcon::StreamingStopping()
{
	stream->setText(QTStr("Basic.Main.StoppingStreaming"));
}

void OBSSystemTrayIcon::StreamingStopped(bool)
{
	stream->setText(QTStr("Basic.Main.StartStreaming"));
	OnDeactivate();
}

// Recording
void OBSSystemTrayIcon::RecordingStarted(bool)
{
	record->setText(QTStr("Basic.Main.StopRecording"));
	OnActivate();
}

void OBSSystemTrayIcon::RecordingStopping()
{
	record->setText(QTStr("Basic.Main.StoppingRecording"));
}

void OBSSystemTrayIcon::RecordingStopped()
{
	record->setText(QTStr("Basic.Main.StartRecording"));
	OnDeactivate();
}

void OBSSystemTrayIcon::RecordingPaused()
{
#ifdef __APPLE__
	QIcon trayIconFile = QIcon(":/res/images/obs_paused_macos.svg");
	trayIconFile.setIsMask(true);
#else
	QIcon trayIconFile = QIcon(":/res/images/obs_paused.png");
#endif
	setIcon(QIcon::fromTheme("obs-tray-paused", trayIconFile));
}

void OBSSystemTrayIcon::RecordingUnpaused()
{
#ifdef __APPLE__
	QIcon trayIconFile = QIcon(":/res/images/tray_active_macos.svg");
	trayIconFile.setIsMask(true);
#else
	QIcon trayIconFile = QIcon(":/res/images/tray_active.png");
#endif
	setIcon(QIcon::fromTheme("obs-tray-active", trayIconFile));
}

// Replay Buffer
void OBSSystemTrayIcon::UpdateReplayBuffer(bool enable)
{
	replay->setEnabled(enable);
}

void OBSSystemTrayIcon::ReplayBufferStarted()
{
	replay->setText(QTStr("Basic.Main.StopReplayBuffer"));
	OnActivate();
}

void OBSSystemTrayIcon::ReplayBufferStopping()
{
	replay->setText(QTStr("Basic.Main.StoppingReplayBuffer"));
}

void OBSSystemTrayIcon::ReplayBufferStopped()
{
	replay->setText(QTStr("Basic.Main.StartReplayBuffer"));
	OnDeactivate();
}

// Virtual Camera
void OBSSystemTrayIcon::EnableVirtualCam()
{
	virtualCam->setEnabled(true);
}

void OBSSystemTrayIcon::VirtualCamStarted()
{
	virtualCam->setText(QTStr("Basic.Main.StopVirtualCam"));
	OnActivate();
}

void OBSSystemTrayIcon::VirtualCamStopped()
{
	virtualCam->setText(QTStr("Basic.Main.StartVirtualCam"));
	OnDeactivate();
}

void OBSSystemTrayIcon::IconActivated(QSystemTrayIcon::ActivationReason reason)
{
	OBSBasic *main = OBSBasic::Get();

	// Refresh projector list
	previewProjector.data()->clear();
	studioProgramProjector.data()->clear();
	main->AddProjectorMenuMonitors(previewProjector.data(), main,
				       &OBSBasic::OpenPreviewProjector);
	main->AddProjectorMenuMonitors(studioProgramProjector.data(), main,
				       &OBSBasic::OpenStudioProgramProjector);

#ifdef __APPLE__
	UNUSED_PARAMETER(reason);
#else
	if (reason == QSystemTrayIcon::Trigger)
		emit VisibilityActionTriggered();
#endif
}

void OBSSystemTrayIcon::ShowNotification(const QString &text,
					 QSystemTrayIcon::MessageIcon n)
{
	if (!supportsMessages())
		return;

	showMessage("OBS Studio", text, n, 10000);
}
