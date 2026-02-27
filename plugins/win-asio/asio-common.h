/******************************************************************************
    Copyright (C) 2022-2025 pkv <pkv@obsproject.com>

    This file is part of win-asio.
    It uses the Steinberg ASIO SDK, which is licensed under the GNU GPL v3.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
#pragma once
#include "iasiodrv.h"
#include <obs-module.h>

#define MAX_DEVICE_CHANNELS 32
#define MAX_CH_NAME_LENGTH  32
#define MAX_NUM_ASIO_DEVICES 16

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#define ASIO_LOG(level, format, ...)  \
	blog(level, "[asio_device '%s']: " format, (dev && dev->device_name) ? dev->device_name : "(null)", ##__VA_ARGS__)

#define ASIO_LOG2(level, format, ...) \
	blog(level, "[asio_device_list]: " format, ##__VA_ARGS__)

#define debug(format, ...) ASIO_LOG(LOG_DEBUG, format, ##__VA_ARGS__)
#define warn(format, ...)  ASIO_LOG(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...)  ASIO_LOG(LOG_INFO, format, ##__VA_ARGS__)
#define info2(format, ...) ASIO_LOG2(LOG_INFO, format, ##__VA_ARGS__)
#define error(format, ...) ASIO_LOG(LOG_ERROR, format, ##__VA_ARGS__)
#define error2(format, ...) ASIO_LOG2(LOG_ERROR, format, ##__VA_ARGS__)
