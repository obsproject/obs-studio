/******************************************************************************
    Copyright (C) 2014 by Ruwen Hahn <palana@stunned.de>

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

#include "window-remux.hpp"

#include "obs-app.hpp"

#include <QCloseEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

#include "qt-wrappers.hpp"

#include <memory>
#include <cmath>

using namespace std;

OBSRemux::OBSRemux(const char *path, QWidget *parent)
	: QDialog (parent),
	  worker  (new RemuxWorker),
	  ui      (new Ui::OBSRemux),
	  recPath (path)
{
	ui->setupUi(this);

	ui->progressBar->setVisible(false);
	ui->buttonBox->button(QDialogButtonBox::Ok)->
			setEnabled(false);
	ui->targetFile->setEnabled(false);
	ui->browseTarget->setEnabled(false);

	ui->progressBar->setMinimum(0);
	ui->progressBar->setMaximum(1000);
	ui->progressBar->setValue(0);

	installEventFilter(CreateShortcutFilter());

	connect(ui->browseSource, &QPushButton::clicked,
			[&]() { BrowseInput(); });
	connect(ui->browseTarget, &QPushButton::clicked,
			[&]() { BrowseOutput(); });

	connect(ui->sourceFile, &QLineEdit::textChanged,
			this, &OBSRemux::inputChanged);

	ui->buttonBox->button(QDialogButtonBox::Ok)->
			setText(QTStr("Remux.Remux"));

	connect(ui->buttonBox->button(QDialogButtonBox::Ok),
		SIGNAL(clicked()), this, SLOT(Remux()));

	connect(ui->buttonBox->button(QDialogButtonBox::Close),
		SIGNAL(clicked()), this, SLOT(close()));

	worker->moveToThread(&remuxer);
	remuxer.start();

	//gcc-4.8 can't use QPointer<RemuxWorker> below
	RemuxWorker *worker_ = worker;
	connect(worker_, &RemuxWorker::updateProgress,
			this, &OBSRemux::updateProgress);
	connect(&remuxer, &QThread::finished, worker_, &QObject::deleteLater);
	connect(worker_, &RemuxWorker::remuxFinished,
			this, &OBSRemux::remuxFinished);
	connect(this, &OBSRemux::remux, worker_, &RemuxWorker::remux);
}

bool OBSRemux::Stop()
{
	if (!worker->job)
		return true;

	if (QMessageBox::critical(nullptr,
				QTStr("Remux.ExitUnfinishedTitle"),
				QTStr("Remux.ExitUnfinished"),
				QMessageBox::Yes | QMessageBox::No,
				QMessageBox::No) ==
			QMessageBox::Yes) {
		os_event_signal(worker->stop);
		return true;
	}

	return false;
}

OBSRemux::~OBSRemux()
{
	Stop();
	remuxer.quit();
	remuxer.wait();
}

#define RECORDING_PATTERN "(*.flv *.mp4 *.mov *.mkv *.ts *.m3u8)"

void OBSRemux::BrowseInput()
{
	QString path = ui->sourceFile->text();
	if (path.isEmpty())
		path = recPath;

	path = QFileDialog::getOpenFileName(this,
			QTStr("Remux.SelectRecording"), path,
			QTStr("Remux.OBSRecording") + QString(" ") +
			RECORDING_PATTERN);

	inputChanged(path);
}

void OBSRemux::inputChanged(const QString &path)
{
	if (!QFileInfo::exists(path)) {
		ui->buttonBox->button(QDialogButtonBox::Ok)->
			setEnabled(false);
		return;
	}

	ui->sourceFile->setText(path);
	ui->buttonBox->button(QDialogButtonBox::Ok)->
			setEnabled(true);

	QFileInfo fi(path);
	QString mp4 = fi.path() + "/" + fi.baseName() + ".mp4";
	ui->targetFile->setText(mp4);

	ui->targetFile->setEnabled(true);
	ui->browseTarget->setEnabled(true);
}

void OBSRemux::BrowseOutput()
{
	QString path(ui->targetFile->text());
	path = QFileDialog::getSaveFileName(this, QTStr("Remux.SelectTarget"),
				path, RECORDING_PATTERN);

	if (path.isEmpty())
		return;

	ui->targetFile->setText(path);
}

void OBSRemux::Remux()
{
	if (QFileInfo::exists(ui->targetFile->text()))
		if (QMessageBox::question(this, QTStr("Remux.FileExistsTitle"),
					QTStr("Remux.FileExists"),
					QMessageBox::Yes | QMessageBox::No) !=
				QMessageBox::Yes)
			return;

	media_remux_job_t mr_job = nullptr;
	if (!media_remux_job_create(&mr_job, QT_TO_UTF8(ui->sourceFile->text()),
				QT_TO_UTF8(ui->targetFile->text())))
		return;

	worker->job = job_t(mr_job, media_remux_job_destroy);
	worker->lastProgress = 0.f;

	ui->progressBar->setVisible(true);
	ui->buttonBox->button(QDialogButtonBox::Ok)->
			setEnabled(false);

	emit remux();
}

void OBSRemux::closeEvent(QCloseEvent *event)
{
	if (!Stop())
		event->ignore();
	else
		QDialog::closeEvent(event);
}

void OBSRemux::reject()
{
	if (!Stop())
		return;

	QDialog::reject();
}

void OBSRemux::updateProgress(float percent)
{
	ui->progressBar->setValue(percent * 10);
}

void OBSRemux::remuxFinished(bool success)
{
	QMessageBox::information(this, QTStr("Remux.FinishedTitle"),
			success ?
			QTStr("Remux.Finished") : QTStr("Remux.FinishedError"));

	worker->job.reset();
	ui->progressBar->setVisible(false);
	ui->buttonBox->button(QDialogButtonBox::Ok)->
			setEnabled(true);
}

RemuxWorker::RemuxWorker()
{
	os_event_init(&stop, OS_EVENT_TYPE_MANUAL);
}

RemuxWorker::~RemuxWorker()
{
	os_event_destroy(stop);
}

void RemuxWorker::UpdateProgress(float percent)
{
	if (abs(lastProgress - percent) < 0.1f)
		return;

	emit updateProgress(percent);
	lastProgress = percent;
}

void RemuxWorker::remux()
{
	auto callback = [](void *data, float percent)
	{
		auto rw = static_cast<RemuxWorker*>(data);
		rw->UpdateProgress(percent);
		return !!os_event_try(rw->stop);
	};

	bool success = media_remux_job_process(job.get(), callback, this);

	emit remuxFinished(os_event_try(stop) && success);
}
