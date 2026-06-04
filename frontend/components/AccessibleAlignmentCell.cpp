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

#include "AccessibleAlignmentCell.hpp"

#include <OBSApp.hpp>

using namespace std::string_view_literals;
constexpr std::array indexToStrings = {
	"Basic.TransformWindow.Alignment.TopLeft"sv,    "Basic.TransformWindow.Alignment.TopCenter"sv,
	"Basic.TransformWindow.Alignment.TopRight"sv,   "Basic.TransformWindow.Alignment.CenterLeft"sv,
	"Basic.TransformWindow.Alignment.Center"sv,     "Basic.TransformWindow.Alignment.CenterRight"sv,
	"Basic.TransformWindow.Alignment.BottomLeft"sv, "Basic.TransformWindow.Alignment.BottomCenter"sv,
	"Basic.TransformWindow.Alignment.BottomRight"sv};

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
		return QString(indexToStrings[index_].data());
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
