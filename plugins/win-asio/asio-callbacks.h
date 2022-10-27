/******************************************************************************
    Copyright (C) 2022-2026 pkv <pkv@obsproject.com>

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
#include "asio-common.h"

struct asio_callback_set {
	void (*buffer_switch)(long index, long directProcess);
	ASIOTime *(*buffer_switch_time_info)(ASIOTime *params, long index, long directProcess);
	long (*asio_message)(long selector, long value, void *opt, double *message);
	void (*sample_rate_changed)(ASIOSampleRate);
};

extern const struct asio_callback_set callback_sets[MAX_NUM_ASIO_DEVICES];
