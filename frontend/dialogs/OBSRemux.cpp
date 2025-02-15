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

#include "OBSRemux.hpp"

#include <utility/RemuxEntryPathItemDelegate.hpp>
#include <utility/RemuxQueueModel.hpp>
#include <utility/RemuxWorker.hpp>
#include <widgets/OBSBasic.hpp>

#include <qt-wrappers.hpp>

#include <QDirIterator>
#include <QDropEvent>
#include <QMimeData>
#include <QPushButton>

#include "moc_OBSRemux.cpp"

OBSRemux::OBSRemux(const char *path, QWidget *parent, bool autoRemux_)
	: QDialog(parent),
	  queueModel(new RemuxQueueModel),
	  worker(new RemuxWorker()),
	  ui(new Ui::OBSRemux),
	  recPath(path),
	  autoRemux(autoRemux_)
{
	setAcceptDrops(true);

	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	ui->setupUi(this);

	ui->progressBar->setVisible(false);
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)->setEnabled(false);

	if (autoRemux) {
		resize(280, 40);
		ui->tableView->hide();
		ui->buttonBox->hide();
		ui->label->hide();
	}

	ui->progressBar->setMinimum(0);
	ui->progressBar->setMaximum(1000);
	ui->progressBar->setValue(0);

	ui->tableView->setModel(queueModel);
	ui->tableView->setItemDelegateForColumn(RemuxEntryColumn::InputPath,
						new RemuxEntryPathItemDelegate(false, recPath));
	ui->tableView->setItemDelegateForColumn(RemuxEntryColumn::OutputPath,
						new RemuxEntryPathItemDelegate(true, recPath));
	ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Stretch);
	ui->tableView->horizontalHeader()->setSectionResizeMode(RemuxEntryColumn::State,
								QHeaderView::ResizeMode::Fixed);
	ui->tableView->setEditTriggers(QAbstractItemView::EditTrigger::CurrentChanged);
	ui->tableView->setTextElideMode(Qt::ElideMiddle);
	ui->tableView->setWordWrap(false);

	installEventFilter(CreateShortcutFilter());

	ui->buttonBox->button(QDialogButtonBox::Ok)->setText(QTStr("Remux.Remux"));
	ui->buttonBox->button(QDialogButtonBox::Reset)->setText(QTStr("Remux.ClearFinished"));
	ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)->setText(QTStr("Remux.ClearAll"));
	ui->buttonBox->button(QDialogButtonBox::Reset)->setDisabled(true);

	connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &OBSRemux::beginRemux);
	connect(ui->buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, this, &OBSRemux::clearFinished);
	connect(ui->buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this,
		&OBSRemux::clearAll);
	connect(ui->buttonBox->button(QDialogButtonBox::Close), &QPushButton::clicked, this, &OBSRemux::close);

	worker->moveToThread(&remuxer);
	remuxer.start();

	connect(worker.data(), &RemuxWorker::updateProgress, this, &OBSRemux::updateProgress);
	connect(&remuxer, &QThread::finished, worker.data(), &QObject::deleteLater);
	connect(worker.data(), &RemuxWorker::remuxFinished, this, &OBSRemux::remuxFinished);
	connect(this, &OBSRemux::remux, worker.data(), &RemuxWorker::remux);

	connect(queueModel.data(), &RemuxQueueModel::rowsInserted, this, &OBSRemux::rowCountChanged);
	connect(queueModel.data(), &RemuxQueueModel::rowsRemoved, this, &OBSRemux::rowCountChanged);

	QModelIndex index = queueModel->createIndex(0, 1);
	QMetaObject::invokeMethod(ui->tableView, "setCurrentIndex", Qt::QueuedConnection,
				  Q_ARG(const QModelIndex &, index));
}

bool OBSRemux::stopRemux()
{
	if (!worker->isWorking)
		return true;

	// By locking the worker thread's mutex, we ensure that its
	// update poll will be blocked as long as we're in here with
	// the popup open.
	QMutexLocker lock(&worker->updateMutex);

	bool exit = false;

	if (QMessageBox::critical(nullptr, QTStr("Remux.ExitUnfinishedTitle"), QTStr("Remux.ExitUnfinished"),
				  QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
		exit = true;
	}

	if (exit) {
		// Inform the worker it should no longer be
		// working. It will interrupt accordingly in
		// its next update callback.
		worker->isWorking = false;
	}

	return exit;
}

OBSRemux::~OBSRemux()
{
	stopRemux();
	remuxer.quit();
	remuxer.wait();
}

void OBSRemux::rowCountChanged(const QModelIndex &, int, int)
{
	// See if there are still any rows ready to remux. Change
	// the state of the "go" button accordingly.
	// There must be more than one row, since there will always be
	// at least one row for the empty insertion point.
	if (queueModel->rowCount() > 1) {
		ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
		ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)->setEnabled(true);
		ui->buttonBox->button(QDialogButtonBox::Reset)->setEnabled(queueModel->canClearFinished());
	} else {
		ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
		ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)->setEnabled(false);
		ui->buttonBox->button(QDialogButtonBox::Reset)->setEnabled(false);
	}
}

void OBSRemux::dropEvent(QDropEvent *ev)
{
	QStringList urlList;

	for (QUrl url : ev->mimeData()->urls()) {
		QFileInfo fileInfo(url.toLocalFile());

		if (fileInfo.isDir()) {
			QStringList directoryFilter;
			directoryFilter << "*.flv"
					<< "*.mp4"
					<< "*.mov"
					<< "*.mkv"
					<< "*.ts"
					<< "*.m3u8";

			QDirIterator dirIter(fileInfo.absoluteFilePath(), directoryFilter, QDir::Files,
					     QDirIterator::Subdirectories);

			while (dirIter.hasNext()) {
				urlList.append(dirIter.next());
			}
		} else {
			urlList.append(fileInfo.canonicalFilePath());
		}
	}

	if (urlList.empty()) {
		QMessageBox::information(nullptr, QTStr("Remux.NoFilesAddedTitle"), QTStr("Remux.NoFilesAdded"),
					 QMessageBox::Ok);
	} else if (!autoRemux) {
		QModelIndex insertIndex = queueModel->index(queueModel->rowCount() - 1, RemuxEntryColumn::InputPath);
		queueModel->setData(insertIndex, urlList, RemuxEntryRole::NewPathsToProcessRole);
	}
}

void OBSRemux::dragEnterEvent(QDragEnterEvent *ev)
{
	if (ev->mimeData()->hasUrls() && !worker->isWorking)
		ev->accept();
}

void OBSRemux::beginRemux()
{
	if (worker->isWorking) {
		stopRemux();
		return;
	}

	bool proceedWithRemux = true;
	QFileInfoList overwriteFiles = queueModel->checkForOverwrites();

	if (!overwriteFiles.empty()) {
		QString message = QTStr("Remux.FileExists");
		message += "\n\n";

		for (QFileInfo fileInfo : overwriteFiles)
			message += fileInfo.canonicalFilePath() + "\n";

		if (OBSMessageBox::question(this, QTStr("Remux.FileExistsTitle"), message) != QMessageBox::Yes)
			proceedWithRemux = false;
	}

	if (!proceedWithRemux)
		return;

	// Set all jobs to "pending" first.
	queueModel->beginProcessing();

	ui->progressBar->setVisible(true);
	ui->buttonBox->button(QDialogButtonBox::Ok)->setText(QTStr("Remux.Stop"));
	setAcceptDrops(false);

	remuxNextEntry();
}

void OBSRemux::AutoRemux(QString inFile, QString outFile)
{
	if (inFile != "" && outFile != "" && autoRemux) {
		ui->progressBar->setVisible(true);
		emit remux(inFile, outFile);
		autoRemuxFile = outFile;
	}
}

void OBSRemux::remuxNextEntry()
{
	worker->lastProgress = 0.f;

	QString inputPath, outputPath;
	if (queueModel->beginNextEntry(inputPath, outputPath)) {
		emit remux(inputPath, outputPath);
	} else {
		queueModel->autoRemux = autoRemux;
		queueModel->endProcessing();

		if (!autoRemux) {
			OBSMessageBox::information(this, QTStr("Remux.FinishedTitle"),
						   queueModel->checkForErrors() ? QTStr("Remux.FinishedError")
										: QTStr("Remux.Finished"));
		}

		ui->progressBar->setVisible(autoRemux);
		ui->buttonBox->button(QDialogButtonBox::Ok)->setText(QTStr("Remux.Remux"));
		ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)->setEnabled(true);
		ui->buttonBox->button(QDialogButtonBox::Reset)->setEnabled(queueModel->canClearFinished());
		setAcceptDrops(true);
	}
}

void OBSRemux::closeEvent(QCloseEvent *event)
{
	if (!stopRemux())
		event->ignore();
	else
		QDialog::closeEvent(event);
}

void OBSRemux::reject()
{
	if (!stopRemux())
		return;

	QDialog::reject();
}

void OBSRemux::updateProgress(float percent)
{
	ui->progressBar->setValue(percent * 10);
}

void OBSRemux::remuxFinished(bool success)
{
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);

	queueModel->finishEntry(success);

	if (autoRemux && autoRemuxFile != "") {
		QTimer::singleShot(3000, this, &OBSRemux::close);

		OBSBasic *main = OBSBasic::Get();
		main->ShowStatusBarMessage(QTStr("Basic.StatusBar.AutoRemuxedTo").arg(autoRemuxFile));
	}

	remuxNextEntry();
}

void OBSRemux::clearFinished()
{
	queueModel->clearFinished();
}

void OBSRemux::clearAll()
{
	queueModel->clearAll();
}
