/******************************************************************************
    Copyright (C) 2019 by Dillon Pentz <dillon@vodbox.io>

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

#include "OBSMissingFiles.hpp"

#include <OBSApp.hpp>
#include <utility/MissingFilesModel.hpp>
#include <utility/MissingFilesPathItemDelegate.hpp>

#include <QFileDialog>

#include "moc_OBSMissingFiles.cpp"

// TODO: Fix redefinition error of due to clash with enums defined in importer code.
enum MissingFilesRole { EntryStateRole = Qt::UserRole, NewPathsToProcessRole };

// TODO: Fix redefinition error of due to clash with enums defined in importer code.
enum MissingFilesColumn { Source, OriginalPath, NewPath, State, Count };

OBSMissingFiles::OBSMissingFiles(obs_missing_files_t *files, QWidget *parent)
	: QDialog(parent),
	  filesModel(new MissingFilesModel),
	  ui(new Ui::OBSMissingFiles)
{
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	ui->setupUi(this);

	ui->tableView->setModel(filesModel);
	ui->tableView->setItemDelegateForColumn(MissingFilesColumn::OriginalPath,
						new MissingFilesPathItemDelegate(false, ""));
	ui->tableView->setItemDelegateForColumn(MissingFilesColumn::NewPath,
						new MissingFilesPathItemDelegate(true, ""));
	ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Stretch);
	ui->tableView->horizontalHeader()->setSectionResizeMode(MissingFilesColumn::Source,
								QHeaderView::ResizeMode::ResizeToContents);
	ui->tableView->horizontalHeader()->setMaximumSectionSize(width() / 3);
	ui->tableView->horizontalHeader()->setSectionResizeMode(MissingFilesColumn::State,
								QHeaderView::ResizeMode::ResizeToContents);
	ui->tableView->setEditTriggers(QAbstractItemView::EditTrigger::CurrentChanged);

	ui->warningIcon->setPixmap(filesModel->warningIcon.pixmap(QSize(32, 32)));

	for (size_t i = 0; i < obs_missing_files_count(files); i++) {
		obs_missing_file_t *f = obs_missing_files_get_file(files, (int)i);

		const char *oldPath = obs_missing_file_get_path(f);
		const char *name = obs_missing_file_get_source_name(f);

		addMissingFile(oldPath, name);
	}

	QString found = QTStr("MissingFiles.NumFound").arg("0", QString::number(obs_missing_files_count(files)));

	ui->found->setText(found);

	fileStore = files;

	connect(ui->doneButton, &QPushButton::clicked, this, &OBSMissingFiles::saveFiles);
	connect(ui->browseButton, &QPushButton::clicked, this, &OBSMissingFiles::browseFolders);
	connect(ui->cancelButton, &QPushButton::clicked, this, &OBSMissingFiles::close);
	connect(filesModel, &MissingFilesModel::dataChanged, this, &OBSMissingFiles::dataChanged);

	QModelIndex index = filesModel->createIndex(0, 1);
	QMetaObject::invokeMethod(ui->tableView, "setCurrentIndex", Qt::QueuedConnection,
				  Q_ARG(const QModelIndex &, index));
}

OBSMissingFiles::~OBSMissingFiles()
{
	obs_missing_files_destroy(fileStore);
}

void OBSMissingFiles::addMissingFile(const char *originalPath, const char *sourceName)
{
	QStringList list;

	list.append(originalPath);
	list.append(sourceName);

	QModelIndex insertIndex = filesModel->index(filesModel->rowCount() - 1, MissingFilesColumn::Source);

	filesModel->setData(insertIndex, list, MissingFilesRole::NewPathsToProcessRole);
}

void OBSMissingFiles::saveFiles()
{
	for (int i = 0; i < filesModel->files.length(); i++) {
		MissingFilesState state = filesModel->files[i].state;
		if (state != MissingFilesState::Missing) {
			obs_missing_file_t *f = obs_missing_files_get_file(fileStore, i);

			QString path = filesModel->files[i].newPath;

			if (state == MissingFilesState::Cleared) {
				obs_missing_file_issue_callback(f, "");
			} else {
				char *p = bstrdup(path.toStdString().c_str());
				obs_missing_file_issue_callback(f, p);
				bfree(p);
			}
		}
	}

	QDialog::accept();
}

void OBSMissingFiles::browseFolders()
{
	QString dir = QFileDialog::getExistingDirectory(this, QTStr("MissingFiles.SelectDir"), "",
							QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if (dir != "") {
		dir += "/";
		filesModel->fileCheckLoop(filesModel->files, dir, true);
	}
}

void OBSMissingFiles::dataChanged()
{
	QString found =
		QTStr("MissingFiles.NumFound")
			.arg(QString::number(filesModel->found()), QString::number(obs_missing_files_count(fileStore)));

	ui->found->setText(found);

	ui->tableView->resizeColumnToContents(MissingFilesColumn::State);
	ui->tableView->resizeColumnToContents(MissingFilesColumn::Source);
}

QIcon OBSMissingFiles::GetWarningIcon()
{
	return filesModel->warningIcon;
}

void OBSMissingFiles::SetWarningIcon(const QIcon &icon)
{
	ui->warningIcon->setPixmap(icon.pixmap(QSize(32, 32)));
	filesModel->warningIcon = icon;
}
