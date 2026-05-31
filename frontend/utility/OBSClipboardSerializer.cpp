#include "OBSClipboardSerializer.hpp"

OBSData OBSClipboardSerializer::SerializeSceneItem(OBSSceneItem item)
{
	if (!item) {
		return {};
	}

	return {};
}

OBSData OBSClipboardSerializer::SerializeFilters(OBSSource source)
{
	if (!source) {
		return {};
	}
	return {};
}

OBSData OBSClipboardSerializer::SerializeTransform(OBSSceneItem item)
{
	if (!item) {
		return {};
	}
	return {};
}

OBSData OBSClipboardSerializer::SerializeTransition(OBSSceneItem item, bool show)
{
	if (!item) {
		return {};
	}
	return {};
}
