/******************************************************************************
    Copyright (C) 2025 by Taylor Giampaolo <warchamp7@obsproject.com>

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

#include <components/AccessibleAlignmentCell.hpp>

#include <QAccessible>
#include <QAccessibleInterface>
#include <QAccessibleWidget>

class AlignmentSelector;

class AccessibleAlignmentSelector : public QAccessibleWidget {
	mutable QHash<int, QAccessible::Id> cellInterfaces{};
	static constexpr int cellCount = 9;

public:
	explicit AccessibleAlignmentSelector(AlignmentSelector *widget);
	~AccessibleAlignmentSelector();

	QRect rect() const override;
	QAccessible::Role role() const override;
	QAccessible::State state() const override;
	QString text(QAccessible::Text t) const override;
	QAccessibleInterface *child(int index) const override;
	int childCount() const override;
	int indexOfChild(const QAccessibleInterface *child) const override;
	bool isValid() const override;
	QAccessibleInterface *focusChild() const override;

	QVariant value() const;
};
