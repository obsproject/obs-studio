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
 * A new scene has been created.
 *
 * @dataField sceneName | String  | Name of the new scene
 * @dataField sceneUuid | String  | UUID of the new scene
 * @dataField isGroup   | Boolean | Whether the new scene is a group
 *
 * @eventType SceneCreated
 * @eventSubscription Scenes
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category scenes
 */
void EventHandler::HandleSceneCreated(obs_source_t *source)
{
	json eventData;
	eventData["sceneName"] = obs_source_get_name(source);
	eventData["sceneUuid"] = obs_source_get_uuid(source);
	eventData["isGroup"] = obs_source_is_group(source);
	BroadcastEvent(EventSubscription::Scenes, "SceneCreated", eventData);
}

/**
 * A scene has been removed.
 *
 * @dataField sceneName | String  | Name of the removed scene
 * @dataField sceneUuid | String  | UUID of the removed scene
 * @dataField isGroup   | Boolean | Whether the scene was a group
 *
 * @eventType SceneRemoved
 * @eventSubscription Scenes
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category scenes
 */
void EventHandler::HandleSceneRemoved(obs_source_t *source)
{
	json eventData;
	eventData["sceneName"] = obs_source_get_name(source);
	eventData["sceneUuid"] = obs_source_get_uuid(source);
	eventData["isGroup"] = obs_source_is_group(source);
	BroadcastEvent(EventSubscription::Scenes, "SceneRemoved", eventData);
}

/**
 * The name of a scene has changed.
 *
 * @dataField sceneUuid    | String | UUID of the scene
 * @dataField oldSceneName | String | Old name of the scene
 * @dataField sceneName    | String | New name of the scene
 *
 * @eventType SceneNameChanged
 * @eventSubscription Scenes
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category scenes
 */
void EventHandler::HandleSceneNameChanged(obs_source_t *source, std::string oldSceneName, std::string sceneName)
{
	json eventData;
	eventData["sceneUuid"] = obs_source_get_uuid(source);
	eventData["oldSceneName"] = oldSceneName;
	eventData["sceneName"] = sceneName;
	BroadcastEvent(EventSubscription::Scenes, "SceneNameChanged", eventData);
}

/**
 * The current program scene has changed.
 *
 * @dataField sceneName | String | Name of the scene that was switched to
 * @dataField sceneUuid | String | UUID of the scene that was switched to
 *
 * @eventType CurrentProgramSceneChanged
 * @eventSubscription Scenes
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category scenes
 */
void EventHandler::HandleCurrentProgramSceneChanged()
{
	OBSSourceAutoRelease currentScene = obs_frontend_get_current_scene();

	if (!currentScene)
		return;

	json eventData;
	eventData["sceneName"] = obs_source_get_name(currentScene);
	eventData["sceneUuid"] = obs_source_get_uuid(currentScene);
	BroadcastEvent(EventSubscription::Scenes, "CurrentProgramSceneChanged", eventData);
}

/**
 * The current preview scene has changed.
 *
 * @dataField sceneName | String | Name of the scene that was switched to
 * @dataField sceneUuid | String | UUID of the scene that was switched to
 *
 * @eventType CurrentPreviewSceneChanged
 * @eventSubscription Scenes
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category scenes
 */
void EventHandler::HandleCurrentPreviewSceneChanged()
{
	OBSSourceAutoRelease currentPreviewScene = obs_frontend_get_current_preview_scene();

	// This event may be called when OBS is not in studio mode, however retreiving the source while not in studio mode will return null.
	if (!currentPreviewScene)
		return;

	json eventData;
	eventData["sceneName"] = obs_source_get_name(currentPreviewScene);
	eventData["sceneUuid"] = obs_source_get_uuid(currentPreviewScene);
	BroadcastEvent(EventSubscription::Scenes, "CurrentPreviewSceneChanged", eventData);
}

/**
 * The list of scenes has changed.
 *
 * TODO: Make OBS fire this event when scenes are reordered.
 *
 * @dataField scenes | Array<Object> | Updated array of scenes
 *
 * @eventType SceneListChanged
 * @eventSubscription Scenes
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category scenes
 */
void EventHandler::HandleSceneListChanged()
{
	json eventData;
	eventData["scenes"] = Utils::Obs::ArrayHelper::GetSceneList();
	BroadcastEvent(EventSubscription::Scenes, "SceneListChanged", eventData);
}
