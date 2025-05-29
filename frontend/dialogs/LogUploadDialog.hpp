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

#include "ui_LogUploadDialog.h"

#include <QDialog>

#include <filesystem>

class QTimer;

namespace OBS {

enum class LogFileType;

class LogUploadDialog : public QDialog {
	Q_OBJECT

private:
	std::unique_ptr<Ui::LogUploadDialog> ui;
	std::unique_ptr<QTimer> uploadStatusTimer_;

	LogFileType uploadType_;

public:
	LogUploadDialog(QWidget *parent, LogFileType uploadType);

private slots:
	void startLogUpload();
	void handleUploadSuccess(LogFileType uploadType, const QString &fileURL);
	void handleUploadFailure(LogFileType uploadType, const QString &errorMessage);

	void copyToClipBoard() const;
	void openAnalyzeURL() const;
};
} // namespace OBS
