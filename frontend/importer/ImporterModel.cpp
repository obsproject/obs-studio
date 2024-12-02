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

#include "ImporterModel.hpp"

#include <OBSApp.hpp>
#include <importers/importers.hpp>

#include "moc_ImporterModel.cpp"

int ImporterModel::rowCount(const QModelIndex &) const
{
	return options.length() + 1;
}

int ImporterModel::columnCount(const QModelIndex &) const
{
	return ImporterColumn::Count;
}

QVariant ImporterModel::data(const QModelIndex &index, int role) const
{
	QVariant result = QVariant();

	if (index.row() >= options.length()) {
		if (role == ImporterEntryRole::CheckEmpty)
			result = true;
		else
			return QVariant();
	} else if (role == Qt::DisplayRole) {
		switch (index.column()) {
		case ImporterColumn::Path:
			result = options[index.row()].path;
			break;
		case ImporterColumn::Program:
			result = options[index.row()].program;
			break;
		case ImporterColumn::Name:
			result = options[index.row()].name;
		}
	} else if (role == Qt::EditRole) {
		if (index.column() == ImporterColumn::Name) {
			result = options[index.row()].name;
		}
	} else if (role == Qt::CheckStateRole) {
		switch (index.column()) {
		case ImporterColumn::Selected:
			if (options[index.row()].program != "")
				result = options[index.row()].selected ? Qt::Checked : Qt::Unchecked;
			else
				result = Qt::Unchecked;
		}
	} else if (role == ImporterEntryRole::CheckEmpty) {
		result = options[index.row()].empty;
	}

	return result;
}

Qt::ItemFlags ImporterModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags flags = QAbstractTableModel::flags(index);

	if (index.column() == ImporterColumn::Selected && index.row() != options.length()) {
		flags |= Qt::ItemIsUserCheckable;
	} else if (index.column() == ImporterColumn::Path ||
		   (index.column() == ImporterColumn::Name && index.row() != options.length())) {
		flags |= Qt::ItemIsEditable;
	}

	return flags;
}

void ImporterModel::checkInputPath(int row)
{
	ImporterEntry &entry = options[row];

	if (entry.path.isEmpty()) {
		entry.program = "";
		entry.empty = true;
		entry.selected = false;
		entry.name = "";
	} else {
		entry.empty = false;

		std::string program = DetectProgram(entry.path.toStdString());
		entry.program = QTStr(program.c_str());

		if (program.empty()) {
			entry.selected = false;
		} else {
			std::string name = GetSCName(entry.path.toStdString(), program);
			entry.name = name.c_str();
		}
	}

	emit dataChanged(index(row, 0), index(row, ImporterColumn::Count));
}

bool ImporterModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role == ImporterEntryRole::NewPath) {
		QStringList list = value.toStringList();

		if (list.size() == 0) {
			if (index.row() < options.size()) {
				beginRemoveRows(QModelIndex(), index.row(), index.row());
				options.removeAt(index.row());
				endRemoveRows();
			}
		} else {
			if (list.size() > 0 && index.row() < options.length()) {
				options[index.row()].path = list[0];
				checkInputPath(index.row());

				list.removeAt(0);
			}

			if (list.size() > 0) {
				int row = index.row();
				int lastRow = row + list.size() - 1;
				beginInsertRows(QModelIndex(), row, lastRow);

				for (QString path : list) {
					ImporterEntry entry;
					entry.path = path;

					options.insert(row, entry);

					row++;
				}

				endInsertRows();

				for (row = index.row(); row <= lastRow; row++) {
					checkInputPath(row);
				}
			}
		}
	} else if (index.row() == options.length()) {
		QString path = value.toString();

		if (!path.isEmpty()) {
			ImporterEntry entry;
			entry.path = path;
			entry.selected = role != ImporterEntryRole::AutoPath;
			entry.empty = false;

			beginInsertRows(QModelIndex(), options.length() + 1, options.length() + 1);
			options.append(entry);
			endInsertRows();

			checkInputPath(index.row());
		}
	} else if (index.column() == ImporterColumn::Selected) {
		bool select = value.toBool();

		options[index.row()].selected = select;
	} else if (index.column() == ImporterColumn::Path) {
		QString path = value.toString();
		options[index.row()].path = path;

		checkInputPath(index.row());
	} else if (index.column() == ImporterColumn::Name) {
		QString name = value.toString();
		options[index.row()].name = name;
	}

	emit dataChanged(index, index);

	return true;
}

QVariant ImporterModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	QVariant result = QVariant();

	if (role == Qt::DisplayRole && orientation == Qt::Orientation::Horizontal) {
		switch (section) {
		case ImporterColumn::Path:
			result = QTStr("Importer.Path");
			break;
		case ImporterColumn::Program:
			result = QTStr("Importer.Program");
			break;
		case ImporterColumn::Name:
			result = QTStr("Name");
		}
	}

	return result;
}
