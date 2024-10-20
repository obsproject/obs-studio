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

#pragma once

#include "ui_OBSPermissions.h"
#include "platform.hpp"

#define MACOS_PERMISSIONS_DIALOG_VERSION 1

class OBSPermissions : public QDialog {
	Q_OBJECT

private:
	std::unique_ptr<Ui::OBSPermissions> ui;
	void SetStatus(QPushButton *btn, MacPermissionStatus status, const QString &preference);

public:
	OBSPermissions(QWidget *parent, MacPermissionStatus capture, MacPermissionStatus video,
		       MacPermissionStatus audio, MacPermissionStatus accessibility);

private slots:
	void on_capturePermissionButton_clicked();
	void on_videoPermissionButton_clicked();
	void on_audioPermissionButton_clicked();
	void on_inputMonitoringPermissionButton_clicked();
	void on_accessibilityPermissionButton_clicked();
	void on_continueButton_clicked();
};
