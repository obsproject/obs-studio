#include "OBSClipboardService.hpp"

#include "OBSClipboardMimeTypes.hpp"
#include "OBSClipboardSerializer.hpp"

#include <QApplication>
#include <QClipboard>
#include <QMimeData>

bool OBSClipboardService::canPasteSceneItems(bool duplicate) const
{
	return false;
}

bool OBSClipboardService::canPasteFilters() const
{
	return false;
}

bool OBSClipboardService::canPasteTransform() const
{
	return false;
}

bool OBSClipboardService::canPasteTransition() const
{
	return false;
}

void OBSClipboardService::copySceneItems(const std::vector<OBSSceneItem> &items) {}

void OBSClipboardService::copyFilters(OBSSource source) {}

void OBSClipboardService::copyTransform(OBSSceneItem item) {}

void OBSClipboardService::copyTransition(OBSSceneItem item, bool show) {}

void OBSClipboardService::pasteSceneItems(OBSScene scene, bool duplicate) {}

void OBSClipboardService::pasteFilters(OBSSource destination) {}

void OBSClipboardService::pasteTransform(const std::vector<OBSSceneItem> &items) {}

void OBSClipboardService::pasteTransition(const std::vector<OBSSceneItem> &items, bool show) {}
