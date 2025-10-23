//------------------------------------------------------------------------
// Project     : SDK Base
// Version     : 1.0
//
// Category    : Helpers
// Filename    : base/source/fbuffer.cpp
// Created by  : Steinberg, 2008
// Description : 
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "base/source/fbuffer.h"
#include "base/source/fstring.h"
#include <cstdlib>

namespace Steinberg {

//-------------------------------------------------------------------------------------
Buffer::Buffer () 
: buffer (nullptr)
, memSize (0)
, fillSize (0)
, delta (defaultDelta)
{}

//-------------------------------------------------------------------------------------
Buffer::Buffer (uint32 s, uint8 initVal)
: buffer (nullptr)
, memSize (s)
, fillSize (0)
, delta (defaultDelta)
{
 	if (memSize == 0)
		return;		
	buffer = (int8*)::malloc (memSize);
	if (buffer)
		memset (buffer, initVal, memSize);
	else
		memSize = 0;
}

//-------------------------------------------------------------------------------------
Buffer::Buffer (uint32 s)
: buffer (nullptr)
, memSize (s)
, fillSize (0)
, delta (defaultDelta)
{
 	if (memSize == 0)
		return;		
 	buffer = (int8*)::malloc (memSize);
	if (!buffer)
		memSize = 0;
}

//-------------------------------------------------------------------------------------
Buffer::Buffer (const void* b , uint32 s) 
: buffer (nullptr)
, memSize (s)
, fillSize (s)
, delta (defaultDelta)
{
 	if (memSize == 0)
		return;
 	buffer = (int8*)::malloc (memSize);
 	if (buffer)
		memcpy (buffer, b, memSize);
	else
	{
		memSize = 0;
		fillSize = 0;
	}
}

//-------------------------------------------------------------------------------------
Buffer::Buffer (const Buffer& bufferR)
: buffer (nullptr)
, memSize (bufferR.memSize)
, fillSize (bufferR.fillSize)
, delta (bufferR.delta)
{
 	if (memSize == 0)
		return;

 	buffer = (int8*)::malloc (memSize);
 	if (buffer)
		memcpy (buffer, bufferR.buffer, memSize);
	else
		memSize = 0;
}

//-------------------------------------------------------------------------------------
Buffer::~Buffer ()
{
 	if (buffer)
		::free (buffer);
 	buffer = nullptr;
}

//-------------------------------------------------------------------------------------
void Buffer::operator = (const Buffer& b2)
{
 	if (&b2 != this) 
	{
		setSize (b2.memSize);		
		if (b2.memSize > 0 && buffer)
			memcpy (buffer, b2.buffer, b2.memSize);
		fillSize = b2.fillSize;
		delta = b2.delta;
 	}
}

//-------------------------------------------------------------------------------------
bool Buffer::operator == (const Buffer& b2)const
{
	if (&b2 == this)
		return true;
	if (b2.getSize () != getSize ())
		return false;
	return memcmp (this->int8Ptr (), b2.int8Ptr (), getSize ()) == 0 ? true : false;		
}

//-------------------------------------------------------------------------------------
uint32 Buffer::get (void* b, uint32 size)
{
	uint32 maxGet = memSize - fillSize;
	if (size > maxGet)
		size = maxGet;
	if (size > 0)
		memcpy (b, buffer + fillSize, size);
	fillSize += size;
	return size;
}

//-------------------------------------------------------------------------------------
bool Buffer::put (char16 c)
{
	return put ((const void*)&c, sizeof (c));
}

//-------------------------------------------------------------------------------------
bool Buffer::put (uint8 byte)
{
	if (grow (fillSize + 1) == false)
		return false;
 	
  	buffer [fillSize++] = byte;
  	return true;
}

//-------------------------------------------------------------------------------------
bool Buffer::put (char c)
{
	if (grow (fillSize + 1) == false)
		return false;
 	
  	buffer [fillSize++] = c;
  	return true;
}

//-------------------------------------------------------------------------------------
bool Buffer::put (const void* toPut, uint32 s)
{
	if (!toPut)
		return false;

	if (grow (fillSize + s) == false)
		return false;

	memcpy (buffer + fillSize, toPut, s);
	fillSize += s;
	return true;
}

//-------------------------------------------------------------------------------------
bool Buffer::put (const String& str)
{ 
	return put ((const void*)str.text () , (str.length () + 1) * sizeof (tchar)); 
}

//-------------------------------------------------------------------------------------
bool Buffer::appendString8 (const char8* s)
{
	if (!s)
		return false;

	uint32 len = (uint32) strlen (s);
	return put (s, len);
}

//-------------------------------------------------------------------------------------
bool Buffer::appendString16 (const char16* s)
{
	if (!s)
		return false;
	ConstString str (s);
	uint32 len = (uint32) str.length () * sizeof (char16);
	return put (s, len);
}

//-------------------------------------------------------------------------------------
bool Buffer::prependString8 (const char8* s)
{
	if (!s)
		return false;

	uint32 len = (uint32) strlen (s);
	if (len > 0)
	{
		shiftStart (len);
		memcpy (buffer, s, len);
		return true;
	}
	return false;
}

//-------------------------------------------------------------------------------------
bool Buffer::prependString16 (const char16* s)
{
	if (!s)
		return false;

	ConstString str (s);
	uint32 len = (uint32) str.length () * sizeof (char16);
	
	if (len > 0)
	{
		shiftStart (len);
		memcpy (buffer, s, len);
		return true;
	}
	return false;
}

//-------------------------------------------------------------------------------------
bool Buffer::prependString8 (char8 c)
{
	shiftStart (sizeof (char));
	char* b = (char*)buffer;
	b [0] = c;
	return true;
}

//-------------------------------------------------------------------------------------
bool Buffer::prependString16 (char16 c)
{
	shiftStart (sizeof (char16));
	char16* b = (char16*)buffer;
	b [0] = c;
	return true;
}

//-------------------------------------------------------------------------------------
bool Buffer::copy (uint32 from, uint32 to, uint32 bytes)
{
	if (from + bytes > memSize || bytes == 0)
		return false;
	
	if (to + bytes > memSize)
		setSize (to + bytes);
	
	if (from + bytes > to && from < to)
	{              // overlap
		Buffer tmp (buffer + from, bytes);
		memcpy (buffer + to, tmp, bytes);
	}
	else	
		memcpy (buffer + to, buffer + from, bytes);
	return true;
}

//-------------------------------------------------------------------------------------
bool Buffer::makeHexString (String& result)
{
	unsigned char* data = uint8Ptr ();
	uint32 bytes = getSize ();

	if (data == nullptr || bytes == 0)
		return false;

	char8* stringBuffer = NEWSTR8 ((bytes * 2) + 1);
	if (!stringBuffer)
		return false;

	int32 count = 0;
	while (bytes > 0)
	{
		unsigned char t1 = ((*data) >> 4) & 0x0F;
		unsigned char t2 = (*data) & 0x0F;
		if (t1 < 10)
			t1 += '0';
		else
			t1 = t1 - 10 + 'A';
		if (t2 < 10)
			t2 += '0';
		else
			t2 = t2 - 10 + 'A';
		
		stringBuffer [count++] = t1;
		stringBuffer [count++] = t2;
		data++;
		bytes--;
	}
	stringBuffer [count] = 0;

	result.take ((void*)stringBuffer, false);
	return true;
}

//-------------------------------------------------------------------------------------
bool Buffer::fromHexString (const char8* string)
{
	flush ();
	if (string == nullptr)
		return false;

	int32 len = strlen8 (string);
	if (len == 0 || ((len & 1) == 1)/*odd number*/ )
		return false;

	setSize (len / 2);
	unsigned char* data = uint8Ptr ();

	bool upper = true;
	int32 count = 0;
	while (count < len)
	{
		char c = string [count];

		unsigned char d = 0;
		if (c >= '0' && c <= '9')		d += c - '0';
		else if (c >= 'A' && c <= 'F')	d += c - 'A' + 10;
		else if (c >= 'a' && c <= 'f')	d += c - 'a' + 10;
		else return false; // no hex string

		if (upper)
			data [count >> 1] = static_cast<unsigned char> (d << 4);
		else
			data [count >> 1] += d;

		upper = !upper;
		count++;
	}
	setFillSize (len / 2);
	return true;
}

//------------------------------------------------------------------------
void Buffer::set (uint8 value)
{
	if (buffer)
		memset (buffer, value, memSize);
}

//-------------------------------------------------------------------------------------
bool Buffer::setFillSize (uint32 c)
{
	if (c <= memSize)
	{
		fillSize = c;
		return true;	
	}
	return false;
}

//-------------------------------------------------------------------------------------
bool Buffer::truncateToFillSize ()
{
	if (fillSize < memSize)
		setSize (fillSize);
	
	return true;
}

//-------------------------------------------------------------------------------------
bool Buffer::grow (uint32 newSize)
{
	if (newSize > memSize)
	{
		if (delta == 0)
			delta = defaultDelta;
		uint32 s = ((newSize + delta - 1) / delta) * delta;
		return setSize (s);
	}
	return true;
}

//------------------------------------------------------------------------
void Buffer::shiftAt (uint32 position, int32 amount)
{
	if (amount > 0)
	{
		if (grow (fillSize + amount))
		{
			if (position < fillSize)
				memmove (buffer + amount + position, buffer + position, fillSize - position);
			
			fillSize += amount;
		}
	}	
	else if (amount < 0 && fillSize > 0)
	{
		uint32 toRemove = -amount;
	
		if (toRemove < fillSize)
		{
			if (position < fillSize)
				memmove (buffer + position, buffer + toRemove + position, fillSize - position - toRemove);
			fillSize -= toRemove;
		}
	}
}

//-------------------------------------------------------------------------------------
void Buffer::move (int32 amount, uint8 initVal)
{
	if (memSize == 0)
		return;

	if (amount > 0)
	{
		if ((uint32)amount < memSize)
		{
			memmove (buffer + amount, buffer, memSize - amount);
			memset (buffer, initVal, amount);
		}
		else
			memset (buffer, initVal, memSize);
	}
	else
	{	
		uint32 toRemove = -amount;
		if (toRemove < memSize)
		{
			memmove (buffer, buffer + toRemove, memSize - toRemove);
			memset (buffer + memSize - toRemove, initVal, toRemove);	
		}
		else
			memset (buffer, initVal, memSize);	
	}
}

//-------------------------------------------------------------------------------------
bool Buffer::setSize (uint32 newSize)
{
	if (memSize != newSize)
	{
 		if (buffer)
		{
			if (newSize > 0)
			{
				int8* newBuffer = (int8*) ::realloc (buffer, newSize);
				if (newBuffer == nullptr)
				{
					newBuffer = (int8*)::malloc (newSize);
					if (newBuffer)
					{
						uint32 tmp = newSize;
						if (tmp > memSize)
							tmp = memSize;
						memcpy (newBuffer, buffer, tmp);
						::free (buffer);
						buffer = newBuffer;
					}
					else
					{
						::free (buffer);
						buffer = nullptr;
					}
				}
				else
					buffer = newBuffer;
			}
			else
			{
				::free (buffer);
				buffer = nullptr;
			}
		}
		else
			buffer = (int8*)::malloc (newSize);

		if (newSize > 0 && !buffer)
			memSize = 0;
		else
			memSize = newSize;
		if (fillSize > memSize)
			fillSize = memSize;
	}

	return (newSize > 0) == (buffer != nullptr);
}

//-------------------------------------------------------------------------------------
void Buffer::fillup (uint8 value)
{
	if (getFree () > 0)
		memset (buffer + fillSize, value, getFree ());
}

//-------------------------------------------------------------------------------------
int8* Buffer::operator + (uint32 i)
{
	if (i < memSize)
		return buffer + i;
	
	static int8 eof;
	eof = 0;
	return &eof;
}

//-------------------------------------------------------------------------------------
bool Buffer::swap (int16 swapSize)
{
	return swap (buffer, memSize, swapSize);
}

//-------------------------------------------------------------------------------------
bool Buffer::swap (void* buffer, uint32 bufferSize, int16 swapSize)
{
	if (swapSize != kSwap16 && swapSize != kSwap32 && swapSize != kSwap64)
		return false;
	
	if (swapSize == kSwap16)
	{
		for (uint32 count = 0 ; count < bufferSize ; count += 2)
		{
			SWAP_16 ( * (((int16*)buffer) + count) );
		}
	}
	else if (swapSize == kSwap32)
	{
		for (uint32 count = 0 ; count < bufferSize ; count += 4) 
		{
			SWAP_32 ( * (((int32*)buffer) + count) );
		}
	}
	else if (swapSize == kSwap64)
	{
		for (uint32 count = 0 ; count < bufferSize ; count += 8) 
		{
			SWAP_64 ( * (((int64*)buffer) + count) );
		}
	}

	return true;
}

//-------------------------------------------------------------------------------------
void Buffer::take (Buffer& from)
{
	setSize (0);
	memSize = from.memSize;
	fillSize = from.fillSize;	
	buffer = from.buffer;
	from.buffer = nullptr;
	from.memSize = 0;
	from.fillSize = 0;
}

//-------------------------------------------------------------------------------------
int8* Buffer::pass ()
{
	int8* res = buffer;
	buffer = nullptr;
	memSize = 0;
	fillSize = 0;
	return res;
}

//-------------------------------------------------------------------------------------
bool Buffer::toWideString (int32 sourceCodePage)
{
	if (getFillSize () > 0)
	{
		if (str8 () [getFillSize () - 1] != 0) // multiByteToWideString only works with 0-terminated strings
			endString8 ();

		Buffer dest (getFillSize () * sizeof (char16));
		int32 result = String::multiByteToWideString (dest.str16 (), str8 (), dest.getFree () / sizeof (char16), sourceCodePage);
		if (result > 0)
		{
			dest.setFillSize ((result - 1) * sizeof (char16));
			take (dest);
			return true;
		}
		return false;
	}
	return true;
}

//-------------------------------------------------------------------------------------
bool Buffer::toMultibyteString (int32 destCodePage)
{
	if (getFillSize () > 0)
	{
		int32 textLength = getFillSize () / sizeof (char16); // wideStringToMultiByte only works with 0-terminated strings
		if (str16 () [textLength - 1] != 0)
			endString16 ();

		Buffer dest (getFillSize ());
		int32 result = String::wideStringToMultiByte (dest.str8 (), str16 (), dest.getFree (), destCodePage);
		if (result > 0)
		{
			dest.setFillSize (result - 1);
			take (dest);
			return true;
		}
		return false;
	}
	return true;
}

//------------------------------------------------------------------------
} // namespace Steinberg
