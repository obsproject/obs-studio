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

#include <QStyledItemDelegate>

class ImporterEntryPathItemDelegate : public QStyledItemDelegate {
	Q_OBJECT

public:
	ImporterEntryPathItemDelegate();

	virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem & /* option */,
				      const QModelIndex &index) const override;

	virtual void setEditorData(QWidget *editor, const QModelIndex &index) const override;
	virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
	virtual void paint(QPainter *painter, const QStyleOptionViewItem &option,
			   const QModelIndex &index) const override;

private:
	const char *PATH_LIST_PROP = "pathList";

	void handleBrowse(QWidget *container);
	void handleClear(QWidget *container);

private slots:
	void updateText();
};
