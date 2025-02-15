/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

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

#include "item-widget-helpers.hpp"

#include <QListWidget>

QListWidgetItem *TakeListItem(QListWidget *widget, int row)
{
	QListWidgetItem *item = widget->item(row);

	if (item)
		delete widget->itemWidget(item);

	return widget->takeItem(row);
}

void DeleteListItem(QListWidget *widget, QListWidgetItem *item)
{
	if (item) {
		delete widget->itemWidget(item);
		delete item;
	}
}

void ClearListItems(QListWidget *widget)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 4, 3)
	// Workaround for the SceneTree workaround for QTBUG-105870
	widget->setProperty("clearing", true);
#endif

	widget->setCurrentItem(nullptr, QItemSelectionModel::Clear);

	for (int i = 0; i < widget->count(); i++)
		delete widget->itemWidget(widget->item(i));

	widget->clear();
#if QT_VERSION < QT_VERSION_CHECK(6, 4, 3)
	// Workaround for the SceneTree workaround for QTBUG-105870
	widget->setProperty("clearing", false);
#endif
}
