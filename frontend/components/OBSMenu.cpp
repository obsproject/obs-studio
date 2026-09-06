#include "OBSMenu.hpp"

#include <obs.hpp>

#include <QT>
#include <QWindow>
#include <QCursor>
#include <QString>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#endif

OBSMenu::OBSMenu(QWidget* parent) : parent(parent) {
	connect(parent, &QObject::destroyed, this, &QMenu::deleteLater);
}

// This context menu will automatically delete itself on close.
// No need to manually delete this.
OBSMenu::OBSMenu(QWidget *parent, const bool &deleteOnClose) : parent(parent)
{
	if (deleteOnClose) {
		setAttribute(Qt::WA_DeleteOnClose);
	} else {
		connect(parent, &QObject::destroyed, this, &QMenu::deleteLater);
	}
}

OBSMenu::OBSMenu(const QString &title, QWidget *parent, const bool &deleteOnClose) : parent(parent)
{
	setTitle(title);

	if (deleteOnClose) {
		setAttribute(Qt::WA_DeleteOnClose);
	} else {
		connect(parent, &QObject::destroyed, this, &QMenu::deleteLater);
	}
}

void OBSMenu::showEvent(QShowEvent *event)
{
	HWND parentWindowHandle;

	if (parent->isWindow()) {
		parentWindowHandle = (HWND)parent->winId();
	} else {
		parentWindowHandle = (HWND)parent->parentWidget()->winId();
	}

	DWORD currentDisplayAffinity;
	HWND contextWindowHandle = (HWND)windowHandle()->winId();

	if (GetWindowDisplayAffinity(parentWindowHandle, &currentDisplayAffinity) == TRUE) {
		switch (currentDisplayAffinity) {
		case WDA_EXCLUDEFROMCAPTURE:
			if (SetWindowDisplayAffinity(contextWindowHandle, WDA_EXCLUDEFROMCAPTURE) == FALSE) {
				blog(LOG_INFO, "Tried to hide from capture; could not set display affinity for Context Menu.");
			}
			break;
		case WDA_NONE:
		case WDA_MONITOR:
		default:
			if (SetWindowDisplayAffinity(contextWindowHandle, WDA_NONE) == FALSE) {
				blog(LOG_INFO, "Could not unhide from capture; could not set display affinity for Context Menu.");
			}
		}
	}
}

void OBSMenu::popupMenu() {
	popup(QCursor::pos());
}
