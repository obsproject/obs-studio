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
 * Studio mode has been enabled or disabled.
 *
 * @dataField studioModeEnabled | Boolean | True == Enabled, False == Disabled
 *
 * @eventType StudioModeStateChanged
 * @eventSubscription Ui
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category ui
 * @api events
 */
void EventHandler::HandleStudioModeStateChanged(bool enabled)
{
	json eventData;
	eventData["studioModeEnabled"] = enabled;
	BroadcastEvent(EventSubscription::Ui, "StudioModeStateChanged", eventData);
}

/**
 * A screenshot has been saved.
 *
 * Note: Triggered for the screenshot feature available in `Settings -> Hotkeys -> Screenshot Output` ONLY.
 * Applications using `Get/SaveSourceScreenshot` should implement a `CustomEvent` if this kind of inter-client
 * communication is desired.
 *
 * @dataField savedScreenshotPath | String | Path of the saved image file
 *
 * @eventType ScreenshotSaved
 * @eventSubscription Ui
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.1.0
 * @api events
 * @category ui
 */
void EventHandler::HandleScreenshotSaved()
{
	json eventData;
	eventData["savedScreenshotPath"] = Utils::Obs::StringHelper::GetLastScreenshotFileName();
	BroadcastEvent(EventSubscription::Ui, "ScreenshotSaved", eventData);
}
