#include "OBSMenu.hpp"

#include <obs.hpp>

#include <QT>
#include <QWindow>
#include <QCursor>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#endif

// This context menu will automatically delete itself on close.
// No need to manually delete this.
OBSMenu::OBSMenu(QWidget *parent) : parent(parent)
{
	setAttribute(Qt::WA_DeleteOnClose);
}

void OBSMenu::showEvent(QShowEvent *event)
{
	HWND parentWindowHandle = (HWND)parent->winId();
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
	popup(mapFrom(parent, QCursor::pos()));
}
