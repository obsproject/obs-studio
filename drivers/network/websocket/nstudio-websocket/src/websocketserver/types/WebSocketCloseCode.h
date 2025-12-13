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

namespace WebSocketCloseCode {
	enum WebSocketCloseCode {
		/**
		* For internal use only to tell the request handler not to perform any close action.
		*
		* @enumIdentifier DontClose
		* @enumValue 0
		* @enumType WebSocketCloseCode
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		DontClose = 0,
		/**
		* Unknown reason, should never be used.
		*
		* @enumIdentifier UnknownReason
		* @enumValue 4000
		* @enumType WebSocketCloseCode
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		UnknownReason = 4000,
		/**
		* The server was unable to decode the incoming websocket message.
		*
		* @enumIdentifier MessageDecodeError
		* @enumValue 4002
		* @enumType WebSocketCloseCode
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		MessageDecodeError = 4002,
		/**
		* A data field is required but missing from the payload.
		*
		* @enumIdentifier MissingDataField
		* @enumValue 4003
		* @enumType WebSocketCloseCode
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		MissingDataField = 4003,
		/**
		* A data field's value type is invalid.
		*
		* @enumIdentifier InvalidDataFieldType
		* @enumValue 4004
		* @enumType WebSocketCloseCode
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		InvalidDataFieldType = 4004,
		/**
		* A data field's value is invalid.
		*
		* @enumIdentifier InvalidDataFieldValue
		* @enumValue 4005
		* @enumType WebSocketCloseCode
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		InvalidDataFieldValue = 4005,
		/**
		* The specified `op` was invalid or missing.
		*
		* @enumIdentifier UnknownOpCode
		* @enumValue 4006
		* @enumType WebSocketCloseCode
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		UnknownOpCode = 4006,
		/**
		* The client sent a websocket message without first sending `Identify` message.
		*
		* @enumIdentifier NotIdentified
		* @enumValue 4007
		* @enumType WebSocketCloseCode
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		NotIdentified = 4007,
		/**
		* The client sent an `Identify` message while already identified.
		*
		* Note: Once a client has identified, only `Reidentify` may be used to change session parameters.
		*
		* @enumIdentifier AlreadyIdentified
		* @enumValue 4008
		* @enumType WebSocketCloseCode
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		AlreadyIdentified = 4008,
		/**
		* The authentication attempt (via `Identify`) failed.
		*
		* @enumIdentifier AuthenticationFailed
		* @enumValue 4009
		* @enumType WebSocketCloseCode
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		AuthenticationFailed = 4009,
		/**
		* The server detected the usage of an old version of the obs-websocket RPC protocol.
		*
		* @enumIdentifier UnsupportedRpcVersion
		* @enumValue 4010
		* @enumType WebSocketCloseCode
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		UnsupportedRpcVersion = 4010,
		/**
		* The websocket session has been invalidated by the obs-websocket server.
		*
		* Note: This is the code used by the `Kick` button in the UI Session List. If you receive this code, you must not automatically reconnect.
		*
		* @enumIdentifier SessionInvalidated
		* @enumValue 4011
		* @enumType WebSocketCloseCode
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		SessionInvalidated = 4011,
		/**
		* A requested feature is not supported due to hardware/software limitations.
		*
		* @enumIdentifier UnsupportedFeature
		* @enumValue 4012
		* @enumType WebSocketCloseCode
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		UnsupportedFeature = 4012,
	};
}
