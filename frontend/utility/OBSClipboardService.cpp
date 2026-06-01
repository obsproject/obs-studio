#include "OBSClipboardService.hpp"

#include "OBSClipboardMimeTypes.hpp"
#include "OBSClipboardSerializer.hpp"

#include <QApplication>
#include <QClipboard>
#include <QMimeData>

namespace {
OBSDataAutoRelease CloneData(const OBSDataAutoRelease &data)
{
	if (!data) {
		return {};
	}

	return obs_data_create_from_json(obs_data_get_json(data));
}

OBSSourceAutoRelease ResolveReferencedSource(const OBSDataAutoRelease &sourceData)
{
	const char *uuid = obs_data_get_string(sourceData, "uuid");
	if (uuid && *uuid) {
		return obs_get_source_by_uuid(uuid);
	}

	const char *name = obs_data_get_string(sourceData, "name");
	if (name && *name) {
		return obs_get_source_by_name(name);
	}

	return {};
}

void PrepareRootSceneItem(obs_data_array_t *items, obs_source_t *source = nullptr)
{
	const size_t count = obs_data_array_count(items);

	for (size_t i = 0; i < count; i++) {
		OBSDataAutoRelease item = obs_data_array_item(items, i);

		if (obs_data_get_bool(item, "group_item_backup")) {
			continue;
		}

		obs_data_erase(item, "id");

		if (source) {
			obs_data_set_string(item, "name", obs_source_get_name(source));
			obs_data_set_string(item, "source_uuid", obs_source_get_uuid(source));
		}

		return;
	}
}
} // namespace

bool OBSClipboardService::canPasteSceneItems(bool duplicate) const
{
	OBSData payload = getMimeData(OBSClipboard::SceneItems);
	if (!payload) {
		return false;
	}

	OBSDataArrayAutoRelease serializedItems = obs_data_get_array(payload, "items");
	if (!serializedItems) {
		return false;
	}

	const size_t count = obs_data_array_count(serializedItems);
	for (size_t i = 0; i < count; i++) {
		OBSDataAutoRelease serializedItem = obs_data_array_item(serializedItems, i);

		OBSDataArrayAutoRelease items;
		OBSDataAutoRelease sourceData;
		uint32_t outputFlags = 0;

		if (!OBSClipboardSerializer::DeserializeSceneItem(serializedItem, items, sourceData, outputFlags)) {
			continue;
		}

		const bool requiresReference = !duplicate || (outputFlags & OBS_SOURCE_DO_NOT_DUPLICATE);

		if (!requiresReference || ResolveReferencedSource(sourceData)) {
			return true;
		}
	}

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

void OBSClipboardService::copySceneItems(const std::vector<OBSSceneItem> &items)
{
	OBSDataArrayAutoRelease serializedItems = obs_data_array_create();
	for (OBSSceneItem item : items) {
		OBSData data = OBSClipboardSerializer::SerializeSceneItem(item);
		if (data) {
			obs_data_array_push_back(serializedItems, data);
		}
	}

	if (!obs_data_array_count(serializedItems)) {
		return;
	}

	OBSDataAutoRelease payload = obs_data_create();
	obs_data_set_array(payload, "items", serializedItems);
	setMimeData(OBSClipboard::SceneItems, OBSData(payload.Get()));
}

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

void OBSClipboardService::pasteSceneItems(OBSScene scene, bool duplicate)
{
	if (!scene) {
		return;
	}
	OBSData payload = getMimeData(OBSClipboard::SceneItems);
	if (!payload) {
		return;
	}
	OBSDataArrayAutoRelease serializedItems = obs_data_get_array(payload, "items");
	if (!serializedItems) {
		return;
	}

	for (size_t i = obs_data_array_count(serializedItems); i > 0; i--) {
		OBSDataAutoRelease serializedItem = obs_data_array_item(serializedItems, i - 1);
		OBSDataAutoRelease copy = CloneData(serializedItem);
		if (!copy) {
			continue;
		}

		OBSDataArrayAutoRelease items;
		OBSDataAutoRelease sourceData;
		uint32_t outputFlags = 0;

		if (!OBSClipboardSerializer::DeserializeSceneItem(copy, items, sourceData, outputFlags)) {
			continue;
		}

		const bool useReference = !duplicate || (outputFlags & OBS_SOURCE_DO_NOT_DUPLICATE);

		if (useReference) {
			OBSSourceAutoRelease source = ResolveReferencedSource(sourceData);
			if (!source) {
				continue;
			}

			const char *name = obs_source_get_name(source);

			if (obs_scene_get_group(scene, name)) {
				continue;
			}

			PrepareRootSceneItem(items);
			obs_sceneitems_add(scene, items);
			continue;
		}

		obs_data_erase(sourceData, "uuid");

		const char *id = obs_data_get_string(sourceData, "id");
		if (!id || !*id) {
			continue;
		}

		OBSSourceAutoRelease source = obs_load_source(sourceData);
		if (!source) {
			continue;
		}

		obs_source_load2(source);
		PrepareRootSceneItem(items, source);
		obs_sceneitems_add(scene, items);
	}
}

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
