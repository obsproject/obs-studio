/*
obs-websocket
Copyright (C) 2016-2021 Stephane Lepin <stephane.lepin@gmail.com>
Copyright (C) 2020-2021 Kyle Manning <tt2468@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include "EventHandler.h"

/**
 * A scene item has been created.
 *
 * @dataField sceneName      | String | Name of the scene the item was added to
 * @dataField sceneUuid      | String | UUID of the scene the item was added to
 * @dataField sourceName     | String | Name of the underlying source (input/scene)
 * @dataField sourceUuid     | String | UUID of the underlying source (input/scene)
 * @dataField sceneItemId    | Number | Numeric ID of the scene item
 * @dataField sceneItemIndex | Number | Index position of the item
 *
 * @eventType SceneItemCreated
 * @eventSubscription SceneItems
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category scene items
 */
void EventHandler::HandleSceneItemCreated(void *param, calldata_t *data)
{
	auto eventHandler = static_cast<EventHandler *>(param);

	obs_scene_t *scene = GetCalldataPointer<obs_scene_t>(data, "scene");
	if (!scene)
		return;

	obs_sceneitem_t *sceneItem = GetCalldataPointer<obs_sceneitem_t>(data, "item");
	if (!sceneItem)
		return;

	json eventData;
	eventData["sceneName"] = obs_source_get_name(obs_scene_get_source(scene));
	eventData["sceneUuid"] = obs_source_get_uuid(obs_scene_get_source(scene));
	eventData["sourceName"] = obs_source_get_name(obs_sceneitem_get_source(sceneItem));
	eventData["sourceUuid"] = obs_source_get_uuid(obs_sceneitem_get_source(sceneItem));
	eventData["sceneItemId"] = obs_sceneitem_get_id(sceneItem);
	eventData["sceneItemIndex"] = obs_sceneitem_get_order_position(sceneItem);
	eventHandler->BroadcastEvent(EventSubscription::SceneItems, "SceneItemCreated", eventData);
}

/**
 * A scene item has been removed.
 *
 * This event is not emitted when the scene the item is in is removed.
 *
 * @dataField sceneName   | String | Name of the scene the item was removed from
 * @dataField sceneUuid   | String | UUID of the scene the item was removed from
 * @dataField sourceName  | String | Name of the underlying source (input/scene)
 * @dataField sourceUuid  | String | UUID of the underlying source (input/scene)
 * @dataField sceneItemId | Number | Numeric ID of the scene item
 *
 * @eventType SceneItemRemoved
 * @eventSubscription SceneItems
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category scene items
 */
void EventHandler::HandleSceneItemRemoved(void *param, calldata_t *data)
{
	auto eventHandler = static_cast<EventHandler *>(param);

	obs_scene_t *scene = GetCalldataPointer<obs_scene_t>(data, "scene");
	if (!scene)
		return;

	obs_sceneitem_t *sceneItem = GetCalldataPointer<obs_sceneitem_t>(data, "item");
	if (!sceneItem)
		return;

	json eventData;
	eventData["sceneName"] = obs_source_get_name(obs_scene_get_source(scene));
	eventData["sceneUuid"] = obs_source_get_uuid(obs_scene_get_source(scene));
	eventData["sourceName"] = obs_source_get_name(obs_sceneitem_get_source(sceneItem));
	eventData["sourceUuid"] = obs_source_get_uuid(obs_sceneitem_get_source(sceneItem));
	eventData["sceneItemId"] = obs_sceneitem_get_id(sceneItem);
	eventHandler->BroadcastEvent(EventSubscription::SceneItems, "SceneItemRemoved", eventData);
}

/**
 * A scene's item list has been reindexed.
 *
 * @dataField sceneName  | String        | Name of the scene
 * @dataField sceneUuid  | String        | UUID of the scene
 * @dataField sceneItems | Array<Object> | Array of scene item objects
 *
 * @eventType SceneItemListReindexed
 * @eventSubscription SceneItems
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category scene items
 */
void EventHandler::HandleSceneItemListReindexed(void *param, calldata_t *data)
{
	auto eventHandler = static_cast<EventHandler *>(param);

	obs_scene_t *scene = GetCalldataPointer<obs_scene_t>(data, "scene");
	if (!scene)
		return;

	json eventData;
	eventData["sceneName"] = obs_source_get_name(obs_scene_get_source(scene));
	eventData["sceneUuid"] = obs_source_get_uuid(obs_scene_get_source(scene));
	eventData["sceneItems"] = Utils::Obs::ArrayHelper::GetSceneItemList(scene, true);
	eventHandler->BroadcastEvent(EventSubscription::SceneItems, "SceneItemListReindexed", eventData);
}

/**
 * A scene item's enable state has changed.
 *
 * @dataField sceneName        | String  | Name of the scene the item is in
 * @dataField sceneUuid        | String  | UUID of the scene the item is in
 * @dataField sceneItemId      | Number  | Numeric ID of the scene item
 * @dataField sceneItemEnabled | Boolean | Whether the scene item is enabled (visible)
 *
 * @eventType SceneItemEnableStateChanged
 * @eventSubscription SceneItems
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category scene items
 */
void EventHandler::HandleSceneItemEnableStateChanged(void *param, calldata_t *data)
{
	auto eventHandler = static_cast<EventHandler *>(param);

	obs_scene_t *scene = GetCalldataPointer<obs_scene_t>(data, "scene");
	if (!scene)
		return;

	obs_sceneitem_t *sceneItem = GetCalldataPointer<obs_sceneitem_t>(data, "item");
	if (!sceneItem)
		return;

	bool sceneItemEnabled = calldata_bool(data, "visible");

	json eventData;
	eventData["sceneName"] = obs_source_get_name(obs_scene_get_source(scene));
	eventData["sceneUuid"] = obs_source_get_uuid(obs_scene_get_source(scene));
	eventData["sceneItemId"] = obs_sceneitem_get_id(sceneItem);
	eventData["sceneItemEnabled"] = sceneItemEnabled;
	eventHandler->BroadcastEvent(EventSubscription::SceneItems, "SceneItemEnableStateChanged", eventData);
}

/**
 * A scene item's lock state has changed.
 *
 * @dataField sceneName       | String  | Name of the scene the item is in
 * @dataField sceneUuid       | String  | UUID of the scene the item is in
 * @dataField sceneItemId     | Number  | Numeric ID of the scene item
 * @dataField sceneItemLocked | Boolean | Whether the scene item is locked
 *
 * @eventType SceneItemLockStateChanged
 * @eventSubscription SceneItems
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category scene items
 */
void EventHandler::HandleSceneItemLockStateChanged(void *param, calldata_t *data)
{
	auto eventHandler = static_cast<EventHandler *>(param);

	obs_scene_t *scene = GetCalldataPointer<obs_scene_t>(data, "scene");
	if (!scene)
		return;

	obs_sceneitem_t *sceneItem = GetCalldataPointer<obs_sceneitem_t>(data, "item");
	if (!sceneItem)
		return;

	bool sceneItemLocked = calldata_bool(data, "locked");

	json eventData;
	eventData["sceneName"] = obs_source_get_name(obs_scene_get_source(scene));
	eventData["sceneUuid"] = obs_source_get_uuid(obs_scene_get_source(scene));
	eventData["sceneItemId"] = obs_sceneitem_get_id(sceneItem);
	eventData["sceneItemLocked"] = sceneItemLocked;
	eventHandler->BroadcastEvent(EventSubscription::SceneItems, "SceneItemLockStateChanged", eventData);
}

/**
 * A scene item has been selected in the Ui.
 *
 * @dataField sceneName        | String  | Name of the scene the item is in
 * @dataField sceneUuid        | String  | UUID of the scene the item is in
 * @dataField sceneItemId      | Number  | Numeric ID of the scene item
 *
 * @eventType SceneItemSelected
 * @eventSubscription SceneItems
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category scene items
 */
void EventHandler::HandleSceneItemSelected(void *param, calldata_t *data)
{
	auto eventHandler = static_cast<EventHandler *>(param);

	obs_scene_t *scene = GetCalldataPointer<obs_scene_t>(data, "scene");
	if (!scene)
		return;

	obs_sceneitem_t *sceneItem = GetCalldataPointer<obs_sceneitem_t>(data, "item");
	if (!sceneItem)
		return;

	json eventData;
	eventData["sceneName"] = obs_source_get_name(obs_scene_get_source(scene));
	eventData["sceneUuid"] = obs_source_get_uuid(obs_scene_get_source(scene));
	eventData["sceneItemId"] = obs_sceneitem_get_id(sceneItem);
	eventHandler->BroadcastEvent(EventSubscription::SceneItems, "SceneItemSelected", eventData);
}

/**
 * The transform/crop of a scene item has changed.
 *
 * @dataField sceneName          | String | The name of the scene the item is in
 * @dataField sceneUuid          | String | The UUID of the scene the item is in
 * @dataField sceneItemId        | Number | Numeric ID of the scene item
 * @dataField sceneItemTransform | Object | New transform/crop info of the scene item
 *
 * @eventType SceneItemTransformChanged
 * @eventSubscription SceneItemTransformChanged
 * @complexity 4
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category scene items
 */
void EventHandler::HandleSceneItemTransformChanged(void *param, calldata_t *data)
{
	auto eventHandler = static_cast<EventHandler *>(param);

	if (!eventHandler->_sceneItemTransformChangedRef.load())
		return;

	obs_scene_t *scene = GetCalldataPointer<obs_scene_t>(data, "scene");
	if (!scene)
		return;

	obs_sceneitem_t *sceneItem = GetCalldataPointer<obs_sceneitem_t>(data, "item");
	if (!sceneItem)
		return;

	json eventData;
	eventData["sceneName"] = obs_source_get_name(obs_scene_get_source(scene));
	eventData["sceneUuid"] = obs_source_get_uuid(obs_scene_get_source(scene));
	eventData["sceneItemId"] = obs_sceneitem_get_id(sceneItem);
	eventData["sceneItemTransform"] = Utils::Obs::ObjectHelper::GetSceneItemTransform(sceneItem);
	eventHandler->BroadcastEvent(EventSubscription::SceneItemTransformChanged, "SceneItemTransformChanged", eventData);
}
