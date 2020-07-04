/*
obs-websocket
Copyright (C) 2016-2019	Stéphane Lepin <stephane.lepin@gmail.com>

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

#include "RpcResponse.h"
#include "RpcRequest.h"

RpcResponse::RpcResponse(
	Status status, const QString& messageId,
	const QString& methodName, obs_data_t* additionalFields
) :
	_status(status),
	_messageId(messageId),
	_methodName(methodName),
	_additionalFields(nullptr)
{
	if (additionalFields) {
		_additionalFields = obs_data_create();
		obs_data_apply(_additionalFields, additionalFields);
	}
}

const RpcResponse RpcResponse::ok(const RpcRequest& request, obs_data_t* additionalFields)
{
	RpcResponse response(Status::Ok, request.messageId(), request.methodName(), additionalFields);
	return response;
} 

const RpcResponse RpcResponse::fail(const RpcRequest& request, const QString& errorMessage, obs_data_t* additionalFields)
{
	RpcResponse response(Status::Error, request.messageId(), request.methodName(), additionalFields);
	response._errorMessage = errorMessage;
	return response;
}
