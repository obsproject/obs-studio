#include "OBSContextMenu.hpp"

#include <obs.hpp>

#include <QT>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#endif

OBSContextMenu::OBSContextMenu(QWidget *parent)
{
	setParent(parent);
	setAttribute(Qt::WA_DeleteOnClose);
}

void OBSContextMenu::showEvent(QShowEvent *event)
{
	HWND parentWindowId = (HWND)parentWidget()->winId();
	DWORD parentDisplayAffinity;
	HWND windowId = (HWND)winId();

	if (GetWindowDisplayAffinity(parentWindowId, &parentDisplayAffinity)) {
		switch (parentDisplayAffinity) {
		case WDA_EXCLUDEFROMCAPTURE:
			if (!SetWindowDisplayAffinity(windowId, WDA_EXCLUDEFROMCAPTURE)) {
				blog(LOG_INFO, "Tried to hide from capture; could not set display affinity for Context Menu.");
			}
			break;
		case WDA_NONE:
		case WDA_MONITOR:
		default:
			if (!SetWindowDisplayAffinity(windowId, WDA_NONE)) {
				blog(LOG_INFO, "Could not unhide from capture; could not set display affinity for Context Menu.");
			}
		}
	}
}
