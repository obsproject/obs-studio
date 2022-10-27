/******************************************************************************
    Copyright (C) 2022-2026 pkv <pkv@obsproject.com>

    This file is part of win-asio.
    It is a minimal header extracted from the SDK removing anything we don't use
    in obs ASIO host.
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
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------- Basic Types --------------------- */
typedef int32_t ASIOBool;
typedef int32_t ASIOError;
typedef double ASIOSampleRate;
typedef int32_t ASIOSampleType;

/* --------------------- Boolean Values ------------------ */
#ifndef ASIOTrue
#define ASIOTrue 1
#define ASIOFalse 0
#endif

/* --------------------- Error Codes --------------------- */
enum {
	ASE_OK = 0,
	ASE_SUCCESS = 0x3f4847a0,
	ASE_NotPresent = -1000,
	ASE_HWMalfunction = -999,
	ASE_InvalidParameter = -998,
	ASE_InvalidMode = -997,
	ASE_SPNotAdvancing = -996,
	ASE_NoClock = -995,
	ASE_NotSupported = -994
};

/* --------------------- Sample Types -------------------- */
enum {
	ASIOSTInt16MSB = 0,
	ASIOSTInt24MSB = 1,
	ASIOSTInt32MSB = 2,
	ASIOSTFloat32MSB = 3,
	ASIOSTFloat64MSB = 4,
	ASIOSTInt32MSB16 = 8,
	ASIOSTInt32MSB18 = 9,
	ASIOSTInt32MSB20 = 10,
	ASIOSTInt32MSB24 = 11,
	ASIOSTInt16LSB = 16,
	ASIOSTInt24LSB = 17,
	ASIOSTInt32LSB = 18,
	ASIOSTFloat32LSB = 19,
	ASIOSTFloat64LSB = 20,
	ASIOSTInt32LSB16 = 24,
	ASIOSTInt32LSB18 = 25,
	ASIOSTInt32LSB20 = 26,
	ASIOSTInt32LSB24 = 27,
	ASIOSTLastEntry
};

/* --------------------- Selector Constants --------------- */
enum {
	kAsioSelectorSupported = 1,
	kAsioEngineVersion = 2,
	kAsioResetRequest = 3,
	kAsioBufferSizeChange = 4,
	kAsioResyncRequest = 5,
	kAsioLatenciesChanged = 6,
	kAsioSupportsTimeInfo = 7,
	kAsioSupportsTimeCode = 8,
	kAsioMMCCommand = 9,
	kAsioSupportsInputMonitor = 10,
	kAsioSupportsInputGain = 11,
	kAsioSupportsOutputGain = 12,
	kAsioSupportsInputMeter = 13,
	kAsioSupportsOutputMeter = 14,
	kAsioOverload = 15,
	kAsioCanReportOverload = 16
};

/* --------------------- Channel Info --------------------- */
typedef struct ASIOChannelInfo {
	long channel;
	long isInput;
	long isActive;
	long channelGroup;
	ASIOSampleType type;
	char name[64];
} ASIOChannelInfo;

/* --------------------- Clock Source --------------------- */
typedef struct ASIOClockSource {
	long index;
	long associatedChannel;
	long associatedGroup;
	long isCurrentSource;
	char name[32];
} ASIOClockSource;

/* --------------------- Buffer Info ---------------------- */
typedef struct ASIOBufferInfo {
	long isInput;
	long channelNum;
	void *buffers[2];
} ASIOBufferInfo;

/* --------------------- Time Info (stub) ----------------- */
typedef struct ASIOTime {
	double sampleRate;
	int64_t samplePosition;
	int64_t systemTime;
} ASIOTime;

/* --------------------- Callbacks ------------------------ */
typedef struct ASIOCallbacks {
	void (*bufferSwitch)(long doubleBufferIndex, long directProcess);
	ASIOTime *(*bufferSwitchTimeInfo)(ASIOTime *params, long doubleBufferIndex, long directProcess);
	long (*asioMessage)(long selector, long value, void *message, double *opt);
	void (*sampleRateDidChange)(ASIOSampleRate sRate);
} ASIOCallbacks;

#ifdef __cplusplus
}
#endif
