#pragma once

#include <widgets/OBSQTDisplay.hpp>

#include <QObject>
#include <QPlatformSurfaceEvent>

class SurfaceEventFilter : public QObject {
	OBSQTDisplay *display;

public:
	SurfaceEventFilter(OBSQTDisplay *src) : QObject(src), display(src) {}

protected:
	bool eventFilter(QObject *obj, QEvent *event) override
	{
		bool result = QObject::eventFilter(obj, event);
		QPlatformSurfaceEvent *surfaceEvent;

		switch (event->type()) {
		case QEvent::PlatformSurface:
			surfaceEvent = static_cast<QPlatformSurfaceEvent *>(event);

			switch (surfaceEvent->surfaceEventType()) {
			case QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed:
				display->DestroyDisplay();
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}

		return result;
	}
};
