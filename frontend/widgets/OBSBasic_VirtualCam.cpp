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

#include <components/UIValidation.hpp>
#include <dialogs/OBSBasicVCamConfig.hpp>

#define VIRTUAL_CAM_START "==== Virtual Camera Start =========================================="
#define VIRTUAL_CAM_STOP "==== Virtual Camera Stop ==========================================="

void OBSBasic::StartVirtualCam()
{
	if (!outputHandler || !outputHandler->virtualCam)
		return;
	if (outputHandler->VirtualCamActive())
		return;
	if (disableOutputsRef)
		return;

	SaveProject();

	outputHandler->StartVirtualCam();
}

void OBSBasic::StopVirtualCam()
{
	if (!outputHandler || !outputHandler->virtualCam)
		return;

	SaveProject();

	if (outputHandler->VirtualCamActive())
		outputHandler->StopVirtualCam();

	OnDeactivate();
}

void OBSBasic::OnVirtualCamStart()
{
	if (!outputHandler || !outputHandler->virtualCam)
		return;

	emit VirtualCamStarted();

	if (sysTrayVirtualCam)
		sysTrayVirtualCam->setText(QTStr("Basic.Main.StopVirtualCam"));

	OnEvent(OBS_FRONTEND_EVENT_VIRTUALCAM_STARTED);

	OnActivate();

	blog(LOG_INFO, VIRTUAL_CAM_START);
}

void OBSBasic::OnVirtualCamStop(int)
{
	if (!outputHandler || !outputHandler->virtualCam)
		return;

	emit VirtualCamStopped();

	if (sysTrayVirtualCam)
		sysTrayVirtualCam->setText(QTStr("Basic.Main.StartVirtualCam"));

	OnEvent(OBS_FRONTEND_EVENT_VIRTUALCAM_STOPPED);

	blog(LOG_INFO, VIRTUAL_CAM_STOP);

	OnDeactivate();

	if (!restartingVCam)
		return;

	/* Restarting needs to be delayed to make sure that the virtual camera
	 * implementation is stopped and avoid race condition. */
	QTimer::singleShot(100, this, &OBSBasic::RestartingVirtualCam);
}

void OBSBasic::VirtualCamActionTriggered()
{
	if (outputHandler->VirtualCamActive()) {
		StopVirtualCam();
	} else {
		if (!UIValidation::NoSourcesConfirmation(this))
			return;

		StartVirtualCam();
	}
}

void OBSBasic::OpenVirtualCamConfig()
{
	OBSBasicVCamConfig dialog(vcamConfig, outputHandler->VirtualCamActive(), this);

	connect(&dialog, &OBSBasicVCamConfig::Accepted, this, &OBSBasic::UpdateVirtualCamConfig);
	connect(&dialog, &OBSBasicVCamConfig::AcceptedAndRestart, this, &OBSBasic::RestartVirtualCam);

	dialog.exec();
}

void log_vcam_changed(const VCamConfig &config, bool starting)
{
	const char *action = starting ? "Starting" : "Changing";

	switch (config.type) {
	case VCamOutputType::Invalid:
		break;
	case VCamOutputType::ProgramView:
		blog(LOG_INFO, "%s Virtual Camera output to Program", action);
		break;
	case VCamOutputType::PreviewOutput:
		blog(LOG_INFO, "%s Virtual Camera output to Preview", action);
		break;
	case VCamOutputType::SceneOutput:
		blog(LOG_INFO, "%s Virtual Camera output to Scene : %s", action, config.scene.c_str());
		break;
	case VCamOutputType::SourceOutput:
		blog(LOG_INFO, "%s Virtual Camera output to Source : %s", action, config.source.c_str());
		break;
	}
}

void OBSBasic::UpdateVirtualCamConfig(const VCamConfig &config)
{
	vcamConfig = config;

	outputHandler->UpdateVirtualCamOutputSource();
	log_vcam_changed(config, false);
}

void OBSBasic::RestartVirtualCam(const VCamConfig &config)
{
	restartingVCam = true;

	StopVirtualCam();

	vcamConfig = config;
}

void OBSBasic::RestartingVirtualCam()
{
	if (!restartingVCam)
		return;

	outputHandler->UpdateVirtualCamOutputSource();
	StartVirtualCam();
	restartingVCam = false;
}

bool OBSBasic::VirtualCamActive()
{
	if (!outputHandler)
		return false;
	return outputHandler->VirtualCamActive();
}
