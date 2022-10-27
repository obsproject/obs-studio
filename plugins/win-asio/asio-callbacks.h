/* Copyright (c) 2022-2025 pkv <pkv@obsproject.com>
 * This file is part of win-asio.
 *
 * This file uses the Steinberg ASIO SDK, which is licensed under the GNU GPL v3.
 *
 * This file and all modifications by pkv <pkv@obsproject.com> are licensed under
 * the GNU General Public License, version 3 or later, to comply with the SDK license.
 */
#pragma once

#include "iasiodrv.h"

#define MAX_NUM_ASIO_DEVICES 16

struct asio_callback_set {
	void (*buffer_switch)(long index, long directProcess);
	ASIOTime *(*buffer_switch_time_info)(ASIOTime *params, long index, long directProcess);
	long (*asio_message)(long selector, long value, void *opt, double *message);
	void (*sample_rate_changed)(ASIOSampleRate);
};

extern const struct asio_callback_set callback_sets[MAX_NUM_ASIO_DEVICES];

// Forward declarations for each callback set
void buffer_switch_0(long, long);
ASIOTime *buffer_switch_time_info_0(ASIOTime *, long, long);
long asio_message_callback_0(long, long, void *, double *);
void sample_rate_changed_callback_0(ASIOSampleRate);

void buffer_switch_1(long, long);
ASIOTime *buffer_switch_time_info_1(ASIOTime *, long, long);
long asio_message_callback_1(long, long, void *, double *);
void sample_rate_changed_callback_1(ASIOSampleRate);

void buffer_switch_2(long, long);
ASIOTime *buffer_switch_time_info_2(ASIOTime *, long, long);
long asio_message_callback_2(long, long, void *, double *);
void sample_rate_changed_callback_2(ASIOSampleRate);

void buffer_switch_3(long, long);
ASIOTime *buffer_switch_time_info_3(ASIOTime *, long, long);
long asio_message_callback_3(long, long, void *, double *);
void sample_rate_changed_callback_3(ASIOSampleRate);

void buffer_switch_4(long, long);
ASIOTime *buffer_switch_time_info_4(ASIOTime *, long, long);
long asio_message_callback_4(long, long, void *, double *);
void sample_rate_changed_callback_4(ASIOSampleRate);

void buffer_switch_5(long, long);
ASIOTime *buffer_switch_time_info_5(ASIOTime *, long, long);
long asio_message_callback_5(long, long, void *, double *);
void sample_rate_changed_callback_5(ASIOSampleRate);

void buffer_switch_6(long, long);
ASIOTime *buffer_switch_time_info_6(ASIOTime *, long, long);
long asio_message_callback_6(long, long, void *, double *);
void sample_rate_changed_callback_6(ASIOSampleRate);

void buffer_switch_7(long, long);
ASIOTime *buffer_switch_time_info_7(ASIOTime *, long, long);
long asio_message_callback_7(long, long, void *, double *);
void sample_rate_changed_callback_7(ASIOSampleRate);

void buffer_switch_8(long, long);
ASIOTime *buffer_switch_time_info_8(ASIOTime *, long, long);
long asio_message_callback_8(long, long, void *, double *);
void sample_rate_changed_callback_8(ASIOSampleRate);

void buffer_switch_9(long, long);
ASIOTime *buffer_switch_time_info_9(ASIOTime *, long, long);
long asio_message_callback_9(long, long, void *, double *);
void sample_rate_changed_callback_9(ASIOSampleRate);

void buffer_switch_10(long, long);
ASIOTime *buffer_switch_time_info_10(ASIOTime *, long, long);
long asio_message_callback_10(long, long, void *, double *);
void sample_rate_changed_callback_10(ASIOSampleRate);

void buffer_switch_11(long, long);
ASIOTime *buffer_switch_time_info_11(ASIOTime *, long, long);
long asio_message_callback_11(long, long, void *, double *);
void sample_rate_changed_callback_11(ASIOSampleRate);

void buffer_switch_12(long, long);
ASIOTime *buffer_switch_time_info_12(ASIOTime *, long, long);
long asio_message_callback_12(long, long, void *, double *);
void sample_rate_changed_callback_12(ASIOSampleRate);

void buffer_switch_13(long, long);
ASIOTime *buffer_switch_time_info_13(ASIOTime *, long, long);
long asio_message_callback_13(long, long, void *, double *);
void sample_rate_changed_callback_13(ASIOSampleRate);

void buffer_switch_14(long, long);
ASIOTime *buffer_switch_time_info_14(ASIOTime *, long, long);
long asio_message_callback_14(long, long, void *, double *);
void sample_rate_changed_callback_14(ASIOSampleRate);

void buffer_switch_15(long, long);
ASIOTime *buffer_switch_time_info_15(ASIOTime *, long, long);
long asio_message_callback_15(long, long, void *, double *);
void sample_rate_changed_callback_15(ASIOSampleRate);
