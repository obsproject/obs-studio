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

namespace RequestStatus {
	enum RequestStatus {
		/**
		* Unknown status, should never be used.
		*
		* @enumIdentifier Unknown
		* @enumValue 0
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		Unknown = 0,

		/**
		* For internal use to signify a successful field check.
		*
		* @enumIdentifier NoError
		* @enumValue 10
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		NoError = 10,

		/**
		* The request has succeeded.
		*
		* @enumIdentifier Success
		* @enumValue 100
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		Success = 100,

		/**
		* The `requestType` field is missing from the request data.
		*
		* @enumIdentifier MissingRequestType
		* @enumValue 203
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		MissingRequestType = 203,
		/**
		* The request type is invalid or does not exist.
		*
		* @enumIdentifier UnknownRequestType
		* @enumValue 204
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		UnknownRequestType = 204,
		/**
		* Generic error code.
		*
		* Note: A comment is required to be provided by obs-websocket.
		*
		* @enumIdentifier GenericError
		* @enumValue 205
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		GenericError = 205,
		/**
		* The request batch execution type is not supported.
		*
		* @enumIdentifier UnsupportedRequestBatchExecutionType
		* @enumValue 206
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		UnsupportedRequestBatchExecutionType = 206,
		/**
		* The server is not ready to handle the request.
		*
		* Note: This usually occurs during OBS scene collection change or exit. Requests may be tried again after a delay if this code is given.
		*
		* @enumIdentifier NotReady
		* @enumValue 207
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.3.0
		* @api enums
		*/
		NotReady = 207,

		/**
		* A required request field is missing.
		*
		* @enumIdentifier MissingRequestField
		* @enumValue 300
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		MissingRequestField = 300,
		/**
		* The request does not have a valid requestData object.
		*
		* @enumIdentifier MissingRequestData
		* @enumValue 301
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		MissingRequestData = 301,

		/**
		* Generic invalid request field message.
		*
		* Note: A comment is required to be provided by obs-websocket.
		*
		* @enumIdentifier InvalidRequestField
		* @enumValue 400
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		InvalidRequestField = 400,
		/**
		* A request field has the wrong data type.
		*
		* @enumIdentifier InvalidRequestFieldType
		* @enumValue 401
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		InvalidRequestFieldType = 401,
		/**
		* A request field (number) is outside of the allowed range.
		*
		* @enumIdentifier RequestFieldOutOfRange
		* @enumValue 402
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		RequestFieldOutOfRange = 402,
		/**
		* A request field (string or array) is empty and cannot be.
		*
		* @enumIdentifier RequestFieldEmpty
		* @enumValue 403
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		RequestFieldEmpty = 403,
		/**
		* There are too many request fields (eg. a request takes two optionals, where only one is allowed at a time).
		*
		* @enumIdentifier TooManyRequestFields
		* @enumValue 404
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		TooManyRequestFields = 404,

		/**
		* An output is running and cannot be in order to perform the request.
		*
		* @enumIdentifier OutputRunning
		* @enumValue 500
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		OutputRunning = 500,
		/**
		* An output is not running and should be.
		*
		* @enumIdentifier OutputNotRunning
		* @enumValue 501
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		OutputNotRunning = 501,
		/**
		* An output is paused and should not be.
		*
		* @enumIdentifier OutputPaused
		* @enumValue 502
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		OutputPaused = 502,
		/**
		* An output is not paused and should be.
		*
		* @enumIdentifier OutputNotPaused
		* @enumValue 503
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		OutputNotPaused = 503,
		/**
		* An output is disabled and should not be.
		*
		* @enumIdentifier OutputDisabled
		* @enumValue 504
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		OutputDisabled = 504,
		/**
		* Studio mode is active and cannot be.
		*
		* @enumIdentifier StudioModeActive
		* @enumValue 505
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		StudioModeActive = 505,
		/**
		* Studio mode is not active and should be.
		*
		* @enumIdentifier StudioModeNotActive
		* @enumValue 506
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		StudioModeNotActive = 506,

		/**
		* The resource was not found.
		*
		* Note: Resources are any kind of object in obs-websocket, like inputs, profiles, outputs, etc.
		*
		* @enumIdentifier ResourceNotFound
		* @enumValue 600
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		ResourceNotFound = 600,
		/**
		* The resource already exists.
		*
		* @enumIdentifier ResourceAlreadyExists
		* @enumValue 601
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		ResourceAlreadyExists = 601,
		/**
		* The type of resource found is invalid.
		*
		* @enumIdentifier InvalidResourceType
		* @enumValue 602
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		InvalidResourceType = 602,
		/**
		* There are not enough instances of the resource in order to perform the request.
		*
		* @enumIdentifier NotEnoughResources
		* @enumValue 603
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		NotEnoughResources = 603,
		/**
		* The state of the resource is invalid. For example, if the resource is blocked from being accessed.
		*
		* @enumIdentifier InvalidResourceState
		* @enumValue 604
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		InvalidResourceState = 604,
		/**
		* The specified input (obs_source_t-OBS_SOURCE_TYPE_INPUT) had the wrong kind.
		*
		* @enumIdentifier InvalidInputKind
		* @enumValue 605
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		InvalidInputKind = 605,
		/**
		* The resource does not support being configured.
		*
		* This is particularly relevant to transitions, where they do not always have changeable settings.
		*
		* @enumIdentifier ResourceNotConfigurable
		* @enumValue 606
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		ResourceNotConfigurable = 606,
		/**
		* The specified filter (obs_source_t-OBS_SOURCE_TYPE_FILTER) had the wrong kind.
		*
		* @enumIdentifier InvalidFilterKind
		* @enumValue 607
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		InvalidFilterKind = 607,

		/**
		* Creating the resource failed.
		*
		* @enumIdentifier ResourceCreationFailed
		* @enumValue 700
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		ResourceCreationFailed = 700,
		/**
		* Performing an action on the resource failed.
		*
		* @enumIdentifier ResourceActionFailed
		* @enumValue 701
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		ResourceActionFailed = 701,
		/**
		* Processing the request failed unexpectedly.
		*
		* Note: A comment is required to be provided by obs-websocket.
		*
		* @enumIdentifier RequestProcessingFailed
		* @enumValue 702
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		RequestProcessingFailed = 702,
		/**
		* The combination of request fields cannot be used to perform an action.
		*
		* @enumIdentifier CannotAct
		* @enumValue 703
		* @enumType RequestStatus
		* @rpcVersion -1
		* @initialVersion 5.0.0
		* @api enums
		*/
		CannotAct = 703,
	};
}
