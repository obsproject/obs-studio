//------------------------------------------------------------------------
// Project     : SDK Base
// Version     : 1.0
//
// Category    : Helpers
// Filename    : base/source/fstreamer.h
// Created by  : Steinberg, 12/2005
// Description : Extract of typed stream i/o methods from FStream
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/funknown.h"

namespace Steinberg {

//------------------------------------------------------------------------
enum FSeekMode
{
	kSeekSet,
	kSeekCurrent,
	kSeekEnd
};

//------------------------------------------------------------------------
// FStreamer
//------------------------------------------------------------------------
/** Byteorder-aware base class for typed stream i/o. */
//------------------------------------------------------------------------
class FStreamer
{
public:
//------------------------------------------------------------------------
	FStreamer (int16 byteOrder = BYTEORDER);
	virtual ~FStreamer () {}

	/** @name Implementing class must override. */
	///@{
	virtual TSize readRaw (void*, TSize) = 0;          ///< Read one buffer of size.
	virtual TSize writeRaw (const void*, TSize) = 0;   ///< Write one buffer of size.
	virtual int64 seek (int64, FSeekMode) = 0;         ///< Set file position for stream.
	virtual int64 tell () = 0;                         ///< Return current file position.
	///@}

	/** @name Streams are byteOrder aware. */
	///@{
	inline void setByteOrder (int32 e) { byteOrder = (int16)e; }
	inline int32 getByteOrder () const { return byteOrder; }
	///@}

	/** @name read and write int8 and char. */
	///@{
	bool writeChar8 (char8); 
	bool readChar8 (char8&);                         
	bool writeUChar8 (unsigned char);   
	bool readUChar8 (unsigned char&);
	bool writeChar16 (char16 c); 
	bool readChar16 (char16& c); 

	bool writeInt8 (int8 c);   
	bool readInt8 (int8& c);
	bool writeInt8u (uint8 c);   
	bool readInt8u (uint8& c);
	///@}

	/** @name read and write int16. */
	///@{
	bool writeInt16 (int16);
	bool readInt16 (int16&);
	bool writeInt16Array (const int16* array, int32 count);  
	bool readInt16Array (int16* array, int32 count);  
	bool writeInt16u (uint16);
	bool readInt16u (uint16&);
	bool writeInt16uArray (const uint16* array, int32 count);  
	bool readInt16uArray (uint16* array, int32 count);  
	///@}

	/** @name read and write int32. */
	///@{
	bool writeInt32 (int32);  
	bool readInt32 (int32&);  
	bool writeInt32Array (const int32* array, int32 count);  
	bool readInt32Array (int32* array, int32 count);  
	bool writeInt32u (uint32);
	bool readInt32u (uint32&); 
	bool writeInt32uArray (const uint32* array, int32 count);  
	bool readInt32uArray (uint32* array, int32 count);  
	///@}

	/** @name read and write int64. */
	///@{
	bool writeInt64 (int64);
	bool readInt64 (int64&);
	bool writeInt64Array (const int64* array, int32 count);  
	bool readInt64Array (int64* array, int32 count);  
	bool writeInt64u (uint64);
	bool readInt64u (uint64&);
	bool writeInt64uArray (const uint64* array, int32 count);  
	bool readInt64uArray (uint64* array, int32 count);  
	///@}

	/** @name read and write float and float array. */
	///@{
	bool writeFloat (float);
	bool readFloat (float&);
	bool writeFloatArray (const float* array, int32 count);  
	bool readFloatArray (float* array, int32 count);  
	///@}

	/** @name read and write double and double array. */
	///@{
	bool writeDouble (double);                         
	bool readDouble (double&);                         
	bool writeDoubleArray (const double* array, int32 count);  
	bool readDoubleArray (double* array, int32 count);  
	///@}

	/** @name read and write Boolean. */
	///@{
	bool writeBool (bool);                                   ///< Write one boolean
	bool readBool (bool&);                                   ///< Read one bool.
	///@}

	/** @name read and write Strings. */
	///@{
	TSize writeString8 (const char8* ptr, bool terminate = false); ///< a direct output function writing only one string (ascii 8bit)
	TSize readString8 (char8* ptr, TSize size);				///< a direct input function reading only one string (ascii) (ended by a \n or \0 or eof)
	
	bool writeStr8 (const char8* ptr);				       ///< write a string length (strlen) and string itself
	char8* readStr8 ();									   ///< read a string length and string text (The return string must be deleted when use is finished)

	static int32 getStr8Size (const char8* ptr);	       ///< returns the size of a saved string

	bool writeStringUtf8 (const tchar* ptr);               ///< always terminated, converts to utf8 if non ascii characters are in string
	int32 readStringUtf8 (tchar* ptr, int32 maxSize);      ///< read a UTF8 string
	///@}

	bool skip (uint32 bytes);
	bool pad (uint32 bytes);


//------------------------------------------------------------------------
protected:
	int16 byteOrder;
};


//------------------------------------------------------------------------
/** FStreamSizeHolder Declaration
	remembers size of stream chunk for backward compatibility.
	
	<b>Example:</b>
	@code
	externalize (a)
	{
		FStreamSizeHolder sizeHolder;
		sizeHolder.beginWrite ();	// sets start mark, writes dummy size
		a << ....
		sizeHolder.endWrite ();		// jumps to start mark, updates size, jumps back here
	}

	internalize (a)
	{
		FStreamSizeHolder sizeHolder;
		sizeHolder.beginRead ();	// reads size, mark
		a >> ....
		sizeHolder.endRead ();		// jumps forward if new version has larger size
	}
	@endcode
*/
//------------------------------------------------------------------------
class FStreamSizeHolder
{
public:
	FStreamSizeHolder (FStreamer &s);

	void beginWrite ();	///< remembers position and writes 0
	int32 endWrite ();	///< writes and returns size (since the start marker)
	int32 beginRead ();	///< returns size
	void endRead ();	///< jump to end of chunk

protected:
	FStreamer &stream;
	int64 sizePos;
};

class IBStream;

//------------------------------------------------------------------------
// IBStreamer
//------------------------------------------------------------------------
/** Wrapper class for typed reading/writing from or to IBStream. 
	Can be used framework-independent in plug-ins. */
//------------------------------------------------------------------------
class IBStreamer: public FStreamer
{
public:
//------------------------------------------------------------------------
	/** Constructor for a given IBSTream and a byteOrder. */
	IBStreamer (IBStream* stream, int16 byteOrder = BYTEORDER);

	IBStream* getStream () { return stream; }	///< Returns the associated IBStream.

	// FStreamer overrides:					
	TSize readRaw (void*, TSize) SMTG_OVERRIDE;				///< Read one buffer of size.
	TSize writeRaw (const void*, TSize) SMTG_OVERRIDE;		///< Write one buffer of size.
	int64 seek (int64, FSeekMode) SMTG_OVERRIDE;			///< Set file position for stream.
	int64 tell () SMTG_OVERRIDE;							///< Return current file position.
//------------------------------------------------------------------------
protected:
	IBStream* stream;
};

//------------------------------------------------------------------------
} // namespace Steinberg
