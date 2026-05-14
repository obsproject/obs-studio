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

#include <Idian/CollapsibleGroup.hpp>
#include <Idian/ExpandButton.hpp>
#include <Idian/Row.hpp>
#include <Idian/RowList.hpp>
#include <Idian/RowInfo.hpp>
#include <Idian/ToggleSwitch.hpp>
#include <Idian/Utils.hpp>

#include <QPixmap>
#include <QVBoxLayout>

namespace idian {
CollapsibleGroup::CollapsibleGroup(QWidget *parent) : QFrame(parent), Utils(this)
{
	mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);
	setLayout(mainLayout);

	rowWidget = new Row(this);

	mainLayout->addWidget(rowWidget);

	auto *headerInfo = new RowInfo(rowWidget);
	rowWidget->layout()->addWidget(headerInfo);

	expandButton = new ExpandButton(this);
	rowWidget->layout()->addWidget(expandButton);
	rowWidget->setBuddy(expandButton);

	propertyList = new RowList(this);
	propertyList->setVisible(false);
	mainLayout->addWidget(propertyList);

	connect(expandButton, &QAbstractButton::clicked, this, &CollapsibleGroup::toggleVisibility);
}

void CollapsibleGroup::setCheckable(bool check)
{
	checkable = check;

	if (checkable && !toggleSwitch) {
		propertyList->setEnabled(false);
		Utils::polishChildren(propertyList);

		toggleSwitch = new ToggleSwitch(false);

		rowWidget->layout()->insertWidget(rowWidget->layout()->count() - 1, toggleSwitch);
		connect(toggleSwitch, &ToggleSwitch::toggled, propertyList, &RowList::setEnabled);
		connect(toggleSwitch, &ToggleSwitch::toggled, this, &CollapsibleGroup::toggled);
	}

	if (!checkable && toggleSwitch) {
		propertyList->setEnabled(true);
		Utils::polishChildren(propertyList);

		toggleSwitch->deleteLater();
	}
}

void CollapsibleGroup::setChecked(bool checked)
{
	if (!isCheckable()) {
		throw std::logic_error("Called setChecked on a non-checkable collapse row.");
	}

	toggleSwitch->setChecked(checked);
}

bool CollapsibleGroup::isChecked()
{
	return toggleSwitch->isChecked();
}

void CollapsibleGroup::toggleVisibility()
{
	bool visible = !propertyList->isVisible();

	setExpanded(visible);
}

void CollapsibleGroup::setExpanded(bool expand)
{
	Utils::toggleClass("expanded", expand);
	Utils::repolish(rowWidget);

	propertyList->setVisible(expand);
	expandButton->setChecked(expand);
}
} // namespace idian
