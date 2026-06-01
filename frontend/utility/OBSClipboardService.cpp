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
	return !!getMimeData(OBSClipboard::SourceFilters);
}

bool OBSClipboardService::canPasteTransform() const
{
	return !!getMimeData(OBSClipboard::SceneItemTransform);
}

bool OBSClipboardService::canPasteTransition() const
{
	return !!getMimeData(OBSClipboard::SceneItemTransition);
}

void OBSClipboardService::copySceneItems(const std::vector<OBSSceneItem> &items) {}

void OBSClipboardService::copyFilters(OBSSource source)
{
	if (!source) {
		return;
	}
	OBSData payload = OBSClipboardSerializer::SerializeFilters(source);
	setMimeData(OBSClipboard::SourceFilters, payload);
}

void OBSClipboardService::copyTransform(OBSSceneItem item)
{
	if (!item) {
		return;
	}
	OBSData payload = OBSClipboardSerializer::SerializeTransform(item);
	setMimeData(OBSClipboard::SceneItemTransform, payload);
}

void OBSClipboardService::copyTransition(OBSSceneItem item, bool show)
{
	OBSData payload = OBSClipboardSerializer::SerializeTransition(item, show);
	setMimeData(OBSClipboard::SceneItemTransition, payload);
}

void OBSClipboardService::pasteSceneItems(OBSScene scene, bool duplicate) {}

void OBSClipboardService::pasteFilters(OBSSource destination)
{
	if (!destination) {
		return;
	}
	OBSData payload = getMimeData(OBSClipboard::SourceFilters);
	if (!payload) {
		return;
	}
	OBSDataArrayAutoRelease filters;
	if (!OBSClipboardSerializer::DeserializeFilters(payload, filters)) {
		return;
	}
	obs_source_restore_filters(destination, filters);
}

void OBSClipboardService::pasteTransform(const std::vector<OBSSceneItem> &items)
{
	OBSData payload = getMimeData(OBSClipboard::SceneItemTransform);
	if (!payload) {
		return;
	}

	obs_transform_info transform;
	obs_sceneitem_crop crop;

	if (!OBSClipboardSerializer::DeserializeTransform(payload, transform, crop))
		return;

	for (OBSSceneItem item : items) {
		if (!item || obs_sceneitem_locked(item))
			continue;

		obs_sceneitem_defer_update_begin(item);
		obs_sceneitem_set_info2(item, &transform);
		obs_sceneitem_set_crop(item, &crop);
		obs_sceneitem_defer_update_end(item);
	}
}

void OBSClipboardService::pasteTransition(const std::vector<OBSSceneItem> &items, bool show)
{
	OBSData payload = getMimeData(OBSClipboard::SceneItemTransition);
	if (!payload) {
		return;
	}
	for (OBSSceneItem item : items) {
		if (item) {
			obs_sceneitem_transition_load(item, payload, show);
		}
	}
}

void OBSClipboardService::setMimeData(const char *mimeType, const OBSData &payload)
{
	if (!mimeType) {
		return;
	}
	if (!payload) {
		return;
	}
	QMimeData *mimeData = new QMimeData();

	OBSDataAutoRelease envelope = obs_data_create();
	obs_data_set_int(envelope, "version", OBSClipboard::PayloadVersion);
	obs_data_set_obj(envelope, "payload", payload);

	mimeData->setData(mimeType, QByteArray(obs_data_get_json(envelope)));

	QApplication::clipboard()->setMimeData(mimeData);
}

OBSData OBSClipboardService::getMimeData(const char *mimeType) const
{
	if (!mimeType) {
		return {};
	}
	const QMimeData *mimeData = QApplication::clipboard()->mimeData();
	if (!mimeData || !mimeData->hasFormat(mimeType)) {
		return {};
	}
	QByteArray data = mimeData->data(mimeType);
	OBSDataAutoRelease envelope = obs_data_create_from_json(data.constData());
	if (!envelope) {
		return {};
	}
	if (obs_data_get_int(envelope, "version") != OBSClipboard::PayloadVersion) {
		return {};
	}
	OBSDataAutoRelease payload = obs_data_get_obj(envelope, "payload");

	return OBSData(payload);
}
