#include "OBSClipboardService.hpp"

bool OBSClipboardService::canPasteSceneItems(bool duplicate) const
{
	return false;
}

void OBSClipboardService::copySceneItems(const std::vector<OBSSceneItem> &items) {}

void OBSClipboardService::pasteSceneItems(OBSScene scene, bool duplicate) {}
