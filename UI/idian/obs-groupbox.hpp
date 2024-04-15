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

#include <QLayout>
#include <QLabel>
#include <QWidget>
#include <QMouseEvent>

#include "obs-actionrow.hpp"
#include "obs-propertieslist.hpp"
#include "obs-toggleswitch.hpp"

class OBSGroupBox : public QFrame {
	Q_OBJECT

public:
	OBSGroupBox(QWidget *parent = nullptr);
	OBSGroupBox(const QString &name, QWidget *parent = nullptr);
	OBSGroupBox(const QString &name, bool checkable,
		    QWidget *parent = nullptr);
	OBSGroupBox(const QString &name, const QString &desc,
		    QWidget *parent = nullptr);
	OBSGroupBox(const QString &name, const QString &desc,
		    bool checkable = false, QWidget *parent = nullptr);

	OBSPropertiesList *properties() const { return plist; }

	// ToDo add event for checkable group being enabled/disabled
	// ToDo allow setting enabled state
	// (Maybe) ToDo add option for hiding properties list when disabled

	void addRow(OBSActionBaseClass *ar) const;

private:
	QGridLayout *layout = nullptr;

	QLabel *nameLbl = nullptr;
	QLabel *descLbl = nullptr;

	OBSPropertiesList *plist = nullptr;

	OBSToggleSwitch *toggleSwitch = nullptr;
};
