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

#include "LogUploadDialog.hpp"

#include <OBSApp.hpp>

#include <QClipboard>
#include <QDesktopServices>
#include <QUrlQuery>

#include "moc_LogUploadDialog.cpp"

namespace OBS {
LogUploadDialog::LogUploadDialog(QWidget *parent, LogFileType uploadType)
	: QDialog(parent),
	  ui(new Ui::LogUploadDialog),
	  uploadType_(uploadType)
{
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
	ui->setupUi(this);

	ui->stackedWidget->setCurrentIndex(0);
	ui->uploadProgress->setVisible(false);

	if (uploadType_ == LogFileType::CrashLog) {
		ui->analyzeURL->hide();
		ui->description->setText(Str("LogUploadDialog.Labels.Description.CrashLog"));
	}

	connect(ui->confirmUploadButton, &QPushButton::clicked, this, &LogUploadDialog::startLogUpload);
	connect(ui->retryButton, &QPushButton::clicked, this, &LogUploadDialog::startLogUpload);
	connect(ui->copyURL, &QPushButton::clicked, this, &LogUploadDialog::copyToClipBoard);
	connect(ui->analyzeURL, &QPushButton::clicked, this, &LogUploadDialog::openAnalyzeURL);

	OBSApp *app = App();
	connect(app, &OBSApp::logUploadFinished, this, &LogUploadDialog::handleUploadSuccess);
	connect(app, &OBSApp::logUploadFailed, this, &LogUploadDialog::handleUploadFailure);

	installEventFilter(CreateShortcutFilter());

	LogFileState uploadState = app->getLogFileState(uploadType);
	switch (uploadState) {
	case LogFileState::Uploaded:
		startLogUpload();
		break;
	default:
		break;
	}
}

void LogUploadDialog::startLogUpload()
{
	if (uploadType_ == LogFileType::NoType) {
		return;
	}

	ui->confirmUploadButton->setEnabled(false);
	ui->uploadProgress->setVisible(true);
	setCursor(Qt::WaitCursor);

	OBSApp *app = App();

	switch (uploadType_) {
	case LogFileType::CrashLog:
		app->uploadLastCrashLog();
		break;
	case LogFileType::CurrentAppLog:
		app->uploadCurrentAppLog();
		break;
	case LogFileType::LastAppLog:
		app->uploadLastAppLog();
		break;
	default:
		break;
	}
}

void LogUploadDialog::handleUploadSuccess(LogFileType, const QString &fileURL)
{
	unsetCursor();
	ui->confirmUploadButton->setEnabled(true);
	ui->uploadProgress->setVisible(false);

	ui->urlEdit->setText(fileURL);
	ui->stackedWidget->setCurrentIndex(1);
}

void LogUploadDialog::handleUploadFailure(LogFileType, const QString &errorMessage)
{
	unsetCursor();
	ui->confirmUploadButton->setEnabled(true);
	ui->uploadProgress->setVisible(false);

	QString errorDescription = QTStr("LogUploadDialog.Errors.Template").arg(errorMessage);
	ui->uploadErrorMessage->setText(errorDescription);
	ui->stackedWidget->setCurrentIndex(2);
}

void LogUploadDialog::copyToClipBoard() const
{
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText(ui->urlEdit->text());
}

void LogUploadDialog::openAnalyzeURL() const
{
	QUrlQuery queryParameters;
	queryParameters.addQueryItem("log_url", QUrl::toPercentEncoding(ui->urlEdit->text()));
	QUrl analyzerUrl = QUrl("https://obsproject.com/tools/analyzer", QUrl::TolerantMode);

	analyzerUrl.setQuery(queryParameters);

	QDesktopServices::openUrl(analyzerUrl);
}
} // namespace OBS
