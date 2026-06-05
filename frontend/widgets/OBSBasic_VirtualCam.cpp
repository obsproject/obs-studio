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

void OBSBasic::SetupVirtualCam()
{
	virtualCam.reset(new OutputObj(VIRTUAL_CAM_ID, "virtualcam_output", nullptr));

	connect(virtualCam.data(), &OutputObj::started, this, &OBSBasic::OnVirtualCamStart);
	connect(virtualCam.data(), &OutputObj::stopped, this, &OBSBasic::OnVirtualCamStop);
}

void OBSBasic::StartVirtualCam()
{
	if (!virtualCam)
		return;
	if (disableOutputsRef)
		return;

	SaveProject();

	bool success = virtualCam->start();

	if (!success) {
		QString errorReason;

		const char *error = obs_output_get_last_error(virtualCam->getOutput());
		if (error) {
			errorReason = QT_UTF8(error);
		} else {
			errorReason = QTStr("Output.StartFailedGeneric");
		}

		QMessageBox::critical(this, QTStr("Output.StartVirtualCamFailed"), errorReason);
	}
}

void OBSBasic::StopVirtualCam()
{
	if (!virtualCam)
		return;
	SaveProject();
	virtualCam->stop();
	OnDeactivate();
}

void OBSBasic::OnVirtualCamStart()
{
	emit VirtualCamStarted();

	if (sysTrayVirtualCam)
		sysTrayVirtualCam->setText(QTStr("Basic.Main.StopVirtualCam"));

	OnEvent(OBS_FRONTEND_EVENT_VIRTUALCAM_STARTED);

	OnActivate();

	blog(LOG_INFO, VIRTUAL_CAM_START);
}

void OBSBasic::OnVirtualCamStop(int)
{
	emit VirtualCamStopped();

	if (sysTrayVirtualCam)
		sysTrayVirtualCam->setText(QTStr("Basic.Main.StartVirtualCam"));

	OnEvent(OBS_FRONTEND_EVENT_VIRTUALCAM_STOPPED);

	blog(LOG_INFO, VIRTUAL_CAM_STOP);

	OnDeactivate();
}

void OBSBasic::VirtualCamActionTriggered()
{
	if (virtualCam->isActive()) {
		StopVirtualCam();
	} else {
		if (!UIValidation::NoSourcesConfirmation(this))
			return;

		StartVirtualCam();
	}
}

void OBSBasic::OpenVirtualCamConfig()
{
	OBSBasicVCamConfig dialog(vcamConfig, VirtualCamActive(), this);
	connect(&dialog, &OBSBasicVCamConfig::Accepted, this, &OBSBasic::UpdateVirtualCamConfig);
	dialog.exec();
}

void OBSBasic::UpdateVirtualCamConfig(const VCamConfig &config)
{
	vcamConfig = config;

	OBSSourceAutoRelease source;

	switch (vcamConfig.type) {
	case VCamOutputType::Invalid:
	case VCamOutputType::ProgramView:
		virtualCam->setType(OutputType::Program);
		break;
	case VCamOutputType::PreviewOutput: {
		virtualCam->setType(OutputType::Preview);
		break;
	}
	case VCamOutputType::SceneOutput:
		virtualCam->setType(OutputType::Scene);
		source = obs_get_source_by_name(vcamConfig.scene.c_str());
		break;
	case VCamOutputType::SourceOutput:
		virtualCam->setType(OutputType::Source);
		source = obs_get_source_by_name(vcamConfig.source.c_str());
	}

	virtualCam->setSource(source.Get());
	virtualCam->update();
}

bool OBSBasic::VirtualCamActive()
{
	return virtualCam ? virtualCam->isActive() : false;
}

OBSOutput OBSBasic::GetVirtualCamOutput()
{
	return virtualCam ? virtualCam->getOutput() : nullptr;
}
