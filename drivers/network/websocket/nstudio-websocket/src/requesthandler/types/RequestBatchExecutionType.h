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

#include <stdint.h>

namespace RequestBatchExecutionType {
	enum RequestBatchExecutionType : int8_t {
		/**
		* Not a request batch.
		*
		* @enumIdentifier None
		* @enumValue -1
		* @enumType RequestBatchExecutionType
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		None = -1,
		/**
		* A request batch which processes all requests serially, as fast as possible.
		*
		* Note: To introduce artificial delay, use the `Sleep` request and the `sleepMillis` request field.
		*
		* @enumIdentifier SerialRealtime
		* @enumValue 0
		* @enumType RequestBatchExecutionType
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		SerialRealtime = 0,
		/**
		* A request batch type which processes all requests serially, in sync with the graphics thread. Designed to
		* provide high accuracy for animations.
		*
		* Note: To introduce artificial delay, use the `Sleep` request and the `sleepFrames` request field.
		*
		* @enumIdentifier SerialFrame
		* @enumValue 1
		* @enumType RequestBatchExecutionType
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		SerialFrame = 1,
		/**
		* A request batch type which processes all requests using all available threads in the thread pool.
		*
		* Note: This is mainly experimental, and only really shows its colors during requests which require lots of
		* active processing, like `GetSourceScreenshot`.
		*
		* @enumIdentifier Parallel
		* @enumValue 2
		* @enumType RequestBatchExecutionType
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		Parallel = 2,
	};

	inline bool IsValid(int8_t executionType)
	{
		return executionType >= None && executionType <= Parallel;
	}
}
