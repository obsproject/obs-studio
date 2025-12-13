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

namespace WebSocketOpCode {
	enum WebSocketOpCode : uint8_t {
		/**
		* The initial message sent by obs-websocket to newly connected clients.
		*
		* @enumIdentifier Hello
		* @enumValue 0
		* @enumType WebSocketOpCode
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		Hello = 0,
		/**
		* The message sent by a newly connected client to obs-websocket in response to a `Hello`.
		*
		* @enumIdentifier Identify
		* @enumValue 1
		* @enumType WebSocketOpCode
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		Identify = 1,
		/**
		* The response sent by obs-websocket to a client after it has successfully identified with obs-websocket.
		*
		* @enumIdentifier Identified
		* @enumValue 2
		* @enumType WebSocketOpCode
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		Identified = 2,
		/**
		* The message sent by an already-identified client to update identification parameters.
		*
		* @enumIdentifier Reidentify
		* @enumValue 3
		* @enumType WebSocketOpCode
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		Reidentify = 3,
		/**
		* The message sent by obs-websocket containing an event payload.
		*
		* @enumIdentifier Event
		* @enumValue 5
		* @enumType WebSocketOpCode
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		Event = 5,
		/**
		* The message sent by a client to obs-websocket to perform a request.
		*
		* @enumIdentifier Request
		* @enumValue 6
		* @enumType WebSocketOpCode
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		Request = 6,
		/**
		* The message sent by obs-websocket in response to a particular request from a client.
		*
		* @enumIdentifier RequestResponse
		* @enumValue 7
		* @enumType WebSocketOpCode
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		RequestResponse = 7,
		/**
		* The message sent by a client to obs-websocket to perform a batch of requests.
		*
		* @enumIdentifier RequestBatch
		* @enumValue 8
		* @enumType WebSocketOpCode
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		RequestBatch = 8,
		/**
		* The message sent by obs-websocket in response to a particular batch of requests from a client.
		*
		* @enumIdentifier RequestBatchResponse
		* @enumValue 9
		* @enumType WebSocketOpCode
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		RequestBatchResponse = 9,
	};

	inline bool IsValid(uint8_t opCode)
	{
		return opCode >= Hello && opCode <= RequestBatchResponse;
	}
}
