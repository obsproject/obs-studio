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

#include "obs-widgets-base.hpp"
#include "obs-groupbox.hpp"

OBSGroupBox::OBSGroupBox(QWidget *parent) : QFrame(parent), OBSWidgetUtils(this)
{
	layout = new QVBoxLayout(this);
	layout->setSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);

	headerContainer = new QWidget();
	headerLayout = new QHBoxLayout();
	headerLayout->setSpacing(0);
	headerLayout->setContentsMargins(0, 0, 0, 0);
	headerContainer->setLayout(headerLayout);
	OBSWidgetUtils::addClass(headerContainer, "header");

	labelContainer = new QWidget();
	labelLayout = new QVBoxLayout();
	labelLayout->setSpacing(0);
	labelLayout->setContentsMargins(0, 0, 0, 0);
	labelContainer->setLayout(labelLayout);
	labelContainer->setSizePolicy(QSizePolicy::MinimumExpanding,
				      QSizePolicy::Preferred);

	controlContainer = new QWidget();
	controlLayout = new QVBoxLayout();
	controlLayout->setSpacing(0);
	controlLayout->setContentsMargins(0, 0, 0, 0);
	controlContainer->setLayout(controlLayout);

	headerLayout->addWidget(labelContainer);
	headerLayout->addWidget(controlContainer);

	contentsContainer = new QWidget();
	contentsLayout = new QVBoxLayout();
	contentsLayout->setSpacing(0);
	contentsLayout->setContentsMargins(0, 0, 0, 0);
	contentsContainer->setLayout(contentsLayout);
	OBSWidgetUtils::addClass(contentsContainer, "contents");

	layout->addWidget(headerContainer);
	layout->addWidget(contentsContainer);

	plist = new OBSPropertiesList(this);

	setLayout(layout);

	contentsLayout->addWidget(plist);
	/*contentsContainer->setSizePolicy(policy);*/

	nameLbl = new QLabel();
	OBSWidgetUtils::addClass(nameLbl, "title");
	nameLbl->setVisible(false);
	labelLayout->addWidget(nameLbl);

	descLbl = new QLabel();
	OBSWidgetUtils::addClass(descLbl, "description");
	descLbl->setVisible(false);
	labelLayout->addWidget(descLbl);
}

void OBSGroupBox::addRow(OBSActionBaseClass *ar) const
{
	plist->addRow(ar);
}

void OBSGroupBox::setTitle(QString name)
{
	nameLbl->setText(name);
	setAccessibleName(name);
	showTitle(true);
}

void OBSGroupBox::setDescription(QString desc)
{
	descLbl->setText(desc);
	setAccessibleDescription(desc);
	showDescription(true);
}

void OBSGroupBox::showTitle(bool visible)
{
	nameLbl->setVisible(visible);
}

void OBSGroupBox::showDescription(bool visible)
{
	descLbl->setVisible(visible);
}

void OBSGroupBox::setCheckable(bool check)
{
	checkable = check;

	if (checkable && !toggleSwitch) {
		toggleSwitch = new OBSToggleSwitch(true);
		controlLayout->addWidget(toggleSwitch);
		connect(toggleSwitch, &OBSToggleSwitch::toggled, this,
			[=](bool checked) { plist->setEnabled(checked); });
	}

	if (!checkable && toggleSwitch) {
		controlLayout->removeWidget(toggleSwitch);
		toggleSwitch->deleteLater();
	}
}
