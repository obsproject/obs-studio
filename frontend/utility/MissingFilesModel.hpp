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

#pragma once

#include <QAbstractTableModel>
#include <QIcon>

// TODO: Fix redefinition error of due to clash with enums defined in importer code.
enum MissingFilesRole { EntryStateRole = Qt::UserRole, NewPathsToProcessRole };

// TODO: Fix redefinition error of due to clash with enums defined in importer code.
enum MissingFilesColumn { Source, OriginalPath, NewPath, State, Count };

enum MissingFilesState { Missing, Found, Replaced, Cleared };

Q_DECLARE_METATYPE(MissingFilesState);

class MissingFilesModel : public QAbstractTableModel {
	Q_OBJECT

	friend class OBSMissingFiles;

public:
	explicit MissingFilesModel(QObject *parent = 0);

	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	int found() const;
	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role);

	bool allFilesFound = false;
	int recursionLevel = 0;

	QIcon warningIcon;

private:
	struct MissingFileEntry {
		MissingFilesState state = MissingFilesState::Missing;

		QString source;

		QString originalPath;
		QString newPath;
	};

	QList<MissingFileEntry> files;

	void findAllFilesInPath(const QString &path, bool skipPrompt);
	void fileCheckLoop(const QString &path, bool skipPrompt, int depth);
};
