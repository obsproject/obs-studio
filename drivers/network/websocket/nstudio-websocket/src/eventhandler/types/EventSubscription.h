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

#pragma once

namespace EventSubscription {
	enum EventSubscription {
		/**
		* Subcription value used to disable all events.
		*
		* @enumIdentifier None
		* @enumValue 0
		* @enumType EventSubscription
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		None = 0,
		/**
		* Subscription value to receive events in the `General` category.
		*
		* @enumIdentifier General
		* @enumValue (1 << 0)
		* @enumType EventSubscription
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		General = (1 << 0),
		/**
		* Subscription value to receive events in the `Config` category.
		*
		* @enumIdentifier Config
		* @enumValue (1 << 1)
		* @enumType EventSubscription
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		Config = (1 << 1),
		/**
		* Subscription value to receive events in the `Scenes` category.
		*
		* @enumIdentifier Scenes
		* @enumValue (1 << 2)
		* @enumType EventSubscription
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		Scenes = (1 << 2),
		/**
		* Subscription value to receive events in the `Inputs` category.
		*
		* @enumIdentifier Inputs
		* @enumValue (1 << 3)
		* @enumType EventSubscription
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		Inputs = (1 << 3),
		/**
		* Subscription value to receive events in the `Transitions` category.
		*
		* @enumIdentifier Transitions
		* @enumValue (1 << 4)
		* @enumType EventSubscription
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		Transitions = (1 << 4),
		/**
		* Subscription value to receive events in the `Filters` category.
		*
		* @enumIdentifier Filters
		* @enumValue (1 << 5)
		* @enumType EventSubscription
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		Filters = (1 << 5),
		/**
		* Subscription value to receive events in the `Outputs` category.
		*
		* @enumIdentifier Outputs
		* @enumValue (1 << 6)
		* @enumType EventSubscription
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		Outputs = (1 << 6),
		/**
		* Subscription value to receive events in the `SceneItems` category.
		*
		* @enumIdentifier SceneItems
		* @enumValue (1 << 7)
		* @enumType EventSubscription
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		SceneItems = (1 << 7),
		/**
		* Subscription value to receive events in the `MediaInputs` category.
		*
		* @enumIdentifier MediaInputs
		* @enumValue (1 << 8)
		* @enumType EventSubscription
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		MediaInputs = (1 << 8),
		/**
		* Subscription value to receive the `VendorEvent` event.
		*
		* @enumIdentifier Vendors
		* @enumValue (1 << 9)
		* @enumType EventSubscription
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		Vendors = (1 << 9),
		/**
		* Subscription value to receive events in the `Ui` category.
		*
		* @enumIdentifier Ui
		* @enumValue (1 << 10)
		* @enumType EventSubscription
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		Ui = (1 << 10),
		/**
		* Helper to receive all non-high-volume events.
		*
		* @enumIdentifier All
		* @enumValue (General | Config | Scenes | Inputs | Transitions | Filters | Outputs | SceneItems | MediaInputs | Vendors | Ui)
		* @enumType EventSubscription
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		All = (General | Config | Scenes | Inputs | Transitions | Filters | Outputs | SceneItems | MediaInputs | Vendors |
		       Ui),
		/**
		* Subscription value to receive the `InputVolumeMeters` high-volume event.
		*
		* @enumIdentifier InputVolumeMeters
		* @enumValue (1 << 16)
		* @enumType EventSubscription
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		InputVolumeMeters = (1 << 16),
		/**
		* Subscription value to receive the `InputActiveStateChanged` high-volume event.
		*
		* @enumIdentifier InputActiveStateChanged
		* @enumValue (1 << 17)
		* @enumType EventSubscription
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		InputActiveStateChanged = (1 << 17),
		/**
		* Subscription value to receive the `InputShowStateChanged` high-volume event.
		*
		* @enumIdentifier InputShowStateChanged
		* @enumValue (1 << 18)
		* @enumType EventSubscription
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		InputShowStateChanged = (1 << 18),
		/**
		* Subscription value to receive the `SceneItemTransformChanged` high-volume event.
		*
		* @enumIdentifier SceneItemTransformChanged
		* @enumValue (1 << 19)
		* @enumType EventSubscription
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		SceneItemTransformChanged = (1 << 19),
	};
}
