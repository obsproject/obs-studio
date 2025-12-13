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
 * The current scene collection has begun changing.
 *
 * Note: We recommend using this event to trigger a pause of all polling requests, as performing any requests during a
 * scene collection change is considered undefined behavior and can cause crashes!
 *
 * @dataField sceneCollectionName | String | Name of the current scene collection
 *
 * @eventType CurrentSceneCollectionChanging
 * @eventSubscription Config
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category config
 * @api events
 */
void EventHandler::HandleCurrentSceneCollectionChanging()
{
	json eventData;
	eventData["sceneCollectionName"] = Utils::Obs::StringHelper::GetCurrentSceneCollection();
	BroadcastEvent(EventSubscription::Config, "CurrentSceneCollectionChanging", eventData);
}

/**
 * The current scene collection has changed.
 *
 * Note: If polling has been paused during `CurrentSceneCollectionChanging`, this is the que to restart polling.
 *
 * @dataField sceneCollectionName | String | Name of the new scene collection
 *
 * @eventType CurrentSceneCollectionChanged
 * @eventSubscription Config
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category config
 * @api events
 */
void EventHandler::HandleCurrentSceneCollectionChanged()
{
	json eventData;
	eventData["sceneCollectionName"] = Utils::Obs::StringHelper::GetCurrentSceneCollection();
	BroadcastEvent(EventSubscription::Config, "CurrentSceneCollectionChanged", eventData);
}

/**
 * The scene collection list has changed.
 *
 * @dataField sceneCollections | Array<String> | Updated list of scene collections
 *
 * @eventType SceneCollectionListChanged
 * @eventSubscription Config
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category config
 * @api events
 */
void EventHandler::HandleSceneCollectionListChanged()
{
	json eventData;
	eventData["sceneCollections"] = Utils::Obs::ArrayHelper::GetSceneCollectionList();
	BroadcastEvent(EventSubscription::Config, "SceneCollectionListChanged", eventData);
}

/**
 * The current profile has begun changing.
 *
 * @dataField profileName | String | Name of the current profile
 *
 * @eventType CurrentProfileChanging
 * @eventSubscription Config
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category config
 * @api events
 */
void EventHandler::HandleCurrentProfileChanging()
{
	json eventData;
	eventData["profileName"] = Utils::Obs::StringHelper::GetCurrentProfile();
	BroadcastEvent(EventSubscription::Config, "CurrentProfileChanging", eventData);
}

/**
 * The current profile has changed.
 *
 * @dataField profileName | String | Name of the new profile
 *
 * @eventType CurrentProfileChanged
 * @eventSubscription Config
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category config
 * @api events
 */
void EventHandler::HandleCurrentProfileChanged()
{
	json eventData;
	eventData["profileName"] = Utils::Obs::StringHelper::GetCurrentProfile();
	BroadcastEvent(EventSubscription::Config, "CurrentProfileChanged", eventData);
}

/**
 * The profile list has changed.
 *
 * @dataField profiles | Array<String> | Updated list of profiles
 *
 * @eventType ProfileListChanged
 * @eventSubscription Config
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category config
 * @api events
 */
void EventHandler::HandleProfileListChanged()
{
	json eventData;
	eventData["profiles"] = Utils::Obs::ArrayHelper::GetProfileList();
	BroadcastEvent(EventSubscription::Config, "ProfileListChanged", eventData);
}
