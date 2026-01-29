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

#include "MenuCheckBox.hpp"

#include <QMenu>
#include <QMouseEvent>
#include <QStyleOptionButton>
#include <QStylePainter>

MenuCheckBox::MenuCheckBox(const QString &text, QWidget *parent) : QCheckBox(text, parent)
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	setContentsMargins(0, 0, 0, 0);

	if (auto menu = qobject_cast<QMenu *>(parent)) {
		connect(menu, &QMenu::hovered, this, [this, menu](QAction *action) {
			if (action != menuAction) {
				setHovered(false);
				update();
			}
		});
		connect(menu, &QMenu::aboutToHide, this, [this]() { setHovered(false); });
	}
}

void MenuCheckBox::setAction(QAction *action)
{
	menuAction = action;
}

void MenuCheckBox::focusInEvent(QFocusEvent *)
{
	setHovered(true);
}

void MenuCheckBox::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		mousePressInside = true;
		event->accept();
	} else {
		QCheckBox::mousePressEvent(event);
	}
}

void MenuCheckBox::mouseMoveEvent(QMouseEvent *event)
{
	if (!rect().contains(event->pos())) {
		mousePressInside = false;
	}
	event->accept();
}

void MenuCheckBox::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		if (mousePressInside && rect().contains(event->pos())) {
			toggle();
		}
		event->accept();
	} else {
		QCheckBox::mouseReleaseEvent(event);
	}

	mousePressInside = false;
}

void MenuCheckBox::enterEvent(QEnterEvent *)
{
	setHovered(true);
	update();
}

void MenuCheckBox::leaveEvent(QEvent *)
{
	setHovered(false);
	update();
}

void MenuCheckBox::paintEvent(QPaintEvent *)
{
	QStylePainter p(this);
	QStyleOptionButton opt;
	initStyleOption(&opt);

	if (isHovered) {
		opt.state |= QStyle::State_MouseOver;
		opt.state |= QStyle::State_Selected;
	}

	p.drawControl(QStyle::CE_CheckBox, opt);
}

void MenuCheckBox::setHovered(bool hovered)
{
	isHovered = hovered;
}
