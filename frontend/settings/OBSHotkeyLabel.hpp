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

#pragma once

#include <QLabel>
#include <QPointer>

class OBSHotkeyWidget;

class OBSHotkeyLabel : public QLabel {
	Q_OBJECT

public:
	QPointer<OBSHotkeyLabel> pairPartner;
	QPointer<OBSHotkeyWidget> widget;
	void highlightPair(bool highlight);
	void enterEvent(QEnterEvent *event) override;
	void leaveEvent(QEvent *event) override;
	void setToolTip(const QString &toolTip);
};
