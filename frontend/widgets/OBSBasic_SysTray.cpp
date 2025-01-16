/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>
                          Zachary Lund <admin@computerquip.com>
                          Philippe Groarke <philippe.groarke@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "OBSBasic.hpp"

extern bool opt_minimize_tray;

void OBSBasic::SystemTrayInit()
{
#ifdef __APPLE__
	QIcon trayIconFile = QIcon(":/res/images/obs_macos.svg");
	trayIconFile.setIsMask(true);
#else
	QIcon trayIconFile = QIcon(":/res/images/obs.png");
#endif
	trayIcon.reset(new QSystemTrayIcon(QIcon::fromTheme("obs-tray", trayIconFile), this));
	trayIcon->setToolTip("OBS Studio");

	showHide = new QAction(QTStr("Basic.SystemTray.Show"), trayIcon.data());
	sysTrayStream =
		new QAction(StreamingActive() ? QTStr("Basic.Main.StopStreaming") : QTStr("Basic.Main.StartStreaming"),
			    trayIcon.data());
	sysTrayRecord =
		new QAction(RecordingActive() ? QTStr("Basic.Main.StopRecording") : QTStr("Basic.Main.StartRecording"),
			    trayIcon.data());
	sysTrayReplayBuffer = new QAction(ReplayBufferActive() ? QTStr("Basic.Main.StopReplayBuffer")
							       : QTStr("Basic.Main.StartReplayBuffer"),
					  trayIcon.data());
	sysTrayVirtualCam = new QAction(VirtualCamActive() ? QTStr("Basic.Main.StopVirtualCam")
							   : QTStr("Basic.Main.StartVirtualCam"),
					trayIcon.data());
	exit = new QAction(QTStr("Exit"), trayIcon.data());

	trayMenu = new QMenu;
	previewProjector = new QMenu(QTStr("PreviewProjector"));
	studioProgramProjector = new QMenu(QTStr("StudioProgramProjector"));
	AddProjectorMenuMonitors(previewProjector, this, &OBSBasic::OpenPreviewProjector);
	AddProjectorMenuMonitors(studioProgramProjector, this, &OBSBasic::OpenStudioProgramProjector);
	trayMenu->addAction(showHide);
	trayMenu->addSeparator();
	trayMenu->addMenu(previewProjector);
	trayMenu->addMenu(studioProgramProjector);
	trayMenu->addSeparator();
	trayMenu->addAction(sysTrayStream);
	trayMenu->addAction(sysTrayRecord);
	trayMenu->addAction(sysTrayReplayBuffer);
	trayMenu->addAction(sysTrayVirtualCam);
	trayMenu->addSeparator();
	trayMenu->addAction(exit);
	trayIcon->setContextMenu(trayMenu);
	trayIcon->show();

	if (outputHandler && !outputHandler->replayBuffer)
		sysTrayReplayBuffer->setEnabled(false);

	sysTrayVirtualCam->setEnabled(vcamEnabled);

	if (Active())
		OnActivate(true);

	connect(trayIcon.data(), &QSystemTrayIcon::activated, this, &OBSBasic::IconActivated);
	connect(showHide, &QAction::triggered, this, &OBSBasic::ToggleShowHide);
	connect(sysTrayStream, &QAction::triggered, this, &OBSBasic::StreamActionTriggered);
	connect(sysTrayRecord, &QAction::triggered, this, &OBSBasic::RecordActionTriggered);
	connect(sysTrayReplayBuffer.data(), &QAction::triggered, this, &OBSBasic::ReplayBufferActionTriggered);
	connect(sysTrayVirtualCam.data(), &QAction::triggered, this, &OBSBasic::VirtualCamActionTriggered);
	connect(exit, &QAction::triggered, this, &OBSBasic::close);
}

void OBSBasic::IconActivated(QSystemTrayIcon::ActivationReason reason)
{
	// Refresh projector list
	previewProjector->clear();
	studioProgramProjector->clear();
	AddProjectorMenuMonitors(previewProjector, this, &OBSBasic::OpenPreviewProjector);
	AddProjectorMenuMonitors(studioProgramProjector, this, &OBSBasic::OpenStudioProgramProjector);

#ifdef __APPLE__
	UNUSED_PARAMETER(reason);
#else
	if (reason == QSystemTrayIcon::Trigger) {
		EnablePreviewDisplay(previewEnabled && !isVisible());
		ToggleShowHide();
	}
#endif
}

void OBSBasic::SysTrayNotify(const QString &text, QSystemTrayIcon::MessageIcon n)
{
	if (trayIcon && trayIcon->isVisible() && QSystemTrayIcon::supportsMessages()) {
		QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::MessageIcon(n);
		trayIcon->showMessage("OBS Studio", text, icon, 10000);
	}
}

void OBSBasic::SystemTray(bool firstStarted)
{
	if (!QSystemTrayIcon::isSystemTrayAvailable())
		return;
	if (!trayIcon && !firstStarted)
		return;

	bool sysTrayWhenStarted = config_get_bool(App()->GetUserConfig(), "BasicWindow", "SysTrayWhenStarted");
	bool sysTrayEnabled = config_get_bool(App()->GetUserConfig(), "BasicWindow", "SysTrayEnabled");

	if (firstStarted)
		SystemTrayInit();

	if (!sysTrayEnabled) {
		trayIcon->hide();
	} else {
		trayIcon->show();
		if (firstStarted && (sysTrayWhenStarted || opt_minimize_tray)) {
			EnablePreviewDisplay(false);
#ifdef __APPLE__
			EnableOSXDockIcon(false);
#endif
			opt_minimize_tray = false;
		}
	}

	if (isVisible())
		showHide->setText(QTStr("Basic.SystemTray.Hide"));
	else
		showHide->setText(QTStr("Basic.SystemTray.Show"));
}

bool OBSBasic::sysTrayMinimizeToTray()
{
	return config_get_bool(App()->GetUserConfig(), "BasicWindow", "SysTrayMinimizeToTray");
}
