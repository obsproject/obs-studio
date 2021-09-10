/* SPDX-License-Identifier: MIT */
/**
	@file		rawfile.h
	@brief		Defines data structures used to read and write AJA raw files.
	@copyright	(C) 2011-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_RAWFILE_H 
#define AJA_RAWFILE_H

const uint32_t AJAAudioHeaderTag = AJA_FOURCC('a','j','a','h');
const uint32_t AJAVideoHeaderTag = AJA_FOURCC('a','j','v','h');
const uint32_t AJAAudioDataTag   = AJA_FOURCC('a','j','a','d');
const uint32_t AJAVideoDataTag   = AJA_FOURCC('a','j','v','d');

#pragma pack(push,1)

struct AJARawAudioHeader
{
	uint32_t tag;			// should be AJAAudioHeaderTag
	int64_t size;			// sizeof(AJARawAudioHeader)
	int64_t scale;			// video frame scale for timecode, ie. 30000
	int64_t duration;       // video frame duration for timecode, ie. 1001
	int64_t timecode;		// video frame offset of first sample

    uint32_t rate;			// number of samples per second, ie. 48000
	uint32_t channels;		// number of channels, ie. 2,4,8, etc.
	uint32_t bigEndian;		// non-zero if samples are in big endian format
	uint32_t sampleSize;    // number of bytes per sample, ie. 2,3,4
	uint32_t isFloat;		// non-zero if samples are in floating point format

							// crank this out to 1024 bytes for additional future stuff
	uint32_t reserved[(1024 - (sizeof(uint32_t) * 6) - (sizeof(int64_t) * 4)) / sizeof(uint32_t)];
};

struct AJARawVideoHeader
{
	uint32_t tag;			// should be AJAVideoHeaderTag
	int64_t size;			// sizeof(AJARawVideoHeader)
	int64_t scale;			// video frame scale for timecode, ie. 30000
	int64_t duration;       // video frame duration for timecode, ie. 1001
	int64_t timecode;		// video frame offset of first sample

	uint32_t fourcc;		// fourcc of video frames, this format implies fixed bytes per frame
    uint32_t xRes;			// x resolution of video frames, ie. 1920
	uint32_t yRes;			// y resolution of video frames, ie. 1080
	uint32_t bytesPerFrame; // number of bytes per frame of video, all frames are the same
	uint32_t aspectX;		// x aspect of video frames, ie. 1
	uint32_t aspectY;		// y aspect of video frames, ie. 1
	uint32_t interlace;     // interlace of frames, 0-progressive, 1-lower, 2-upper

							// crank this out to 1024 bytes for additional future stuff
	uint32_t reserved[(1024 - (sizeof(uint32_t) * 8) - (sizeof(int64_t) * 4)) / sizeof(uint32_t)];
};

struct AJARawDataHeader
{
	uint32_t tag;           // should be AJAVideoDataTag or AJAAudioDataTag
	int64_t  size;          // if size is zero, go to end of file with this chunk
};

#pragma pack(pop)

#endif	//	AJA_RAWFILE_H
