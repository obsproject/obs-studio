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
#include "asio-callbacks.h"
#include "asio-device.h"

/* ASIO callbacks are quite a pain because they don't store the 'this' pointer. In c++ we could do some template tricks
 * as done by JUCE. But in C, we're more restricted, hence the use of these macros. 
 * The number of callback sets is the number of supported devices. So up to 16. This could be reduced to a smaller
 * number.
 */
#define DEFINE_CALLBACK_SET(N) \
    void buffer_switch_##N(long index, long directProcess) {{ \
        if (current_asio_devices[N]) \
            asio_device_callback(current_asio_devices[N], index); \
    }} \
    ASIOTime* buffer_switch_time_info_##N(ASIOTime* params, long index, long directProcess) {{ \
        UNUSED_PARAMETER(params); \
        UNUSED_PARAMETER(index); \
        UNUSED_PARAMETER(directProcess); \
        return NULL; \
    }} \
    long asio_message_callback_##N(long selector, long value, void* message, double* opt) {{ \
        if (current_asio_devices[N]) \
            return asio_device_asio_message_callback(current_asio_devices[N], selector, value, message, opt); \
        return 0; \
    }} \
    void sample_rate_changed_callback_##N(ASIOSampleRate rate) {{ \
        UNUSED_PARAMETER(rate); \
        if (current_asio_devices[N]) \
            asio_device_reset_request(current_asio_devices[N]); \
    }}

DEFINE_CALLBACK_SET(0)
DEFINE_CALLBACK_SET(1)
DEFINE_CALLBACK_SET(2)
DEFINE_CALLBACK_SET(3)
DEFINE_CALLBACK_SET(4)
DEFINE_CALLBACK_SET(5)
DEFINE_CALLBACK_SET(6)
DEFINE_CALLBACK_SET(7)
DEFINE_CALLBACK_SET(8)
DEFINE_CALLBACK_SET(9)
DEFINE_CALLBACK_SET(10)
DEFINE_CALLBACK_SET(11)
DEFINE_CALLBACK_SET(12)
DEFINE_CALLBACK_SET(13)
DEFINE_CALLBACK_SET(14)
DEFINE_CALLBACK_SET(15)

const struct asio_callback_set callback_sets[MAX_NUM_ASIO_DEVICES] = {
	{buffer_switch_0, buffer_switch_time_info_0, asio_message_callback_0, sample_rate_changed_callback_0},
	{buffer_switch_1, buffer_switch_time_info_1, asio_message_callback_1, sample_rate_changed_callback_1},
	{buffer_switch_2, buffer_switch_time_info_2, asio_message_callback_2, sample_rate_changed_callback_2},
	{buffer_switch_3, buffer_switch_time_info_3, asio_message_callback_3, sample_rate_changed_callback_3},
	{buffer_switch_4, buffer_switch_time_info_4, asio_message_callback_4, sample_rate_changed_callback_4},
	{buffer_switch_5, buffer_switch_time_info_5, asio_message_callback_5, sample_rate_changed_callback_5},
	{buffer_switch_6, buffer_switch_time_info_6, asio_message_callback_6, sample_rate_changed_callback_6},
	{buffer_switch_7, buffer_switch_time_info_7, asio_message_callback_7, sample_rate_changed_callback_7},
	{buffer_switch_8, buffer_switch_time_info_8, asio_message_callback_8, sample_rate_changed_callback_8},
	{buffer_switch_9, buffer_switch_time_info_9, asio_message_callback_9, sample_rate_changed_callback_9},
	{buffer_switch_10, buffer_switch_time_info_10, asio_message_callback_10, sample_rate_changed_callback_10},
	{buffer_switch_11, buffer_switch_time_info_11, asio_message_callback_11, sample_rate_changed_callback_11},
	{buffer_switch_12, buffer_switch_time_info_12, asio_message_callback_12, sample_rate_changed_callback_12},
	{buffer_switch_13, buffer_switch_time_info_13, asio_message_callback_13, sample_rate_changed_callback_13},
	{buffer_switch_14, buffer_switch_time_info_14, asio_message_callback_14, sample_rate_changed_callback_14},
	{buffer_switch_15, buffer_switch_time_info_15, asio_message_callback_15, sample_rate_changed_callback_15},
};
