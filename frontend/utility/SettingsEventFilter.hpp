#pragma once

#include <utility/OBSEventFilter.hpp>
#include <OBSApp.hpp>

#include <QObject>
#include <QKeyEvent>

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
