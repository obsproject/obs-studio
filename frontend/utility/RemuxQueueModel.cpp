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

#include "RemuxQueueModel.hpp"

#include <OBSApp.hpp>

#include <QDir>
#include <QStyle>

#include "moc_RemuxQueueModel.cpp"

int RemuxQueueModel::rowCount(const QModelIndex &) const
{
	return queue.length() + (isProcessing ? 0 : 1);
}

int RemuxQueueModel::columnCount(const QModelIndex &) const
{
	return RemuxEntryColumn::Count;
}

QVariant RemuxQueueModel::data(const QModelIndex &index, int role) const
{
	QVariant result = QVariant();

	if (index.row() >= queue.length()) {
		return QVariant();
	} else if (role == Qt::DisplayRole) {
		switch (index.column()) {
		case RemuxEntryColumn::InputPath:
			result = queue[index.row()].sourcePath;
			break;
		case RemuxEntryColumn::OutputPath:
			result = queue[index.row()].targetPath;
			break;
		}
	} else if (role == Qt::DecorationRole && index.column() == RemuxEntryColumn::State) {
		result = getIcon(queue[index.row()].state);
	} else if (role == RemuxEntryRole::EntryStateRole) {
		result = queue[index.row()].state;
	}

	return result;
}

QVariant RemuxQueueModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	QVariant result = QVariant();

	if (role == Qt::DisplayRole && orientation == Qt::Orientation::Horizontal) {
		switch (section) {
		case RemuxEntryColumn::State:
			result = QString();
			break;
		case RemuxEntryColumn::InputPath:
			result = QTStr("Remux.SourceFile");
			break;
		case RemuxEntryColumn::OutputPath:
			result = QTStr("Remux.TargetFile");
			break;
		}
	}

	return result;
}

Qt::ItemFlags RemuxQueueModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags flags = QAbstractTableModel::flags(index);

	if (index.column() == RemuxEntryColumn::InputPath) {
		flags |= Qt::ItemIsEditable;
	} else if (index.column() == RemuxEntryColumn::OutputPath && index.row() != queue.length()) {
		flags |= Qt::ItemIsEditable;
	}

	return flags;
}

bool RemuxQueueModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	bool success = false;

	if (role == RemuxEntryRole::NewPathsToProcessRole) {
		QStringList pathList = value.toStringList();

		if (pathList.size() == 0) {
			if (index.row() < queue.size()) {
				beginRemoveRows(QModelIndex(), index.row(), index.row());
				queue.removeAt(index.row());
				endRemoveRows();
			}
		} else {
			if (pathList.size() >= 1 && index.row() < queue.length()) {
				queue[index.row()].sourcePath = pathList[0];
				checkInputPath(index.row());

				pathList.removeAt(0);

				success = true;
			}

			if (pathList.size() > 0) {
				int row = index.row();
				int lastRow = row + pathList.size() - 1;
				beginInsertRows(QModelIndex(), row, lastRow);

				for (QString path : pathList) {
					RemuxQueueEntry entry;
					entry.sourcePath = path;
					entry.state = RemuxEntryState::Empty;

					queue.insert(row, entry);
					row++;
				}
				endInsertRows();

				for (row = index.row(); row <= lastRow; row++) {
					checkInputPath(row);
				}

				success = true;
			}
		}
	} else if (index.row() == queue.length()) {
		QString path = value.toString();

		if (!path.isEmpty()) {
			RemuxQueueEntry entry;
			entry.sourcePath = path;
			entry.state = RemuxEntryState::Empty;

			beginInsertRows(QModelIndex(), queue.length() + 1, queue.length() + 1);
			queue.append(entry);
			endInsertRows();

			checkInputPath(index.row());
			success = true;
		}
	} else {
		QString path = value.toString();

		if (path.isEmpty()) {
			if (index.column() == RemuxEntryColumn::InputPath) {
				beginRemoveRows(QModelIndex(), index.row(), index.row());
				queue.removeAt(index.row());
				endRemoveRows();
			}
		} else {
			switch (index.column()) {
			case RemuxEntryColumn::InputPath:
				queue[index.row()].sourcePath = value.toString();
				checkInputPath(index.row());
				success = true;
				break;
			case RemuxEntryColumn::OutputPath:
				queue[index.row()].targetPath = value.toString();
				emit dataChanged(index, index);
				success = true;
				break;
			}
		}
	}

	return success;
}

QVariant RemuxQueueModel::getIcon(RemuxEntryState state)
{
	QVariant icon;
	QStyle *style = QApplication::style();

	switch (state) {
	case RemuxEntryState::Complete:
		icon = style->standardIcon(QStyle::SP_DialogApplyButton);
		break;

	case RemuxEntryState::InProgress:
		icon = style->standardIcon(QStyle::SP_ArrowRight);
		break;

	case RemuxEntryState::Error:
		icon = style->standardIcon(QStyle::SP_DialogCancelButton);
		break;

	case RemuxEntryState::InvalidPath:
		icon = style->standardIcon(QStyle::SP_MessageBoxWarning);
		break;

	default:
		break;
	}

	return icon;
}

void RemuxQueueModel::checkInputPath(int row)
{
	RemuxQueueEntry &entry = queue[row];

	if (entry.sourcePath.isEmpty()) {
		entry.state = RemuxEntryState::Empty;
	} else {
		entry.sourcePath = QDir::toNativeSeparators(entry.sourcePath);
		QFileInfo fileInfo(entry.sourcePath);
		if (fileInfo.exists())
			entry.state = RemuxEntryState::Ready;
		else
			entry.state = RemuxEntryState::InvalidPath;

		QString newExt = ".mp4";
		QString suffix = fileInfo.suffix();

		if (suffix.contains("mov", Qt::CaseInsensitive) || suffix.contains("mp4", Qt::CaseInsensitive)) {
			newExt = ".remuxed." + suffix;
		}

		if (entry.state == RemuxEntryState::Ready)
			entry.targetPath = QDir::toNativeSeparators(fileInfo.path() + QDir::separator() +
								    fileInfo.completeBaseName() + newExt);
	}

	if (entry.state == RemuxEntryState::Ready && isProcessing)
		entry.state = RemuxEntryState::Pending;

	emit dataChanged(index(row, 0), index(row, RemuxEntryColumn::Count));
}

QFileInfoList RemuxQueueModel::checkForOverwrites() const
{
	QFileInfoList list;

	for (const RemuxQueueEntry &entry : queue) {
		if (entry.state == RemuxEntryState::Ready) {
			QFileInfo fileInfo(entry.targetPath);
			if (fileInfo.exists()) {
				list.append(fileInfo);
			}
		}
	}

	return list;
}

bool RemuxQueueModel::checkForErrors() const
{
	bool hasErrors = false;

	for (const RemuxQueueEntry &entry : queue) {
		if (entry.state == RemuxEntryState::Error) {
			hasErrors = true;
			break;
		}
	}

	return hasErrors;
}

void RemuxQueueModel::clearAll()
{
	beginRemoveRows(QModelIndex(), 0, queue.size() - 1);
	queue.clear();
	endRemoveRows();
}

void RemuxQueueModel::clearFinished()
{
	int index = 0;

	for (index = 0; index < queue.size(); index++) {
		const RemuxQueueEntry &entry = queue[index];
		if (entry.state == RemuxEntryState::Complete) {
			beginRemoveRows(QModelIndex(), index, index);
			queue.removeAt(index);
			endRemoveRows();
			index--;
		}
	}
}

bool RemuxQueueModel::canClearFinished() const
{
	bool canClearFinished = false;
	for (const RemuxQueueEntry &entry : queue)
		if (entry.state == RemuxEntryState::Complete) {
			canClearFinished = true;
			break;
		}

	return canClearFinished;
}

void RemuxQueueModel::beginProcessing()
{
	for (RemuxQueueEntry &entry : queue)
		if (entry.state == RemuxEntryState::Ready)
			entry.state = RemuxEntryState::Pending;

	// Signal that the insertion point no longer exists.
	beginRemoveRows(QModelIndex(), queue.length(), queue.length());
	endRemoveRows();

	isProcessing = true;

	emit dataChanged(index(0, RemuxEntryColumn::State), index(queue.length(), RemuxEntryColumn::State));
}

void RemuxQueueModel::endProcessing()
{
	for (RemuxQueueEntry &entry : queue) {
		if (entry.state == RemuxEntryState::Pending) {
			entry.state = RemuxEntryState::Ready;
		}
	}

	// Signal that the insertion point exists again.
	isProcessing = false;
	if (!autoRemux) {
		beginInsertRows(QModelIndex(), queue.length(), queue.length());
		endInsertRows();
	}

	emit dataChanged(index(0, RemuxEntryColumn::State), index(queue.length(), RemuxEntryColumn::State));
}

bool RemuxQueueModel::beginNextEntry(QString &inputPath, QString &outputPath)
{
	bool anyStarted = false;

	for (int row = 0; row < queue.length(); row++) {
		RemuxQueueEntry &entry = queue[row];
		if (entry.state == RemuxEntryState::Pending) {
			entry.state = RemuxEntryState::InProgress;

			inputPath = entry.sourcePath;
			outputPath = entry.targetPath;

			QModelIndex index = this->index(row, RemuxEntryColumn::State);
			emit dataChanged(index, index);

			anyStarted = true;
			break;
		}
	}

	return anyStarted;
}

void RemuxQueueModel::finishEntry(bool success)
{
	for (int row = 0; row < queue.length(); row++) {
		RemuxQueueEntry &entry = queue[row];
		if (entry.state == RemuxEntryState::InProgress) {
			if (success)
				entry.state = RemuxEntryState::Complete;
			else
				entry.state = RemuxEntryState::Error;

			QModelIndex index = this->index(row, RemuxEntryColumn::State);
			emit dataChanged(index, index);

			break;
		}
	}
}
