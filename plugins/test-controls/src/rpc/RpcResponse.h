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

#pragma once

#include <obs-data.h>
#include <QtCore/QString>
#include "../obs-websocket.h"

class RpcRequest;

class RpcResponse
{
public:
	enum Status { Unknown, Ok, Error };

	static RpcResponse ofRequest(const RpcRequest& request);
	static const RpcResponse ok(const RpcRequest& request, obs_data_t* additionalFields = nullptr);
	static const RpcResponse fail(
		const RpcRequest& request, const QString& errorMessage,
		obs_data_t* additionalFields = nullptr
	);

	Status status() {
		return _status;
	}

	const QString& messageId() const {
		return _messageId;
	}
	
	const QString& methodName() const {
		return _methodName;
	}

	const QString& errorMessage() const {
		return _errorMessage;
	}
	
	const OBSData additionalFields() const {
		return OBSData(_additionalFields);
	}

private:
	explicit RpcResponse(
		Status status,
		const QString& messageId, const QString& methodName,
		obs_data_t* additionalFields = nullptr
	);
	const Status _status;
	const QString _messageId;
	const QString _methodName;
	QString _errorMessage;
	OBSDataAutoRelease _additionalFields;
};
