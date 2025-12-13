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

#pragma once

#include <QAbstractTableModel>
#include <QFileInfoList>

enum RemuxEntryState { Empty, Ready, Pending, InProgress, Complete, InvalidPath, Error };

Q_DECLARE_METATYPE(RemuxEntryState);

enum RemuxEntryColumn {
	State,
	InputPath,
	OutputPath,

	Count
};

enum RemuxEntryRole { EntryStateRole = Qt::UserRole, NewPathsToProcessRole };

class RemuxQueueModel : public QAbstractTableModel {
	Q_OBJECT

	friend class OBSRemux;

public:
	RemuxQueueModel(QObject *parent = 0) : QAbstractTableModel(parent), isProcessing(false) {}

	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role);

	QFileInfoList checkForOverwrites() const;
	bool checkForErrors() const;
	void beginProcessing();
	void endProcessing();
	bool beginNextEntry(QString &inputPath, QString &outputPath);
	void finishEntry(bool success);
	bool canClearFinished() const;
	void clearFinished();
	void clearAll();

	bool autoRemux = false;

private:
	struct RemuxQueueEntry {
		RemuxEntryState state;

		QString sourcePath;
		QString targetPath;
	};

	QList<RemuxQueueEntry> queue;
	bool isProcessing;

	static QVariant getIcon(RemuxEntryState state);

	void checkInputPath(int row);
};
