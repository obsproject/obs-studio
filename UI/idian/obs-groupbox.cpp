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

#include "obs-groupbox.hpp"

OBSGroupBox::OBSGroupBox(QWidget *parent) : QFrame(parent)
{
	layout = new QGridLayout(this);
	layout->setVerticalSpacing(6);

	plist = new OBSPropertiesList(this);

	QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	setSizePolicy(policy);
	setLayout(layout);
	setMinimumWidth(300);
	setMaximumWidth(600);

	layout->addWidget(plist, 2, 0, -1, -1);
}

OBSGroupBox::OBSGroupBox(const QString &name, QWidget *parent)
	: OBSGroupBox(parent)
{

	nameLbl = new QLabel();
	nameLbl->setText(name);
	nameLbl->setProperty("class", "title");

	layout->addWidget(nameLbl, 0, 0, Qt::AlignLeft);
}

OBSGroupBox::OBSGroupBox(const QString &name, bool checkable, QWidget *parent)
	: OBSGroupBox(name, parent)
{
	if (checkable) {
		toggleSwitch = new OBSToggleSwitch(true);
		layout->addWidget(toggleSwitch, 0, 1, Qt::AlignRight);
		connect(toggleSwitch, &OBSToggleSwitch::toggled, this,
			[=](bool checked) { plist->setEnabled(checked); });
	}
}

OBSGroupBox::OBSGroupBox(const QString &name, const QString &desc,
			 QWidget *parent)
	: OBSGroupBox(name, parent)
{
	descLbl = new QLabel();
	descLbl->setText(desc);
	descLbl->setProperty("class", "subtitle");
	layout->addWidget(descLbl, 1, 0, Qt::AlignLeft);
}

OBSGroupBox::OBSGroupBox(const QString &name, const QString &desc,
			 bool checkable, QWidget *parent)
	: OBSGroupBox(name, desc, parent)
{
	if (checkable) {
		toggleSwitch = new OBSToggleSwitch(true);
		layout->addWidget(toggleSwitch, 0, 1, 2, 1, Qt::AlignRight);
		connect(toggleSwitch, &OBSToggleSwitch::toggled, this,
			[=](bool checked) { plist->setEnabled(checked); });
	}
}

void OBSGroupBox::addRow(OBSActionBaseClass *ar) const
{
	plist->addRow(ar);
}
