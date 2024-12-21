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

#include "MissingFilesModel.hpp"

#include <widgets/OBSBasic.hpp>

#include <QFileInfo>
#include <QMessageBox>

#include "moc_MissingFilesModel.cpp"

// TODO: Fix redefinition error of due to clash with enums defined in importer code.
enum MissingFilesRole { EntryStateRole = Qt::UserRole, NewPathsToProcessRole };

// TODO: Fix redefinition error of due to clash with enums defined in importer code.
enum MissingFilesColumn { Source, OriginalPath, NewPath, State, Count };

MissingFilesModel::MissingFilesModel(QObject *parent) : QAbstractTableModel(parent)
{
	QStyle *style = QApplication::style();

	warningIcon = style->standardIcon(QStyle::SP_MessageBoxWarning);
}

int MissingFilesModel::rowCount(const QModelIndex &) const
{
	return files.length();
}

int MissingFilesModel::columnCount(const QModelIndex &) const
{
	return MissingFilesColumn::Count;
}

int MissingFilesModel::found() const
{
	int res = 0;

	for (int i = 0; i < files.length(); i++) {
		if (files[i].state != Missing && files[i].state != Cleared)
			res++;
	}

	return res;
}

QVariant MissingFilesModel::data(const QModelIndex &index, int role) const
{
	QVariant result = QVariant();

	if (index.row() >= files.length()) {
		return QVariant();
	} else if (role == Qt::DisplayRole) {
		QFileInfo fi(files[index.row()].originalPath);

		switch (index.column()) {
		case MissingFilesColumn::Source:
			result = files[index.row()].source;
			break;
		case MissingFilesColumn::OriginalPath:
			result = fi.fileName();
			break;
		case MissingFilesColumn::NewPath:
			result = files[index.row()].newPath;
			break;
		case MissingFilesColumn::State:
			switch (files[index.row()].state) {
			case MissingFilesState::Missing:
				result = QTStr("MissingFiles.Missing");
				break;

			case MissingFilesState::Replaced:
				result = QTStr("MissingFiles.Replaced");
				break;

			case MissingFilesState::Found:
				result = QTStr("MissingFiles.Found");
				break;

			case MissingFilesState::Cleared:
				result = QTStr("MissingFiles.Cleared");
				break;
			}
			break;
		}
	} else if (role == Qt::DecorationRole && index.column() == MissingFilesColumn::Source) {
		OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
		OBSSourceAutoRelease source = obs_get_source_by_name(files[index.row()].source.toStdString().c_str());

		if (source) {
			result = main->GetSourceIcon(obs_source_get_id(source));
		}
	} else if (role == Qt::FontRole && index.column() == MissingFilesColumn::State) {
		QFont font = QFont();
		font.setBold(true);

		result = font;
	} else if (role == Qt::ToolTipRole && index.column() == MissingFilesColumn::State) {
		switch (files[index.row()].state) {
		case MissingFilesState::Missing:
			result = QTStr("MissingFiles.Missing");
			break;

		case MissingFilesState::Replaced:
			result = QTStr("MissingFiles.Replaced");
			break;

		case MissingFilesState::Found:
			result = QTStr("MissingFiles.Found");
			break;

		case MissingFilesState::Cleared:
			result = QTStr("MissingFiles.Cleared");
			break;

		default:
			break;
		}
	} else if (role == Qt::ToolTipRole) {
		switch (index.column()) {
		case MissingFilesColumn::OriginalPath:
			result = files[index.row()].originalPath;
			break;
		case MissingFilesColumn::NewPath:
			result = files[index.row()].newPath;
			break;
		default:
			break;
		}
	}

	return result;
}

Qt::ItemFlags MissingFilesModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags flags = QAbstractTableModel::flags(index);

	if (index.column() == MissingFilesColumn::OriginalPath) {
		flags &= ~Qt::ItemIsEditable;
	} else if (index.column() == MissingFilesColumn::NewPath && index.row() != files.length()) {
		flags |= Qt::ItemIsEditable;
	}

	return flags;
}

void MissingFilesModel::fileCheckLoop(QList<MissingFileEntry> files, QString path, bool skipPrompt)
{
	loop = false;
	QUrl url = QUrl().fromLocalFile(path);
	QString dir = url.toDisplayString(QUrl::RemoveScheme | QUrl::RemoveFilename | QUrl::PreferLocalFile);

	bool prompted = skipPrompt;

	for (int i = 0; i < files.length(); i++) {
		if (files[i].state != MissingFilesState::Missing)
			continue;

		QUrl origFile = QUrl().fromLocalFile(files[i].originalPath);
		QString filename = origFile.fileName();
		QString testFile = dir + filename;

		if (os_file_exists(testFile.toStdString().c_str())) {
			if (!prompted) {
				QMessageBox::StandardButton button =
					QMessageBox::question(nullptr, QTStr("MissingFiles.AutoSearch"),
							      QTStr("MissingFiles.AutoSearchText"));

				if (button == QMessageBox::No)
					break;

				prompted = true;
			}
			QModelIndex in = index(i, MissingFilesColumn::NewPath);
			setData(in, testFile, 0);
		}
	}
	loop = true;
}

bool MissingFilesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	bool success = false;

	if (role == MissingFilesRole::NewPathsToProcessRole) {
		QStringList list = value.toStringList();

		int row = index.row() + 1;
		beginInsertRows(QModelIndex(), row, row);

		MissingFileEntry entry;
		entry.originalPath = list[0].replace("\\", "/");
		entry.source = list[1];

		files.insert(row, entry);
		row++;

		endInsertRows();

		success = true;
	} else {
		QString path = value.toString();
		if (index.column() == MissingFilesColumn::NewPath) {
			files[index.row()].newPath = value.toString();
			QString fileName = QUrl(path).fileName();
			QString origFileName = QUrl(files[index.row()].originalPath).fileName();

			if (path.isEmpty()) {
				files[index.row()].state = MissingFilesState::Missing;
			} else if (path.compare(QTStr("MissingFiles.Clear")) == 0) {
				files[index.row()].state = MissingFilesState::Cleared;
			} else if (fileName.compare(origFileName) == 0) {
				files[index.row()].state = MissingFilesState::Found;

				if (loop)
					fileCheckLoop(files, path, false);
			} else {
				files[index.row()].state = MissingFilesState::Replaced;

				if (loop)
					fileCheckLoop(files, path, false);
			}

			emit dataChanged(index, index);
			success = true;
		}
	}

	return success;
}

QVariant MissingFilesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	QVariant result = QVariant();

	if (role == Qt::DisplayRole && orientation == Qt::Orientation::Horizontal) {
		switch (section) {
		case MissingFilesColumn::State:
			result = QTStr("MissingFiles.State");
			break;
		case MissingFilesColumn::Source:
			result = QTStr("Basic.Main.Source");
			break;
		case MissingFilesColumn::OriginalPath:
			result = QTStr("MissingFiles.MissingFile");
			break;
		case MissingFilesColumn::NewPath:
			result = QTStr("MissingFiles.NewFile");
			break;
		}
	}

	return result;
}
