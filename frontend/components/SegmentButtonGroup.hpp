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

#include <components/SegmentButton.hpp>

#include <QButtonGroup>

class SegmentButtonGroup : public QButtonGroup {
	Q_OBJECT

public:
	SegmentButtonGroup(QObject *parent = nullptr);

	void addButton(SegmentButton *button, int id = -1);
	void removeButton(SegmentButton *button);

	SegmentButton *button(int id) const;
	QList<SegmentButton *> buttons() const;

	SegmentButton *checkedButton() const;
};
