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

#include "SegmentButtonGroup.hpp"

SegmentButtonGroup::SegmentButtonGroup(QObject *parent) : QButtonGroup(parent) {}

void SegmentButtonGroup::addButton(SegmentButton *button, int buttonIndex)
{
	QButtonGroup::addButton(button, buttonIndex);
}

void SegmentButtonGroup::removeButton(SegmentButton *button)
{
	QButtonGroup::removeButton(button);
}

SegmentButton *SegmentButtonGroup::button(int id) const
{
	return static_cast<SegmentButton *>(QButtonGroup::button(id));
}

QList<SegmentButton *> SegmentButtonGroup::buttons() const
{
	QList<QAbstractButton *> buttonList = QButtonGroup::buttons();

	QList<SegmentButton *> segmentButtonList;
	segmentButtonList.reserve(buttonList.size());

	for (auto *button : buttonList) {
		auto *segmentButton = static_cast<SegmentButton *>(button);
		segmentButtonList.append(segmentButton);
	}

	return segmentButtonList;
}

SegmentButton *SegmentButtonGroup::checkedButton() const
{
	return static_cast<SegmentButton *>(QButtonGroup::checkedButton());
}
