/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>
                          Philippe Groarke <philippe.groarke@gmail.com>

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

#include <OBSApp.hpp>
#include <utility/OBSEventFilter.hpp>

#include <QKeyEvent>
#include <QObject>

class SettingsEventFilter : public QObject {
	QScopedPointer<OBSEventFilter> shortcutFilter;

public:
	inline SettingsEventFilter() : shortcutFilter((OBSEventFilter *)CreateShortcutFilter()) {}

protected:
	bool eventFilter(QObject *obj, QEvent *event) override
	{
		int key;

		switch (event->type()) {
		case QEvent::KeyPress:
		case QEvent::KeyRelease:
			key = static_cast<QKeyEvent *>(event)->key();
			if (key == Qt::Key_Escape) {
				return false;
			}
		default:
			break;
		}

		return shortcutFilter->filter(obj, event);
	}
};
