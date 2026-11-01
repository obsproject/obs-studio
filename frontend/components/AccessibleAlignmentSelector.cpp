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

#include "AccessibleAlignmentSelector.hpp"

#include <OBSApp.hpp>

AccessibleAlignmentSelector::AccessibleAlignmentSelector(AlignmentSelector *widget_)
	: QAccessibleWidget(widget_, QAccessible::Grouping)
{
	for (int i = 0; i < cellCount; ++i) {
		AccessibleAlignmentCell *cell = new AccessibleAlignmentCell(this, widget_, i);
		QAccessible::registerAccessibleInterface(cell);
		cellInterfaces.insert(i, QAccessible::uniqueId(cell));
	}
}

AccessibleAlignmentSelector::~AccessibleAlignmentSelector()
{
	for (QAccessible::Id id : std::as_const(cellInterfaces)) {
		QAccessible::deleteAccessibleInterface(id);
	}
}

int AccessibleAlignmentSelector::childCount() const
{
	return cellCount;
}

QAccessibleInterface *AccessibleAlignmentSelector::child(int index) const
{
	if (QAccessible::Id id = cellInterfaces.value(index)) {
		return QAccessible::accessibleInterface(id);
	}

	return nullptr;
}

int AccessibleAlignmentSelector::indexOfChild(const QAccessibleInterface *child) const
{
	if (!child) {
		return -1;
	}

	QAccessible::Id id = QAccessible::uniqueId(const_cast<QAccessibleInterface *>(child));
	return cellInterfaces.key(id, -1);
}

bool AccessibleAlignmentSelector::isValid() const
{
	return widget() != nullptr;
}

QAccessibleInterface *AccessibleAlignmentSelector::focusChild() const
{
	for (int i = 0; i < childCount(); ++i) {
		if (child(i)->state().focused) {
			return child(i);
		}
	}
	return nullptr;
}

QRect AccessibleAlignmentSelector::rect() const
{
	return widget()->rect();
}

QString AccessibleAlignmentSelector::text(QAccessible::Text textType) const
{
	if (textType == QAccessible::Name) {
		QString str = widget()->accessibleName();
		if (str.isEmpty()) {
			str = QTStr("Accessible.Widget.Name.AlignmentSelector");
		}
		return str;
	}

	if (textType == QAccessible::Value) {
		return value().toString();
	}

	return QAccessibleWidget::text(textType);
}

QAccessible::Role AccessibleAlignmentSelector::role() const
{
	return QAccessible::Grouping;
}

QAccessible::State AccessibleAlignmentSelector::state() const
{
	QAccessible::State state;

	state.focusable = true;
	state.focused = widget()->hasFocus();
	state.disabled = !widget()->isEnabled();
	state.readOnly = false;

	return state;
}

QVariant AccessibleAlignmentSelector::value() const
{
	for (int i = 0; i < childCount(); ++i) {
		if (child(i)->state().checked) {
			return child(i)->text(QAccessible::Name);
		}
	}

	return QTStr("None");
}
