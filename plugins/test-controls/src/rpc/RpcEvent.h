/*
obs-websocket
Copyright (C) 2016-2020	Stéphane Lepin <stephane.lepin@gmail.com>

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

#include <optional>
#include <obs-data.h>
#include <QtCore/QString>

#include "../obs-midi.h"

class RpcEvent {
public:
	explicit RpcEvent(const QString &updateType,
			  std::optional<uint64_t> streamTime,
			  std::optional<uint64_t> recordingTime,
			  obs_data_t *additionalFields = nullptr);

	const QString &updateType() const { return _updateType; }

	const std::optional<uint64_t> streamTime() const { return _streamTime; }

	const std::optional<uint64_t> recordingTime() const
	{
		return _recordingTime;
	}

	const OBSData additionalFields() const
	{
		return OBSData(_additionalFields);
	}

private:
	QString _updateType;
	std::optional<uint64_t> _streamTime;
	std::optional<uint64_t> _recordingTime;
	OBSDataAutoRelease _additionalFields;
};
