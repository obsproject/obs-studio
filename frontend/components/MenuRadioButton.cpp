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

#include "MenuRadioButton.hpp"

#include <QMenu>
#include <QMouseEvent>
#include <QStyleOptionButton>
#include <QStylePainter>

MenuRadioButton::MenuRadioButton(const QString &text, QWidget *parent) : QRadioButton(text, parent)
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	setContentsMargins(0, 0, 0, 0);

	if (auto menu = qobject_cast<QMenu *>(parent)) {
		connect(menu, &QMenu::aboutToShow, this, [this]() {
			setHovered(false);
			update();
		});
		connect(menu, &QMenu::aboutToHide, this, [this]() {
			setHovered(false);
			update();
		});
	}
}

void MenuRadioButton::focusInEvent(QFocusEvent *)
{
	setHovered(true);
}

void MenuRadioButton::focusOutEvent(QFocusEvent *)
{
	setHovered(false);
}

void MenuRadioButton::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		mousePressInside = true;
		event->accept();
	} else {
		QRadioButton::mousePressEvent(event);
	}
}

void MenuRadioButton::mouseMoveEvent(QMouseEvent *event)
{
	if (!rect().contains(event->pos())) {
		mousePressInside = false;
	}
	event->accept();
}

void MenuRadioButton::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		if (mousePressInside && rect().contains(event->pos())) {
			toggle();
		}
		event->accept();
	} else {
		QRadioButton::mouseReleaseEvent(event);
	}

	mousePressInside = false;
}

void MenuRadioButton::enterEvent(QEnterEvent *)
{
	setHovered(true);
	update();
}

void MenuRadioButton::leaveEvent(QEvent *)
{
	setHovered(false);
	update();
}

void MenuRadioButton::paintEvent(QPaintEvent *)
{
	QStylePainter p(this);
	QStyleOptionButton opt;
	initStyleOption(&opt);

	if (isHovered) {
		opt.state |= QStyle::State_MouseOver;
		opt.state |= QStyle::State_Selected;
	}

	p.drawControl(QStyle::CE_RadioButton, opt);
}

void MenuRadioButton::setHovered(bool hovered)
{
	isHovered = hovered;
}
