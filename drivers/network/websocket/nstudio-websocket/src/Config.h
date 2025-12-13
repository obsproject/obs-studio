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

#include <atomic>
#include <QString>
#include <util/config-file.h>

#include "utils/Json.h"
#include "plugin-macros.generated.h"

struct Config {
	void Load(json config = nullptr);
	void Save();

	std::atomic<bool> PortOverridden = false;
	std::atomic<bool> PasswordOverridden = false;

	std::atomic<bool> FirstLoad = true;
	std::atomic<bool> ServerEnabled = false;
	std::atomic<uint16_t> ServerPort = 4455;
	std::atomic<bool> Ipv4Only = false;
	std::atomic<bool> DebugEnabled = false;
	std::atomic<bool> AlertsEnabled = false;
	std::atomic<bool> AuthRequired = true;
	std::string ServerPassword;
};

json MigrateGlobalConfigData();
bool MigratePersistentData();
