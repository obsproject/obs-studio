/******************************************************************************
    Copyright (C) 2023 by Sebastian Beckmann

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

class DoubleClickFilter : public QObject {
	Q_OBJECT
public:
	inline DoubleClickFilter(QObject *parent = nullptr) : QObject(parent) {}

signals:
	void doubleClicked(QObject *obj);

protected:
	bool eventFilter(QObject *obj, QEvent *event) override
	{
		if (event->type() == QEvent::MouseButtonDblClick) {
			emit doubleClicked(obj);
			return true;
		} else {
			return QObject::eventFilter(obj, event);
		}
	}
};
