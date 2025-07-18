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

namespace {
static const char *indexToString[] = {
	"Basic.TransformWindow.Alignment.TopLeft",    "Basic.TransformWindow.Alignment.TopCenter",
	"Basic.TransformWindow.Alignment.TopRight",   "Basic.TransformWindow.Alignment.CenterLeft",
	"Basic.TransformWindow.Alignment.Center",     "Basic.TransformWindow.Alignment.CenterRight",
	"Basic.TransformWindow.Alignment.BottomLeft", "Basic.TransformWindow.Alignment.BottomCenter",
	"Basic.TransformWindow.Alignment.BottomRight"};
}

AccessibleAlignmentSelector::AccessibleAlignmentSelector(AlignmentSelector *widget)
	: QAccessibleWidget(widget, QAccessible::Grouping),
	  widget(widget)
{
	for (int i = 0; i < 9; ++i) {
		AccessibleAlignmentCell *cell = new AccessibleAlignmentCell(this, widget, i);
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
	return 9;
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
	return widget != nullptr;
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
	return widget->rect();
}

QString AccessibleAlignmentSelector::text(QAccessible::Text textType) const
{
	if (textType == QAccessible::Name) {
		QString str = widget->accessibleName();
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
	state.focused = widget->hasFocus();
	state.disabled = !widget->isEnabled();
	state.readOnly = false;

	return state;
}

QVariant AccessibleAlignmentSelector::value() const
{
	for (int i = 0; i < childCount(); ++i) {
		if (child(i)->state().checked) {
			return QTStr(indexToString[i]);
		}
	}

	return QTStr("None");
}

// --- AccessibleAlignmentCell ---
AccessibleAlignmentCell::AccessibleAlignmentCell(QAccessibleInterface *parent, AlignmentSelector *widget, int index)
	: parent_(parent),
	  widget(widget),
	  index_(index)
{
}

QRect AccessibleAlignmentCell::rect() const
{
	return widget->cellRect(index_);
}

QString AccessibleAlignmentCell::text(QAccessible::Text text) const
{
	if (text == QAccessible::Name || text == QAccessible::Value) {
		return QTStr(indexToString[index_]);
	}
	return QString();
}

QAccessible::State AccessibleAlignmentCell::state() const
{
	QAccessible::State state;
	bool enabled = widget->isEnabled();

	bool isSelectedCell = widget->currentIndex() == index_;
	bool isFocusedCell = widget->focusedCell == index_;

	state.disabled = !enabled;
	state.focusable = enabled;
	state.focused = widget->hasFocus() && isFocusedCell;
	state.checkable = true;
	state.checked = isSelectedCell;
	state.selected = isSelectedCell;

	return state;
}

QAccessible::Role AccessibleAlignmentCell::role() const
{
	return QAccessible::CheckBox;
}
