/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

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

#include "OBSPermissions.hpp"

#include <OBSApp.hpp>

#include "moc_OBSPermissions.cpp"

OBSPermissions::OBSPermissions(QWidget *parent, MacPermissionStatus capture, MacPermissionStatus video,
			       MacPermissionStatus audio, MacPermissionStatus accessibility)
	: QDialog(parent),
	  ui(new Ui::OBSPermissions)
{
	ui->setupUi(this);
	SetStatus(ui->capturePermissionButton, capture, QTStr("MacPermissions.Item.ScreenRecording"));
	SetStatus(ui->videoPermissionButton, video, QTStr("MacPermissions.Item.Camera"));
	SetStatus(ui->audioPermissionButton, audio, QTStr("MacPermissions.Item.Microphone"));
	SetStatus(ui->accessibilityPermissionButton, accessibility, QTStr("MacPermissions.Item.Accessibility"));
}

void OBSPermissions::SetStatus(QPushButton *btn, MacPermissionStatus status, const QString &preference)
{
	if (status == kPermissionAuthorized) {
		btn->setText(QTStr("MacPermissions.AccessGranted"));
	} else if (status == kPermissionNotDetermined) {
		btn->setText(QTStr("MacPermissions.RequestAccess"));
	} else {
		btn->setText(QTStr("MacPermissions.OpenPreferences").arg(preference));
	}
	btn->setEnabled(status != kPermissionAuthorized);
	btn->setProperty("status", status);
}

void OBSPermissions::on_capturePermissionButton_clicked()
{
	OpenMacOSPrivacyPreferences("ScreenCapture");
	RequestPermission(kScreenCapture);
}

void OBSPermissions::on_videoPermissionButton_clicked()
{
	MacPermissionStatus status = (MacPermissionStatus)ui->videoPermissionButton->property("status").toInt();
	if (status == kPermissionNotDetermined) {
		status = RequestPermission(kVideoDeviceAccess);
		SetStatus(ui->videoPermissionButton, status, QTStr("MacPermissions.Item.Camera"));
	} else {
		OpenMacOSPrivacyPreferences("Camera");
	}
}

void OBSPermissions::on_audioPermissionButton_clicked()
{
	MacPermissionStatus status = (MacPermissionStatus)ui->audioPermissionButton->property("status").toInt();
	if (status == kPermissionNotDetermined) {
		status = RequestPermission(kAudioDeviceAccess);
		SetStatus(ui->audioPermissionButton, status, QTStr("MacPermissions.Item.Microphone"));
	} else {
		OpenMacOSPrivacyPreferences("Microphone");
	}
}

void OBSPermissions::on_accessibilityPermissionButton_clicked()
{
	OpenMacOSPrivacyPreferences("Accessibility");
	RequestPermission(kAccessibility);
}

void OBSPermissions::on_continueButton_clicked()
{
	config_set_int(App()->GetAppConfig(), "General", "MacOSPermissionsDialogLastShown",
		       MACOS_PERMISSIONS_DIALOG_VERSION);
	close();
}
