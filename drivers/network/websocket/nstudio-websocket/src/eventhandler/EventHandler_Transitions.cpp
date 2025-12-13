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
 * The current scene transition has changed.
 *
 * @dataField transitionName | String | Name of the new transition
 * @dataField transitionUuid | String | UUID of the new transition
 *
 * @eventType CurrentSceneTransitionChanged
 * @eventSubscription Transitions
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category transitions
 */
void EventHandler::HandleCurrentSceneTransitionChanged()
{
	OBSSourceAutoRelease transition = obs_frontend_get_current_transition();

	json eventData;
	eventData["transitionName"] = obs_source_get_name(transition);
	eventData["transitionUuid"] = obs_source_get_uuid(transition);
	BroadcastEvent(EventSubscription::Transitions, "CurrentSceneTransitionChanged", eventData);
}

/**
 * The current scene transition duration has changed.
 *
 * @dataField transitionDuration | Number | Transition duration in milliseconds
 *
 * @eventType CurrentSceneTransitionDurationChanged
 * @eventSubscription Transitions
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category transitions
 */
void EventHandler::HandleCurrentSceneTransitionDurationChanged()
{
	json eventData;
	eventData["transitionDuration"] = obs_frontend_get_transition_duration();
	BroadcastEvent(EventSubscription::Transitions, "CurrentSceneTransitionDurationChanged", eventData);
}

/**
 * A scene transition has started.
 *
 * @dataField transitionName | String | Scene transition name
 * @dataField transitionUuid | String | Scene transition UUID
 *
 * @eventType SceneTransitionStarted
 * @eventSubscription Transitions
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category transitions
 */
void EventHandler::HandleSceneTransitionStarted(void *param, calldata_t *data)
{
	auto eventHandler = static_cast<EventHandler *>(param);

	obs_source_t *source = GetCalldataPointer<obs_source_t>(data, "source");
	if (!source)
		return;

	json eventData;
	eventData["transitionName"] = obs_source_get_name(source);
	eventData["transitionUuid"] = obs_source_get_uuid(source);
	eventHandler->BroadcastEvent(EventSubscription::Transitions, "SceneTransitionStarted", eventData);
}

/**
 * A scene transition has completed fully.
 *
 * Note: Does not appear to trigger when the transition is interrupted by the user.
 *
 * @dataField transitionName | String | Scene transition name
 * @dataField transitionUuid | String | Scene transition UUID
 *
 * @eventType SceneTransitionEnded
 * @eventSubscription Transitions
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category transitions
 */
void EventHandler::HandleSceneTransitionEnded(void *param, calldata_t *data)
{
	auto eventHandler = static_cast<EventHandler *>(param);

	obs_source_t *source = GetCalldataPointer<obs_source_t>(data, "source");
	if (!source)
		return;

	json eventData;
	eventData["transitionName"] = obs_source_get_name(source);
	eventData["transitionUuid"] = obs_source_get_uuid(source);
	eventHandler->BroadcastEvent(EventSubscription::Transitions, "SceneTransitionEnded", eventData);
}

/**
 * A scene transition's video has completed fully.
 *
 * Useful for stinger transitions to tell when the video *actually* ends.
 * `SceneTransitionEnded` only signifies the cut point, not the completion of transition playback.
 *
 * Note: Appears to be called by every transition, regardless of relevance.
 *
 * @dataField transitionName | String | Scene transition name
 * @dataField transitionUuid | String | Scene transition UUID
 *
 * @eventType SceneTransitionVideoEnded
 * @eventSubscription Transitions
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category transitions
 */
void EventHandler::HandleSceneTransitionVideoEnded(void *param, calldata_t *data)
{
	auto eventHandler = static_cast<EventHandler *>(param);

	obs_source_t *source = GetCalldataPointer<obs_source_t>(data, "source");
	if (!source)
		return;

	json eventData;
	eventData["transitionName"] = obs_source_get_name(source);
	eventData["transitionUuid"] = obs_source_get_uuid(source);
	eventHandler->BroadcastEvent(EventSubscription::Transitions, "SceneTransitionVideoEnded", eventData);
}
