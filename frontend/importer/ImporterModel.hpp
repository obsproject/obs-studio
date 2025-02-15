/******************************************************************************
    Copyright (C) 2019-2020 by Dillon Pentz <dillon@vodbox.io>

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

enum ImporterColumn {
	Selected,
	Name,
	Path,
	Program,

	Count
};

enum ImporterEntryRole { EntryStateRole = Qt::UserRole, NewPath, AutoPath, CheckEmpty };

class ImporterModel : public QAbstractTableModel {
	Q_OBJECT

	friend class OBSImporter;

public:
	ImporterModel(QObject *parent = 0) : QAbstractTableModel(parent) {}

	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role);

private:
	struct ImporterEntry {
		QString path;
		QString program;
		QString name;

		bool selected;
		bool empty;
	};

	QList<ImporterEntry> options;

	void checkInputPath(int row);
};
