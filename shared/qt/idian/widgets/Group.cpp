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

#include <Idian/Group.hpp>

#include <Idian/Utils.hpp>

#include <Idian/moc_Group.cpp>

using idian::Group;

Group::Group(QWidget *parent) : QFrame(parent), Utils(this)
{
	layout = new QVBoxLayout(this);
	layout->setSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);

	headerContainer = new QWidget();
	headerLayout = new QHBoxLayout();
	headerLayout->setSpacing(0);
	headerLayout->setContentsMargins(0, 0, 0, 0);
	headerContainer->setLayout(headerLayout);
	Utils::addClass(headerContainer, "header");

	labelContainer = new QWidget();
	labelLayout = new QVBoxLayout();
	labelLayout->setSpacing(0);
	labelLayout->setContentsMargins(0, 0, 0, 0);
	labelContainer->setLayout(labelLayout);
	labelContainer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

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
	Utils::addClass(contentsContainer, "contents");

	layout->addWidget(headerContainer);
	layout->addWidget(contentsContainer);

	propertyList = new PropertiesList(this);

	setLayout(layout);

	contentsLayout->addWidget(propertyList);

	nameLabel = new QLabel();
	Utils::addClass(nameLabel, "title");
	nameLabel->setVisible(false);
	labelLayout->addWidget(nameLabel);

	descriptionLabel = new QLabel();
	Utils::addClass(descriptionLabel, "description");
	descriptionLabel->setVisible(false);
	labelLayout->addWidget(descriptionLabel);
}

void Group::addRow(GenericRow *row) const
{
	propertyList->addRow(row);
}

void Group::setTitle(QString name)
{
	nameLabel->setText(name);
	setAccessibleName(name);
	showTitle(true);
}

void Group::setDescription(QString desc)
{
	descriptionLabel->setText(desc);
	setAccessibleDescription(desc);
	showDescription(true);
}

void Group::showTitle(bool visible)
{
	nameLabel->setVisible(visible);
}

void Group::showDescription(bool visible)
{
	descriptionLabel->setVisible(visible);
}

void Group::setCheckable(bool check)
{
	checkable = check;

	if (checkable && !toggleSwitch) {
		toggleSwitch = new ToggleSwitch(true);
		controlLayout->addWidget(toggleSwitch);
		connect(toggleSwitch, &ToggleSwitch::toggled, this,
			[this](bool checked) { propertyList->setEnabled(checked); });
	}

	if (!checkable && toggleSwitch) {
		controlLayout->removeWidget(toggleSwitch);
		toggleSwitch->deleteLater();
	}
}
