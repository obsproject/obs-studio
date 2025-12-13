/******************************************************************************
    Copyright (C) 2014-2015 by Ruwen Hahn <palana@stunned.de>

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

#include "OBSHotkeyLabel.hpp"
#include "OBSHotkeyWidget.hpp"

#include <QEnterEvent>
#include <QStyle>

#include "moc_OBSHotkeyLabel.cpp"

static inline void updateStyle(QWidget *widget)
{
	auto style = widget->style();
	style->unpolish(widget);
	style->polish(widget);
	widget->update();
}

void OBSHotkeyLabel::highlightPair(bool highlight)
{
	if (!pairPartner)
		return;

	pairPartner->setProperty("class", highlight ? "text-bright" : "");
	updateStyle(pairPartner);
	setProperty("class", highlight ? "text-bright" : "");
	updateStyle(this);
}

void OBSHotkeyLabel::enterEvent(QEnterEvent *event)
{

	if (!pairPartner)
		return;

	event->accept();
	highlightPair(true);
}

void OBSHotkeyLabel::leaveEvent(QEvent *event)
{
	if (!pairPartner)
		return;

	event->accept();
	highlightPair(false);
}

void OBSHotkeyLabel::setToolTip(const QString &toolTip)
{
	QLabel::setToolTip(toolTip);
	if (widget)
		widget->setToolTip(toolTip);
}
