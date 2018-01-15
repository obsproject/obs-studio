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
#include <QItemDelegate>
#include <QLineEdit>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QPushButton>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QToolButton>

#include "qt-wrappers.hpp"

#include <memory>
#include <cmath>

using namespace std;

enum RemuxEntryColumn
{
	State,
	InputPath,
	OutputPath,

	Count
};

class ReadOnlyItemDelegate : public QStyledItemDelegate
{
public:
	virtual QWidget *createEditor(QWidget * /* parent */,
		const QStyleOptionViewItem & /* option */,
		const QModelIndex & /* index */) const override
	{
		return Q_NULLPTR;
	}
};

class RemuxEntryPathItemDelegate : public QStyledItemDelegate
{
public:

	RemuxEntryPathItemDelegate(bool isOutput, const QString &defaultPath)
		: QStyledItemDelegate(),
		  isOutput(isOutput),
		  defaultPath(defaultPath)
	{

	}

	virtual QWidget *createEditor(QWidget *parent,
			const QStyleOptionViewItem & /* option */,
			const QModelIndex &index) const override
	{
		RemuxEntryState state = index.model()
				->index(index.row(), RemuxEntryColumn::State)
				.data(Qt::UserRole).value<RemuxEntryState>();
		if (isOutput && state != Ready) {
			return Q_NULLPTR;
		}
		else if (!isOutput && state == Complete) {
			return Q_NULLPTR;
		}
		else {
			QSizePolicy buttonSizePolicy(
					QSizePolicy::Policy::Minimum,
					QSizePolicy::Policy::Expanding,
					QSizePolicy::ControlType::PushButton);

			QWidget *container = new QWidget(parent);

			auto browseCallback = [this, container]()
			{
				const_cast<RemuxEntryPathItemDelegate *>(this)
						->handleBrowse(container);
			};

			auto clearCallback = [this, container]()
			{
				const_cast<RemuxEntryPathItemDelegate *>(this)
						->handleClear(container);
			};

			QHBoxLayout *layout = new QHBoxLayout();
			layout->setMargin(0);
			layout->setSpacing(0);

			QLineEdit *text = new QLineEdit();
			text->setObjectName(QStringLiteral("text"));
			text->setSizePolicy(QSizePolicy(
					QSizePolicy::Policy::Expanding,
					QSizePolicy::Policy::Expanding,
					QSizePolicy::ControlType::LineEdit));
			layout->addWidget(text);

			QToolButton *browseButton = new QToolButton();
			browseButton->setText("...");
			browseButton->setSizePolicy(buttonSizePolicy);
			layout->addWidget(browseButton);

			container->connect(browseButton, &QToolButton::clicked,
					browseCallback);

			if (!isOutput) {
				QToolButton *clearButton = new QToolButton();
				clearButton->setText("X");
				clearButton->setSizePolicy(buttonSizePolicy);
				layout->addWidget(clearButton);

				container->connect(clearButton,
						&QToolButton::clicked,
						clearCallback);
			}

			container->setLayout(layout);
			container->setFocusProxy(text);

			return container;
		}
	}

	virtual void setEditorData(QWidget *editor, const QModelIndex &index)
			const override
	{
		QLineEdit *text = editor->findChild<QLineEdit *>();
		text->setText(index.data().toString());
	}

	virtual void setModelData(QWidget *editor,
			QAbstractItemModel *model,
			const QModelIndex &index) const override
	{
		QLineEdit *text = editor->findChild<QLineEdit *>();
		model->setData(index, text->text());
	}

	virtual void paint(QPainter *painter,
			const QStyleOptionViewItem &option,
			const QModelIndex &index) const override
	{
		RemuxEntryState state = index.model()
				->index(index.row(), RemuxEntryColumn::State)
				.data(Qt::UserRole).value<RemuxEntryState>();

		QStyleOptionViewItem localOption = option;
		initStyleOption(&localOption, index);

		if (isOutput) {
			if (state != Ready) {
				QColor background = localOption.palette
						.color(QPalette::ColorGroup::Disabled,
						QPalette::ColorRole::Background);

				localOption.backgroundBrush = QBrush(background);
			}
		}

		QApplication::style()->drawControl(QStyle::CE_ItemViewItem,
				&localOption, painter);
	}

private:
	bool isOutput;
	QString defaultPath;

	void handleBrowse(QWidget *container)
	{
		static const QString RecordingPattern =
				"(*.flv *.mp4 *.mov *.mkv *.ts *.m3u8)";

		QLineEdit *text = container->findChild<QLineEdit *>();

		QString path = text->text();
		if (path.isEmpty())
			path = defaultPath;

		if (isOutput)
			path = QFileDialog::getSaveFileName(container,
					QTStr("Remux.SelectTarget"),
					path, RecordingPattern);
		else
			path = QFileDialog::getOpenFileName(container,
					QTStr("Remux.SelectRecording"), path,
					QTStr("Remux.OBSRecording")
					+ QString(" ") + RecordingPattern);

		if (path.isEmpty())
			return;

		text->setText(path);

		emit commitData(container);
	}

	void handleClear(QWidget *container)
	{
		QLineEdit *text = container->findChild<QLineEdit *>();
		text->clear();

		emit commitData(container);
	}
};

OBSRemux::OBSRemux(const char *path, QWidget *parent)
	: QDialog    (parent),
	  worker     (new RemuxWorker),
	  ui         (new Ui::OBSRemux),
	  tableModel (new QStandardItemModel(1, RemuxEntryColumn::Count)),
	  recPath    (path)
{
	setAcceptDrops(true);

	ui->setupUi(this);

	ui->progressBar->setVisible(false);
	ui->buttonBox->button(QDialogButtonBox::Ok)->
			setEnabled(false);

	ui->progressBar->setMinimum(0);
	ui->progressBar->setMaximum(1000);
	ui->progressBar->setValue(0);

	tableModel->setHeaderData(RemuxEntryColumn::State,
			Qt::Orientation::Horizontal, "");
	tableModel->setHeaderData(RemuxEntryColumn::InputPath,
			Qt::Orientation::Horizontal,
			QTStr("Remux.SourceFile"));
	tableModel->setHeaderData(RemuxEntryColumn::OutputPath,
			Qt::Orientation::Horizontal,
			QTStr("Remux.TargetFile"));
	ui->tableView->setModel(tableModel);
	ui->tableView->setItemDelegateForColumn(RemuxEntryColumn::State,
			new ReadOnlyItemDelegate());
	ui->tableView->setItemDelegateForColumn(RemuxEntryColumn::InputPath,
			new RemuxEntryPathItemDelegate(false, recPath));
	ui->tableView->setItemDelegateForColumn(RemuxEntryColumn::OutputPath,
			new RemuxEntryPathItemDelegate(true, recPath));
	ui->tableView->horizontalHeader()->setSectionResizeMode(
			QHeaderView::ResizeMode::Stretch);
	ui->tableView->horizontalHeader()->setSectionResizeMode(
			RemuxEntryColumn::State,
			QHeaderView::ResizeMode::Fixed);
	ui->tableView->setEditTriggers(
			QAbstractItemView::EditTrigger::CurrentChanged);

	connect(tableModel, SIGNAL(itemChanged(QStandardItem*)),
			this, SLOT(inputCellChanged(QStandardItem*)));

	installEventFilter(CreateShortcutFilter());

	ui->buttonBox->button(QDialogButtonBox::Ok)->
			setText(QTStr("Remux.Remux"));
	ui->buttonBox->button(QDialogButtonBox::Reset)->
			setText(QTStr("Remux.ClearFinished"));
	ui->buttonBox->button(QDialogButtonBox::Reset)->
			setDisabled(true);

	connect(ui->buttonBox->button(QDialogButtonBox::Ok),
		SIGNAL(clicked()), this, SLOT(Remux()));
	connect(ui->buttonBox->button(QDialogButtonBox::Close),
		SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->buttonBox->button(QDialogButtonBox::Reset),
		SIGNAL(clicked()), this, SLOT(clearFinished()));

	worker->moveToThread(&remuxer);
	remuxer.start();

	//gcc-4.8 can't use QPointer<RemuxWorker> below
	RemuxWorker *worker_ = worker;
	connect(worker_, &RemuxWorker::updateProgress,
			this, &OBSRemux::updateProgress);
	connect(worker_, &RemuxWorker::updateEntryState,
			this, &OBSRemux::updateEntryState);
	connect(&remuxer, &QThread::finished, worker_, &QObject::deleteLater);
	connect(worker_, &RemuxWorker::remuxFinished,
			this, &OBSRemux::remuxFinished);
	connect(this, &OBSRemux::remux, worker_, &RemuxWorker::remux);
}

bool OBSRemux::Stop()
{
	if (worker->jobQueue.empty())
		return true;

	// Flag the worker thread to suspend its processing while the modal
	// dialog is shown.
	os_event_signal(worker->wait);

	bool exit = false;

	if (QMessageBox::critical(nullptr,
			QTStr("Remux.ExitUnfinishedTitle"),
			QTStr("Remux.ExitUnfinished"),
			QMessageBox::Yes | QMessageBox::No,
			QMessageBox::No) ==
			QMessageBox::Yes) {
		exit = true;
	}

	if (exit)
	{
		// If the dialog result requests that the job be stopped,
		// we flag the worker thread to stop, then reset the wait
		// flag so it can continue until it does so.
		os_event_signal(worker->stop);
		os_event_reset(worker->wait);

		// And, finally, clear out all the pending jobs so future calls
		// to Stop() will just succeed silently.
		worker->jobQueue.clear();
	}
	else
	{
		// Otherwise, we just direct the worker to go on as planned.
		os_event_reset(worker->wait);
	}

	return exit;
}

OBSRemux::~OBSRemux()
{
	Stop();
	remuxer.quit();
	remuxer.wait();
}

void OBSRemux::inputCellChanged(QStandardItem *item)
{
	if (item->index().column() == RemuxEntryColumn::InputPath)
	{
		QString path = tableModel->data(item->index()).toString();

		if (path.isEmpty()) {
			// If we just blanked the input path and this was not
			// the empty row at the end of the list, remove it.
			if (item->index().row() < tableModel->rowCount() - 1)
				tableModel->removeRow(item->index().row());
		}
		else {
			QFileInfo fi(path);

			RemuxEntryState entryState;
			if (path.isEmpty())
				entryState = RemuxEntryState::Empty;
			else if (fi.exists())
				entryState = RemuxEntryState::Ready;
			else
				entryState = RemuxEntryState::InvalidPath;

			updateEntryState(item->index().row(), entryState);

			if (fi.exists()) {
				QModelIndex outputIndex = tableModel->index(
						item->index().row(),
						RemuxEntryColumn::OutputPath);
				QString outputPath = fi.path() + "/"
						+ fi.baseName() + ".mp4";

				tableModel->setData(outputIndex, outputPath);
			}

			// After this change, see if the last item in the list
			// is still empty. If it isn't, add a new empty row at
			// the end.
			QModelIndex lastItemIndex = tableModel->index(
					tableModel->rowCount() - 1,
					RemuxEntryColumn::InputPath);
			if (tableModel->data(lastItemIndex).toString().length()
					> 0)
				tableModel->appendRow(nullptr);
		}

		// Now, see if there are still any rows ready to remux. Change
		// the state of the "go" button accordingly.
		// There must be more than one row, since there will always be
		// at least one row for the empty insertion point.
		if (tableModel->rowCount() > 1)
			ui->buttonBox->button(QDialogButtonBox::Ok)->
				setEnabled(true);
		else
			ui->buttonBox->button(QDialogButtonBox::Ok)->
				setEnabled(false);
	}
}

void OBSRemux::dropEvent(QDropEvent *ev)
{
	for (QUrl url : ev->mimeData()->urls()) {

		int newRow = tableModel->rowCount() - 1;
		tableModel->insertRow(newRow);

		tableModel->setData(
			tableModel->index(newRow, RemuxEntryColumn::InputPath),
			url.toLocalFile());
	}
}

void OBSRemux::dragEnterEvent(QDragEnterEvent *ev)
{
	if (ev->mimeData()->hasUrls())
		ev->accept();
}

void OBSRemux::Remux()
{
	bool proceedWithRemux = false;
	worker->jobQueue.clear();

	for (int row = 0; row < tableModel->rowCount() - 1; row++) {
		if (tableModel->index(row, RemuxEntryColumn::State)
				.data(Qt::UserRole).value<RemuxEntryState>()
				== RemuxEntryState::Ready) {

			QString sourcePath = tableModel
					->index(row, RemuxEntryColumn::InputPath)
					.data().toString();
			QString targetPath = tableModel
					->index(row, RemuxEntryColumn::OutputPath)
					.data().toString();

			if (QFileInfo::exists(targetPath)) {
				if (OBSMessageBox::question(this,
						QTStr("Remux.FileExistsTitle"),
						QTStr("Remux.FileExistsWithPath")
						.arg(targetPath))
						!= QMessageBox::Yes) {
					proceedWithRemux = false;
					break;
				}
			}

			worker->jobQueue.append(RemuxWorker::JobInfo(row,
					sourcePath, targetPath));
			proceedWithRemux = true;
		}
	}

	if (!proceedWithRemux)
		return;

	worker->lastProgress = 0.f;

	ui->progressBar->setVisible(true);
	ui->buttonBox->button(QDialogButtonBox::Ok)->
			setEnabled(false);
	ui->tableView->setEnabled(false);
	setAcceptDrops(false);

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

void OBSRemux::updateEntryState(int key, RemuxEntryState state)
{
	QModelIndex stateIndex = tableModel->index(key, RemuxEntryColumn::State);
	tableModel->setData(stateIndex, state, Qt::UserRole);

	QVariant icon;
	QStyle *style = QApplication::style();

	switch (state)
	{
		case RemuxEntryState::Complete:
			icon = style->standardIcon(
					QStyle::SP_DialogApplyButton);
			break;

		case RemuxEntryState::InProgress:
			icon = style->standardIcon(
					QStyle::SP_ArrowRight);
			break;

		case RemuxEntryState::Error:
			icon = style->standardIcon(
					QStyle::SP_DialogCancelButton);
			break;

		case RemuxEntryState::InvalidPath:
			icon = style->standardIcon(
					QStyle::SP_MessageBoxWarning);
			break;
	}

	tableModel->setData(stateIndex, icon, Qt::DecorationRole);
}

void OBSRemux::remuxFinished(bool success)
{
	worker->jobQueue.clear();

	OBSMessageBox::information(this, QTStr("Remux.FinishedTitle"),
			success ?
			QTStr("Remux.Finished") : QTStr("Remux.FinishedError"));

	bool anyFinished = false;
	for (int row = 0; row < tableModel->rowCount() - 1; row++) {
		if (tableModel->index(row, RemuxEntryColumn::State)
				.data(Qt::UserRole).value<RemuxEntryState>()
				== RemuxEntryState::Complete) {
			anyFinished = true;
		}
	}

	ui->progressBar->setVisible(false);
	ui->buttonBox->button(QDialogButtonBox::Ok)->
			setEnabled(true);
	ui->buttonBox->button(QDialogButtonBox::Reset)->
			setEnabled(anyFinished);
	ui->tableView->setEnabled(true);
	setAcceptDrops(true);
}

void OBSRemux::clearFinished()
{
	for (int row = 0; row < tableModel->rowCount() - 1; row++) {
		if (tableModel->index(row, RemuxEntryColumn::State)
				.data(Qt::UserRole).value<RemuxEntryState>()
				== RemuxEntryState::Complete) {
			tableModel->removeRow(row);
			row--;
		}
	}

	ui->buttonBox->button(QDialogButtonBox::Reset)->
			setEnabled(false);
}

RemuxWorker::RemuxWorker()
{
	os_event_init(&stop, OS_EVENT_TYPE_MANUAL);
	os_event_init(&wait, OS_EVENT_TYPE_MANUAL);
}

RemuxWorker::~RemuxWorker()
{
	os_event_destroy(stop);
	os_event_destroy(wait);
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
	bool allSuccessful = true;

	auto callback = [](void *data, float percent)
	{
		auto rw = static_cast<RemuxWorker*>(data);
		rw->UpdateProgress(percent);

		// Suspend as long as the wait flag is set.
		while (os_event_try(rw->wait) != EAGAIN)
		{
			QThread::msleep(500);
		}

		return !!os_event_try(rw->stop);
	};

	// Set all jobs to "pending" first.
	for (JobInfo jobInfo : jobQueue)
		emit updateEntryState(jobInfo.jobKey,
				RemuxEntryState::Pending);

	bool stopped = false;

	for (JobInfo jobInfo : jobQueue) {

		emit updateEntryState(jobInfo.jobKey,
				RemuxEntryState::InProgress);

		bool success = false;

		media_remux_job_t mr_job = nullptr;
		if (media_remux_job_create(&mr_job,
					QT_TO_UTF8(jobInfo.sourcePath),
					QT_TO_UTF8(jobInfo.targetPath))) {

			success = media_remux_job_process(mr_job, callback,
					this);

			media_remux_job_destroy(mr_job);
		}

		if (success)
			emit updateEntryState(jobInfo.jobKey, RemuxEntryState::Complete);
		else
			emit updateEntryState(jobInfo.jobKey, RemuxEntryState::Error);

		allSuccessful |= success;

		if (os_event_try(stop) != EAGAIN)
		{
			stopped = true;
			break;
		}
	}

	emit remuxFinished(!stopped && allSuccessful);
}
