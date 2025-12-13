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

#include <memory>
#include <obs.hpp>
#include <util/platform.h>

#include "utils/Obs.h"
#include "plugin-macros.generated.h"

#define CURRENT_RPC_VERSION 1

struct Config;
typedef std::shared_ptr<Config> ConfigPtr;

class EventHandler;
typedef std::shared_ptr<EventHandler> EventHandlerPtr;

class WebSocketApi;
typedef std::shared_ptr<WebSocketApi> WebSocketApiPtr;

class WebSocketServer;
typedef std::shared_ptr<WebSocketServer> WebSocketServerPtr;

os_cpu_usage_info_t *GetCpuUsageInfo();

ConfigPtr GetConfig();

EventHandlerPtr GetEventHandler();

WebSocketApiPtr GetWebSocketApi();

WebSocketServerPtr GetWebSocketServer();

bool IsDebugEnabled();
