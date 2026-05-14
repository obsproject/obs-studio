/******************************************************************************
    Copyright (C) 2026 by Taylor Giampaolo <warchamp7@obsproject.com>

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

#include <Idian/Utils.hpp>

#include <QFrame>

class QPixmap;
class QVBoxLayout;

namespace idian {
class Row;
class RowInfo;
class ExpandButton;
class RowList;
class ToggleSwitch;

class CollapsibleGroup : public QFrame, public Utils {
	Q_OBJECT

public:
	CollapsibleGroup(QWidget *parent = nullptr);

	void setCheckable(bool check);
	bool isCheckable() { return checkable; }

	void setChecked(bool checked);
	bool isChecked();

	void setExpanded(bool expand = true);

	Row *row() { return rowWidget; }
	RowList *list() { return propertyList; }

protected:
	void toggleVisibility();

	QVBoxLayout *mainLayout;

	Row *rowWidget;
	ExpandButton *expandButton;

	RowList *propertyList;

	ToggleSwitch *toggleSwitch = nullptr;
	bool checkable = false;

signals:
	void toggled(bool checked);
};
} // namespace idian
