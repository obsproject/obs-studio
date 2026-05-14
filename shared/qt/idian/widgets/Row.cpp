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

#include <Idian/ComboBox.hpp>
#include <Idian/DoubleSpinBox.hpp>
#include <Idian/ExpandButton.hpp>
#include <Idian/RowList.hpp>
#include <Idian/Row.hpp>
#include <Idian/SpinBox.hpp>
#include <Idian/ToggleSwitch.hpp>
#include <Idian/Utils.hpp>

#include <QApplication>
#include <QComboBox>
#include <QSizePolicy>
#include <QSvgRenderer>

#include <Idian/moc_Row.cpp>

namespace idian {
Row::Row(QWidget *parent) : QFrame(parent), Utils(this)
{
	rowLayout = new QHBoxLayout(this);
	rowLayout->setContentsMargins(0, 0, 0, 0);

	setLayout(rowLayout);
}

void Row::setBuddy(QWidget *widget)
{
	buddyWidget = widget;
	Utils::addClass(widget, "row-buddy");
	connectBuddyWidget(widget);
}

void Row::setChangeCursor(bool change)
{
	changeCursor = change;
	Utils::toggleClass("cursor-pointer", change);
}

void Row::enterEvent(QEnterEvent *event)
{
	if (!isEnabled())
		return;

	if (changeCursor) {
		setCursor(Qt::PointingHandCursor);
	}

	if (buddyWidget)
		Utils::repolish(buddyWidget);

	QFrame::enterEvent(event);
}

void Row::leaveEvent(QEvent *event)
{
	if (buddyWidget)
		Utils::repolish(buddyWidget);

	QFrame::leaveEvent(event);
}

void Row::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() & Qt::LeftButton) {
		emit clicked();
	}
	QFrame::mouseReleaseEvent(event);
}

void Row::keyReleaseEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Enter) {
		emit clicked();
	}
	QFrame::keyReleaseEvent(event);
}

void Row::connectBuddyWidget(QWidget *widget)
{
	setAttribute(Qt::WA_Hover, true);
	setFocusPolicy(Qt::StrongFocus);
	applyStateStylingEventFilter(this);

	// If element is a ToggleSwitch and checkable, forward clicks to the widget
	ToggleSwitch *obsToggle = dynamic_cast<ToggleSwitch *>(widget);
	if (obsToggle && obsToggle->isCheckable()) {
		setChangeCursor(true);
		widget->setFocusProxy(this);

		connect(this, &Row::clicked, obsToggle, &ToggleSwitch::click);
		return;
	}

	// If element is any other QAbstractButton subclass, and checkable, forward clicks to the widget.
	QAbstractButton *button = dynamic_cast<QAbstractButton *>(widget);
	if (button && button->isCheckable()) {
		setChangeCursor(true);
		widget->setFocusProxy(this);

		connect(this, &Row::clicked, button, &QAbstractButton::click);
		return;
	}

	// If element is an ComboBox, clicks toggle the dropdown.
	ComboBox *obsCombo = dynamic_cast<ComboBox *>(widget);
	if (obsCombo) {
		setChangeCursor(true);
		widget->setFocusProxy(this);

		connect(this, &Row::clicked, obsCombo, &ComboBox::togglePopup);
		return;
	}
}
} // namespace idian
