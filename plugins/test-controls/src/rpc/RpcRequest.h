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

// forward declarations
class RpcResponse;

class RpcRequest
{
public:
	explicit RpcRequest(const QString& messageId, const QString& methodName, obs_data_t* params);
	
	const QString& messageId() const
	{
		return _messageId;
	}
		
	const QString& methodName() const
	{
		return _methodName;
	}

	const OBSData parameters() const
	{
		return OBSData(_parameters);
	}

	const RpcResponse success(obs_data_t* additionalFields = nullptr) const;
	const RpcResponse failed(const QString& errorMessage, obs_data_t* additionalFields = nullptr) const;

	const bool hasField(QString fieldName, obs_data_type expectedFieldType = OBS_DATA_NULL,
					obs_data_number_type expectedNumberType = OBS_DATA_NUM_INVALID) const;
	const bool hasBool(QString fieldName) const;
	const bool hasString(QString fieldName) const;
	const bool hasNumber(QString fieldName, obs_data_number_type expectedNumberType = OBS_DATA_NUM_INVALID) const;
	const bool hasInteger(QString fieldName) const;
	const bool hasDouble(QString fieldName) const;
	const bool hasArray(QString fieldName) const;
	const bool hasObject(QString fieldName) const;

private:
	const QString _messageId;
	const QString _methodName;
	OBSDataAutoRelease _parameters;
};
