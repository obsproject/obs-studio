//-----------------------------------------------------------------------------
// Project     : SDK Core
//
// Category    : SDK Core Interfaces
// Filename    : pluginterfaces/base/ibstream.h
// Created by  : Steinberg, 01/2004
// Description : Interface for reading/writing streams
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "funknown.h"

namespace Steinberg {

//------------------------------------------------------------------------
/** Base class for streams.
\ingroup pluginBase
- read/write binary data from/to stream
- get/set stream read-write position (read and write position is the same)
*/
class IBStream: public FUnknown
{
public:
	enum IStreamSeekMode
	{
		kIBSeekSet = 0, ///< set absolute seek position
		kIBSeekCur,     ///< set seek position relative to current position
		kIBSeekEnd      ///< set seek position relative to stream end
	};

//------------------------------------------------------------------------
	/** Reads binary data from stream.
	\param buffer : destination buffer
	\param numBytes : amount of bytes to be read
	\param numBytesRead : result - how many bytes have been read from stream (set to 0 if this is of no interest) */
	virtual tresult PLUGIN_API read (void* buffer, int32 numBytes, int32* numBytesRead = nullptr) = 0;
	
	/** Writes binary data to stream.
	\param buffer : source buffer
	\param numBytes : amount of bytes to write
	\param numBytesWritten : result - how many bytes have been written to stream (set to 0 if this is of no interest) */
	virtual tresult PLUGIN_API write (void* buffer, int32 numBytes, int32* numBytesWritten = nullptr) = 0;
	
	/** Sets stream read-write position. 
	\param pos : new stream position (dependent on mode)
	\param mode : value of enum IStreamSeekMode
	\param result : new seek position (set to 0 if this is of no interest) */
	virtual tresult PLUGIN_API seek (int64 pos, int32 mode, int64* result = nullptr) = 0;
	
	/** Gets current stream read-write position. 
	\param pos : is assigned the current position if function succeeds */
	virtual tresult PLUGIN_API tell (int64* pos) = 0;
//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IBStream, 0xC3BF6EA2, 0x30994752, 0x9B6BF990, 0x1EE33E9B)

//------------------------------------------------------------------------
/** Stream with a size. 
\ingroup pluginBase
[extends IBStream] when stream type supports it (like file and memory stream)
*/
class ISizeableStream: public FUnknown
{
public:
//------------------------------------------------------------------------
	/** Return the stream size */
	virtual tresult PLUGIN_API getStreamSize (int64& size) = 0;
	/** Set the steam size. File streams can only be resized if they are write enabled. */
	virtual tresult PLUGIN_API setStreamSize (int64 size) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};
DECLARE_CLASS_IID (ISizeableStream, 0x04F9549E, 0xE02F4E6E, 0x87E86A87, 0x47F4E17F)

//------------------------------------------------------------------------
} // namespace Steinberg
