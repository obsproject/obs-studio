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

#include <Idian/ListHeader.hpp>
#include <Idian/Utils.hpp>

#include <Idian/moc_ListHeader.cpp>

using idian::ListHeader;

ListHeader::ListHeader(QWidget *parent) : QFrame(parent), Utils(this)
{
	widgetUtils = new idian::Utils(this);

	layout_ = new QHBoxLayout();
	layout_->setSpacing(0);
	layout_->setContentsMargins(0, 0, 0, 0);

	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

	setLayout(layout_);

	auto *textLayout = new QVBoxLayout();
	nameLabel = new QLabel();
	nameLabel->setVisible(false);
	widgetUtils->addClass(nameLabel, "title");

	descriptionLabel = new QLabel();
	descriptionLabel->setVisible(false);
	widgetUtils->addClass(descriptionLabel, "description");

	textLayout->addWidget(nameLabel);
	textLayout->addWidget(descriptionLabel);

	layout_->addLayout(textLayout);
}

ListHeader::ListHeader(QWidget *parent, QString title) : ListHeader(parent)
{
	setTitle(title);
}

ListHeader::ListHeader(QWidget *parent, QString title, QString description) : ListHeader(parent)
{
	setTitle(title);
	setDescription(description);
}

void ListHeader::setTitle(QString name)
{
	nameLabel->setText(name);
	setAccessibleName(name);
	showTitle(true);
}

void ListHeader::setDescription(QString desc)
{
	descriptionLabel->setText(desc);
	setAccessibleDescription(desc);
	showDescription(true);
}

void ListHeader::showTitle(bool visible)
{
	nameLabel->setVisible(visible);
}

void ListHeader::showDescription(bool visible)
{
	descriptionLabel->setVisible(visible);
}

void ListHeader::setCheckable(bool check)
{
	checkable = check;

	if (checkable && !toggleSwitch) {
		toggleSwitch = new ToggleSwitch(true);
		layout_->addWidget(toggleSwitch);
		connect(toggleSwitch, &ToggleSwitch::toggled, this, [this](bool checked) { emit toggled(checked); });
	}

	if (!checkable && toggleSwitch) {
		layout_->removeWidget(toggleSwitch);
		toggleSwitch->deleteLater();
	}
}
