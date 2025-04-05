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

#include "OBSIdianWidget.hpp"
#include "OBSGroupBox.hpp"

OBSGroupBox::OBSGroupBox(QWidget *parent) : QFrame(parent), OBSIdianUtils(this)
{
	layout = new QVBoxLayout(this);
	layout->setSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);

	headerContainer = new QWidget();
	headerLayout = new QHBoxLayout();
	headerLayout->setSpacing(0);
	headerLayout->setContentsMargins(0, 0, 0, 0);
	headerContainer->setLayout(headerLayout);
	OBSIdianUtils::addClass(headerContainer, "header");

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
	OBSIdianUtils::addClass(contentsContainer, "contents");

	layout->addWidget(headerContainer);
	layout->addWidget(contentsContainer);

	propertyList = new OBSPropertiesList(this);

	setLayout(layout);

	contentsLayout->addWidget(propertyList);
	/*contentsContainer->setSizePolicy(policy);*/

	nameLabel = new QLabel();
	OBSIdianUtils::addClass(nameLabel, "title");
	nameLabel->setVisible(false);
	labelLayout->addWidget(nameLabel);

	descriptionLabel = new QLabel();
	OBSIdianUtils::addClass(descriptionLabel, "description");
	descriptionLabel->setVisible(false);
	labelLayout->addWidget(descriptionLabel);
}

void OBSGroupBox::addRow(OBSActionRow *actionRow) const
{
	propertyList->addRow(actionRow);
}

void OBSGroupBox::setTitle(QString name)
{
	nameLabel->setText(name);
	setAccessibleName(name);
	showTitle(true);
}

void OBSGroupBox::setDescription(QString desc)
{
	descriptionLabel->setText(desc);
	setAccessibleDescription(desc);
	showDescription(true);
}

void OBSGroupBox::showTitle(bool visible)
{
	nameLabel->setVisible(visible);
}

void OBSGroupBox::showDescription(bool visible)
{
	descriptionLabel->setVisible(visible);
}

void OBSGroupBox::setCheckable(bool check)
{
	checkable = check;

	if (checkable && !toggleSwitch) {
		toggleSwitch = new OBSToggleSwitch(true);
		controlLayout->addWidget(toggleSwitch);
		connect(toggleSwitch, &OBSToggleSwitch::toggled, this,
			[=](bool checked) { propertyList->setEnabled(checked); });
	}

	if (!checkable && toggleSwitch) {
		controlLayout->removeWidget(toggleSwitch);
		toggleSwitch->deleteLater();
	}
}
