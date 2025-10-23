//------------------------------------------------------------------------
// Project     : SDK Base
// Version     : 1.0
//
// Category    : Helpers
// Filename    : base/source/fstreamer.cpp
// Created by  : Steinberg, 15.12.2005
// Description : Extract of typed stream i/o methods from FStream
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "fstreamer.h"

#include "base/source/fstring.h"
#include "base/source/fbuffer.h"
#include "pluginterfaces/base/ibstream.h"

#ifndef UNICODE
#include "pluginterfaces/base/futils.h"
#endif

namespace Steinberg {

//------------------------------------------------------------------------
// IBStreamer
//------------------------------------------------------------------------
IBStreamer::IBStreamer (IBStream* stream, int16 _byteOrder)
: FStreamer (_byteOrder)
, stream (stream)
{}

//------------------------------------------------------------------------
TSize IBStreamer::readRaw (void* buffer, TSize size)
{
	int32 numBytesRead = 0;
	stream->read (buffer, (int32)size, &numBytesRead);
	return numBytesRead;
}

//------------------------------------------------------------------------
TSize IBStreamer::writeRaw (const void* buffer, TSize size)
{
	int32 numBytesWritten = 0;
	stream->write ((void*)buffer, (int32)size, &numBytesWritten);
	return numBytesWritten;
}

//------------------------------------------------------------------------
int64 IBStreamer::seek (int64 pos, FSeekMode mode)
{
	int64 result = -1;
	stream->seek (pos, mode, &result);
	return result;
}

//------------------------------------------------------------------------
int64 IBStreamer::tell ()
{
	int64 pos = 0;
	stream->tell (&pos);
	return pos;
}

//------------------------------------------------------------------------
// FStreamSizeHolder Implementation
//------------------------------------------------------------------------
FStreamSizeHolder::FStreamSizeHolder (FStreamer &s)
: stream (s), sizePos (-1)
{}

//------------------------------------------------------------------------
void FStreamSizeHolder::beginWrite ()
{
	sizePos = stream.tell ();
	stream.writeInt32 (0L);
}

//------------------------------------------------------------------------
int32 FStreamSizeHolder::endWrite ()
{
	if (sizePos < 0)
		return 0;
	
	int64 currentPos = stream.tell ();

	stream.seek (sizePos, kSeekSet);
	int32 size = int32 (currentPos - sizePos - sizeof (int32));
	stream.writeInt32 (size);
	
	stream.seek (currentPos, kSeekSet);
	return size;
}

//------------------------------------------------------------------------
int32 FStreamSizeHolder::beginRead ()
{
	sizePos = stream.tell ();
	int32 size = 0;
	stream.readInt32 (size);
	sizePos += size + sizeof (int32);
	return size;
}

//------------------------------------------------------------------------
void FStreamSizeHolder::endRead ()
{
	if (sizePos >= 0)
		stream.seek (sizePos, kSeekSet);
}

//------------------------------------------------------------------------
// FStreamer
//------------------------------------------------------------------------
FStreamer::FStreamer (int16 _byteOrder)
: byteOrder (_byteOrder)
{}

// int8 / char -----------------------------------------------------------
//------------------------------------------------------------------------
bool FStreamer::writeChar8 (char8 c)
{
	return writeRaw ((void*)&c, sizeof (char8)) == sizeof (char8);
}

//------------------------------------------------------------------------
bool FStreamer::readChar8 (char8& c)
{
	return readRaw ((void*)&c, sizeof (char8)) == sizeof (char8);
}

//------------------------------------------------------------------------
bool FStreamer::writeUChar8 (unsigned char c)
{
	return writeRaw ((void*)&c, sizeof (unsigned char)) == sizeof (unsigned char);
}

//------------------------------------------------------------------------
bool FStreamer::readUChar8 (unsigned char& c)
{
	return readRaw ((void*)&c, sizeof (unsigned char)) == sizeof (unsigned char);
}

//------------------------------------------------------------------------
bool FStreamer::writeChar16 (char16 c)
{
	if (BYTEORDER != byteOrder)
		SWAP_16 (c);
	return writeRaw ((void*)&c, sizeof (char16)) == sizeof (char16);
}

//------------------------------------------------------------------------
bool FStreamer::readChar16 (char16& c)
{
	if (readRaw ((void*)&c, sizeof (char16)) == sizeof (char16))
	{
		if (BYTEORDER != byteOrder)
			SWAP_16 (c);
		return true;
	}
	c = 0;
	return false;
}

//------------------------------------------------------------------------
bool FStreamer::writeInt8 (int8 c)
{
	return writeRaw ((void*)&c, sizeof (int8)) == sizeof (int8);
}

//------------------------------------------------------------------------
bool FStreamer::readInt8 (int8& c)
{
	return readRaw ((void*)&c, sizeof (int8)) == sizeof (int8);
}

//------------------------------------------------------------------------
bool FStreamer::writeInt8u (uint8 c)
{
	return writeRaw ((void*)&c, sizeof (uint8)) == sizeof (uint8);
}

//------------------------------------------------------------------------
bool FStreamer::readInt8u (uint8& c)
{
	return readRaw ((void*)&c, sizeof (uint8)) == sizeof (uint8);
}

// int16 -----------------------------------------------------------------
//------------------------------------------------------------------------
bool FStreamer::writeInt16 (int16 i)
{
	if (BYTEORDER != byteOrder)
		SWAP_16 (i);
	return writeRaw ((void*)&i, sizeof (int16)) == sizeof (int16);
}

//------------------------------------------------------------------------
bool FStreamer::readInt16 (int16& i)
{
	if (readRaw ((void*)&i, sizeof (int16)) == sizeof (int16))
	{
		if (BYTEORDER != byteOrder)
			SWAP_16 (i);
		return true;
	}
	i = 0;
	return false;
}

//------------------------------------------------------------------------
bool FStreamer::writeInt16Array (const int16* array, int32 count)
{
	for (int32 i = 0; i < count; i++)
	{
		if (!writeInt16 (array[i]))
			return false;
	}
	return true;
}

//------------------------------------------------------------------------
bool FStreamer::readInt16Array (int16* array, int32 count)
{
	for (int32 i = 0; i < count; i++)
	{
		if (!readInt16 (array[i]))
			return false;
	}
	return true;
}

//------------------------------------------------------------------------
bool FStreamer::writeInt16u (uint16 i)
{
	if (BYTEORDER != byteOrder)
		SWAP_16 (i);
	return writeRaw ((void*)&i, sizeof (uint16)) == sizeof (uint16);
}

//------------------------------------------------------------------------
bool FStreamer::readInt16u (uint16& i)
{
	if (readRaw ((void*)&i, sizeof (uint16)) == sizeof (uint16))
	{
		if (BYTEORDER != byteOrder)
			SWAP_16 (i);
		return true;
	}
	i = 0;
	return false;
}

//------------------------------------------------------------------------
bool FStreamer::writeInt16uArray (const uint16* array, int32 count)
{
	for (int32 i = 0; i < count; i++)
	{
		if (!writeInt16u (array[i]))
			return false;
	}
	return true;
}

//------------------------------------------------------------------------
bool FStreamer::readInt16uArray (uint16* array, int32 count)
{
	for (int32 i = 0; i < count; i++)
	{
		if (!readInt16u (array[i]))
			return false;
	}
	return true;
}

// int32 -----------------------------------------------------------------
//------------------------------------------------------------------------
bool FStreamer::writeInt32 (int32 i)
{
	if (BYTEORDER != byteOrder)
		SWAP_32 (i);
	return writeRaw ((void*)&i, sizeof (int32)) == sizeof (int32);
}

//------------------------------------------------------------------------
bool FStreamer::readInt32 (int32& i)
{
	if (readRaw ((void*)&i, sizeof (int32)) == sizeof (int32))
	{
		if (BYTEORDER != byteOrder)
			SWAP_32 (i);
		return true;
	}
	i = 0;
	return false;
}

//------------------------------------------------------------------------
bool FStreamer::writeInt32Array (const int32* array, int32 count)
{
	for (int32 i = 0; i < count; i++)
	{
		if (!writeInt32 (array[i]))
			return false;
	}
	return true;
}

//------------------------------------------------------------------------
bool FStreamer::readInt32Array (int32* array, int32 count)
{
	for (int32 i = 0; i < count; i++)
	{
		if (!readInt32 (array[i]))
			return false;
	}
	return true;
}

//------------------------------------------------------------------------
bool FStreamer::writeInt32u (uint32 i)
{
	if (BYTEORDER != byteOrder)
		SWAP_32 (i);
	return writeRaw ((void*)&i, sizeof (uint32)) == sizeof (uint32);
}

//------------------------------------------------------------------------
bool FStreamer::readInt32u (uint32& i)
{
	if (readRaw ((void*)&i, sizeof (uint32)) == sizeof (uint32))
	{
		if (BYTEORDER != byteOrder)
			SWAP_32 (i);
		return true;
	}
	i = 0;
	return false;
}

//------------------------------------------------------------------------
bool FStreamer::writeInt32uArray (const uint32* array, int32 count)
{
	for (int32 i = 0; i < count; i++)
	{
		if (!writeInt32u (array[i]))
			return false;
	}
	return true;
}

//------------------------------------------------------------------------
bool FStreamer::readInt32uArray (uint32* array, int32 count)
{
	for (int32 i = 0; i < count; i++)
	{
		if (!readInt32u (array[i]))
			return false;
	}
	return true;
}

// int64 -----------------------------------------------------------------
//------------------------------------------------------------------------
bool FStreamer::writeInt64 (int64 i)
{
	if (BYTEORDER != byteOrder)
		SWAP_64 (i);
	return writeRaw ((void*)&i, sizeof (int64)) == sizeof (int64);
}

//------------------------------------------------------------------------
bool FStreamer::readInt64 (int64& i)
{
	if (readRaw ((void*)&i, sizeof (int64)) == sizeof (int64))
	{
		if (BYTEORDER != byteOrder)
			SWAP_64 (i);
		return true;
	}
	i = 0;
	return false;
}

//------------------------------------------------------------------------
bool FStreamer::writeInt64Array (const int64* array, int32 count)
{
	for (int32 i = 0; i < count; i++)
	{
		if (!writeInt64 (array[i]))
			return false;
	}
	return true;
}

//------------------------------------------------------------------------
bool FStreamer::readInt64Array (int64* array, int32 count)
{
	for (int32 i = 0; i < count; i++)
	{
		if (!readInt64 (array[i]))
			return false;
	}
	return true;
}

//------------------------------------------------------------------------
bool FStreamer::writeInt64u (uint64 i)
{
	if (BYTEORDER != byteOrder)
		SWAP_64 (i);
	return writeRaw ((void*)&i, sizeof (uint64)) == sizeof (uint64);
}

//------------------------------------------------------------------------
bool FStreamer::readInt64u (uint64& i)
{
	if (readRaw ((void*)&i, sizeof (uint64)) == sizeof (uint64))
	{
		if (BYTEORDER != byteOrder)
			SWAP_64 (i);
		return true;
	}
	i = 0;
	return false;
}

//------------------------------------------------------------------------
bool FStreamer::writeInt64uArray (const uint64* array, int32 count)
{
	for (int32 i = 0; i < count; i++)
	{
		if (!writeInt64u (array[i]))
			return false;
	}
	return true;
}

//------------------------------------------------------------------------
bool FStreamer::readInt64uArray (uint64* array, int32 count)
{
	for (int32 i = 0; i < count; i++)
	{
		if (!readInt64u (array[i]))
			return false;
	}
	return true;
}

// float / double --------------------------------------------------------
//------------------------------------------------------------------------
bool FStreamer::writeFloat (float f)
{
	if (BYTEORDER != byteOrder)
		SWAP_32 (f);
	return writeRaw ((void*)&f, sizeof (float)) == sizeof (float);
}

//------------------------------------------------------------------------
bool FStreamer::readFloat (float& f)
{
	if (readRaw ((void*)&f, sizeof (float)) == sizeof (float))
	{
		if (BYTEORDER != byteOrder)
			SWAP_32 (f);
		return true;
	}
	f = 0.f;
	return false;
}

//------------------------------------------------------------------------
bool FStreamer::writeFloatArray (const float* array, int32 count)
{
	for (int32 i = 0; i < count; i++)
	{
		if (!writeFloat (array[i]))
			return false;
	}
	return true;
}

//------------------------------------------------------------------------
bool FStreamer::readFloatArray (float* array, int32 count)
{
	for (int32 i = 0; i < count; i++)
	{
		if (!readFloat (array[i]))
			return false;
	}
	return true;
}

//------------------------------------------------------------------------
bool FStreamer::writeDouble (double d)
{
	if (BYTEORDER != byteOrder)
		SWAP_64 (d);
	return writeRaw ((void*)&d, sizeof (double)) == sizeof (double);
}

//------------------------------------------------------------------------
bool FStreamer::readDouble (double& d)
{
	if (readRaw ((void*)&d, sizeof (double)) == sizeof (double))
	{
		if (BYTEORDER != byteOrder)
			SWAP_64 (d);
		return true;
	}
	d = 0.0;
	return false;
}

//------------------------------------------------------------------------
bool FStreamer::writeDoubleArray (const double* array, int32 count)
{
	for (int32 i = 0; i < count; i++)
	{
		if (!writeDouble (array[i]))
			return false;
	}
	return true;
}

//------------------------------------------------------------------------
bool FStreamer::readDoubleArray (double* array, int32 count)
{
	for (int32 i = 0; i < count; i++)
	{
		if (!readDouble (array[i]))
			return false;
	}
	return true;
}

//------------------------------------------------------------------------
bool FStreamer::readBool (bool& b)
{
	int16 v = 0;
	bool res = readInt16 (v);
	b = (v != 0);
	return res;
}

//------------------------------------------------------------------------
bool FStreamer::writeBool (bool b)
{
	return writeInt16 ((int16)b);
}

//------------------------------------------------------------------------
TSize FStreamer::writeString8 (const char8* ptr, bool terminate)
{
	TSize size = strlen (ptr);
	if (terminate) // write \0
		size++;

	return writeRaw ((void*)ptr, size);
}

//------------------------------------------------------------------------
TSize FStreamer::readString8 (char8* ptr, TSize size)
{
	if (size < 1 || ptr == nullptr)
		return 0;

	TSize i = 0;
	char8 c = 0;
	while (i < size)
	{
		if (readRaw ((void*)&c, sizeof (char)) != sizeof (char))
			break;
		ptr[i] = c;
		if (c == '\n' || c == '\0')
			break;
		i++;
	}
	// remove at end \n (LF) or \r\n (CR+LF)
	if (c == '\n')
	{
		if (i > 0 && ptr[i - 1] == '\r')
			i--;
	}
	if (i >= size)
		i = size - 1;
	ptr[i] = 0;
	
	return i;
}

//------------------------------------------------------------------------
bool FStreamer::writeStringUtf8 (const tchar* ptr)
{
	bool isUtf8 = false;

	String str (ptr);
	if (str.isAsciiString () == false) 
	{
		str.toMultiByte (kCP_Utf8);
		isUtf8 = true;
	}
	else
	{
		str.toMultiByte ();
	}

	if (isUtf8) 
		if (writeRaw (kBomUtf8, kBomUtf8Length) != kBomUtf8Length)
			return false;

	TSize size = str.length () + 1;
	if (writeRaw (str.text8 (), size) != size)
		return false;

	return true;
}

//------------------------------------------------------------------------
int32 FStreamer::readStringUtf8 (tchar* ptr, int32 nChars)
{
	char8 c = 0;

	ptr [0] = 0;

	Buffer tmp;
	tmp.setDelta (1024);

	while (true)
	{
		if (readRaw ((void*)&c, sizeof (char)) != sizeof (char))
			break;
		tmp.put (c);
		if (c == '\0')
			break;
	}

	char8* source = tmp.str8 ();
	uint32 codePage = kCP_Default; // for legacy take default page if no utf8 bom is present...
	if (tmp.getFillSize () > 2)
	{
		if (memcmp (source, kBomUtf8, kBomUtf8Length) == 0)
		{
			codePage = kCP_Utf8;
			source += 3;
		}
	}

	if (tmp.getFillSize () > 1)
	{
#ifdef UNICODE
		ConstString::multiByteToWideString (ptr, source, nChars, codePage);
#else
		if (codePage == kCP_Utf8)
		{
			Buffer wideBuffer (tmp.getFillSize () * 3);
			ConstString::multiByteToWideString (wideBuffer.wcharPtr (), source, wideBuffer.getSize () / 2, kCP_Utf8);
			ConstString::wideStringToMultiByte (ptr, wideBuffer.wcharPtr (), nChars);		
		}
		else
		{
			memcpy (ptr, source, Min<TSize> (nChars, tmp.getFillSize ()));
		}
#endif
	}

	ptr[nChars - 1] = 0;
	return ConstString (ptr).length ();
}

//------------------------------------------------------------------------
bool FStreamer::writeStr8 (const char8* s)
{
	int32 length = (s) ? (int32) strlen (s) + 1 : 0;
	if (!writeInt32 (length))
		return false;

	if (length > 0)
		return writeRaw (s, sizeof (char8) * length) == static_cast<TSize>(sizeof (char8) * length);

	return true;
}

//------------------------------------------------------------------------
int32 FStreamer::getStr8Size (const char8* s) 
{
	return sizeof (int32) + (int32)strlen (s) + 1;
}

//------------------------------------------------------------------------
char8* FStreamer::readStr8 ()
{
	int32 length;
	if (!readInt32 (length))
		return nullptr;
	
	// check corruption
	if (length > 262144)
		return nullptr;

	char8* s = (length > 0) ? NEWSTR8 (length) : nullptr;
	if (s)
		readRaw (s, length * sizeof (char8));
	return s;
}

//------------------------------------------------------------------------
bool FStreamer::skip (uint32 bytes)
{
    int8 tmp;
	while (bytes-- > 0) 
	{
		if (readInt8 (tmp) == false)
			return false;	
    }
	return true;
}

//------------------------------------------------------------------------
bool FStreamer::pad (uint32 bytes)
{
    while (bytes-- > 0) 
	{
		if (writeInt8 (0) == false)
			return false;	
	}
	return true;
}

//------------------------------------------------------------------------
} // namespace Steinberg
