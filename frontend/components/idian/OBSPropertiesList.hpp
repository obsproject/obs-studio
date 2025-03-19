/******************************************************************************
    Copyright (C) 2023 by Dennis Sädtler <dennis@obsproject.com>

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

#include <QFrame>
#include <QWidget>
#include <QLayout>

#include "OBSIdianWidget.hpp"

class OBSActionRow;

class OBSPropertiesList : public QFrame {
	Q_OBJECT

public:
	OBSPropertiesList(QWidget *parent = nullptr);

	void addRow(OBSActionRow *actionRow);
	void clear();

	QList<OBSActionRow *> rows() const { return rowsList; }

private:
	OBSActionRow *first = nullptr;
	OBSActionRow *last = nullptr;

	QVBoxLayout *layout;
	QList<OBSActionRow *> rowsList;
};

/**
* Spacer with only cosmetic functionality
*/
class OBSPropertiesListSpacer : public QFrame {
	Q_OBJECT
public:
	OBSPropertiesListSpacer(QWidget *parent = nullptr) : QFrame(parent)
	{
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	}
};
