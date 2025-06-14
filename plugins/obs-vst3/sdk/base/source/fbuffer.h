//------------------------------------------------------------------------
// Project     : SDK Base
// Version     : 1.0
//
// Category    : Helpers
// Filename    : base/source/fbuffer.h
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

#pragma once

#include "pluginterfaces/base/ftypes.h"
#include <cstring> 

namespace Steinberg {
class String;

//------------------------------------------------------------------------
/** Buffer.
@ingroup adt

A Buffer is an object-oriented wrapper for a piece of memory.
It adds several utility functions, e.g. for managing the size of the Buffer,
appending or prepending values or strings to it.
Internally it uses the standard memory functions malloc(), free(), etc. */
//------------------------------------------------------------------------
class Buffer
{
public:
//---------------------------------------------------------------------
	
	/**	Default constructor, allocates no memory at all.
	*/
	Buffer ();

	/**	Constructor - creates a new Buffer with a given size and copies contents from optional memory pointer.
	\param[in] b : optional memory pointer with the size of at least the given size
	\param[in] size : the size of the new Buffer to be allocated, in bytes.
	*/
	Buffer (const void* b, uint32 size);

	/**	Constructor - creates a new Buffer with a given size and fills it all with a given value.
	\param[in] size : the size of the new Buffer to be allocated, in bytes.
	\param[in] initVal : the initial value the Buffer will be completely filled with
	*/
	Buffer (uint32 size, uint8 initVal);

	/**	Constructor - creates a new Buffer with a given size.
	\param[in] size : the size of the new Buffer to be allocated, in bytes.
	*/
	Buffer (uint32 size);

	/**	Copy constructor - creates a new Buffer from a given Buffer.
	\param[in] buff : the Buffer from which all memory will be copied to the new one
	*/
	Buffer (const Buffer& buff);

	/**	Destructor - deallocates the internal memory.
	*/
	virtual ~Buffer ();
	
	/**	Assignment operator - copies contents from a given Buffer and increases the size if necessary.
	\param[in] buff : the Buffer from which all memory will be copied
	*/
	void operator = (const Buffer& buff);
	
	/**	Comparison operator - copies contents from a given Buffer and increases the size if necessary.
	\param[in] buff : the Buffer to be compared to
	\return true, if the given Buffer's content is equal to this one, else false
	*/
	bool operator == (const Buffer& buff)const;

	uint32 getSize () const {return memSize;}		///< \return the actual size of the Buffer's memory, in bytes.

	/**	Sets a new size for this Buffer, keeping as much content as possible.
	\param[in] newSize : the new size for the Buffer, in bytes, newSize maybe zero
	\return true, if the new size could be adapted, else false
	*/
	bool setSize (uint32 newSize);

	/**	Increases the Buffer to the next block, block size given by delta.
	\param[in] memSize : the new minimum size of the Buffer, newSize maybe zero
	\return true, if the Buffer could be grown successfully, else false
	*/
	bool grow (uint32 memSize);
	bool setMaxSize (uint32 size) {return grow (size);}	///< see \ref grow()

	void fillup (uint8 initVal = 0);				///< set from fillSize to end
	uint32 getFillSize ()const {return fillSize;}	///< \return the actual fill size
	bool setFillSize (uint32 c);					///< sets a new fill size, does not change any memory
	inline void flush () {setFillSize (0);}			///< sets fill size to zero
	bool truncateToFillSize ();						///< \return always true, truncates the size of the Buffer to the actual fill size

	bool isFull () const { return (fillSize == memSize); }	///< \return true, if all memory is filled up, else false
	uint32 getFree () const { return (memSize - fillSize); }///< \return remaining memory

	inline void shiftStart (int32 amount) {return shiftAt (0, amount);} ///< moves all memory by given amount, grows the Buffer if necessary
	void shiftAt (uint32 position, int32 amount);						///< moves memory starting at the given position
	void move (int32 amount, uint8 initVal = 0);						///< shifts memory at start without growing the buffer, so data is lost and initialized with init val

	bool copy (uint32 from, uint32 to, uint32 bytes);	///< copies a number of bytes from one position to another, the size may be adapted
	uint32 get (void* b, uint32 size);					///< copy to buffer from fillSize, and shift fillSize

	void setDelta (uint32 d) {delta = d;}				///< define the block size by which the Buffer grows, see \ref grow()

	bool put (uint8);							///< append value at end, grows Buffer if necessary 
	bool put (char16 c);                        ///< append value at end, grows Buffer if necessary
	bool put (char c);							///< append value at end, grows Buffer if necessary
	bool put (const void* , uint32 size);		///< append bytes from a given buffer, grows Buffer if necessary
	bool put (void* , uint32 size);				///< append bytes from a given buffer, grows Buffer if necessary
	bool put (uint8* , uint32 size);			///< append bytes from a given buffer, grows Buffer if necessary
	bool put (char8* , uint32 size);			///< append bytes from a given buffer, grows Buffer if necessary
	bool put (const uint8* , uint32 size);		///< append bytes from a given buffer, grows Buffer if necessary
	bool put (const char8* , uint32 size);		///< append bytes from a given buffer, grows Buffer if necessary
	bool put (const String&);					///< append String at end, grows Buffer if necessary

	void set (uint8 value);		///< fills complete Buffer with given value
	
	// strings ----------------
	bool appendString (const tchar* s);
	bool appendString (tchar* s);
	bool appendString (tchar c)                   { return put (c); }

	bool appendString8 (const char8* s);	
	bool appendString16 (const char16* s);

	bool appendString8 (char8* s)                 { return appendString8 ((const char8*)s); }
	bool appendString8 (unsigned char* s)		  { return appendString8 ((const char8*)s); }         
	bool appendString8 (const unsigned char* s)   { return appendString8 ((const char8*)s); }

	bool appendString8 (char8 c)                  { return put ((uint8)c); }
	bool appendString8 (unsigned char c)          { return put (c); }
	bool appendString16 (char16 c)                { return put (c); }
	bool appendString16 (char16* s)               { return appendString16 ((const char16*)s); }

	bool prependString (const tchar* s);   
	bool prependString (tchar* s);   
	bool prependString (tchar c);                

	bool prependString8 (const char8* s);           
	bool prependString16 (const char16* s);

	bool prependString8 (char8 c);
	bool prependString8 (unsigned char c)         { return prependString8 ((char8)c); }
	bool prependString8 (char8* s)                { return prependString8 ((const char8*)s); }
	bool prependString8 (unsigned char* s)        { return prependString8((const char8*)s); }           
	bool prependString8 (const unsigned char* s)  { return prependString8 ((const char8*)s); } 
	bool prependString16 (char16 c);
	bool prependString16 (char16* s)              { return prependString16 ((const char16*)s); }

	bool operator+= (const char* s)               { return appendString8 (s); }
	bool operator+= (char c)                      { return appendString8 (c); }
	bool operator+= (const char16* s)             { return appendString16 (s); }
	bool operator+= (char16 c)                    { return appendString16 (c); }

	bool operator= (const char* s)                { flush (); return appendString8 (s); }
	bool operator= (const char16* s)              { flush (); return appendString16 (s); }
	bool operator= (char8 c)                      { flush (); return appendString8 (c); }
	bool operator= (char16 c)                     { flush (); return appendString16 (c); }

	void endString () {put (tchar (0));}
	void endString8 () {put (char8 (0));}
	void endString16 () {put (char16 (0));}

	bool makeHexString (String& result);
	bool fromHexString (const char8* string);

	// conversion
	operator void* () const { return (void*)buffer; }				///< conversion
	inline tchar*   str ()   const {return (tchar*)buffer;}			///< conversion
	inline char8*   str8 ()   const {return (char8*)buffer;}		///< conversion
	inline char16*  str16 ()   const {return (char16*)buffer;}		///< conversion
	inline int8*   int8Ptr ()   const {return (int8*)buffer;}		///< conversion
	inline uint8*  uint8Ptr ()  const {return (uint8*)buffer; }		///< conversion
	inline int16*  int16Ptr ()  const {return (int16*)buffer; }		///< conversion
    inline uint16* uint16Ptr () const {return (uint16*)buffer; }	///< conversion
	inline int32*  int32Ptr ()  const {return (int32*)buffer; }		///< conversion
	inline uint32* uint32Ptr () const {return (uint32*)buffer; }	///< conversion
	inline float*  floatPtr ()  const {return (float*)buffer; }		///< conversion
	inline double* doublePtr () const {return (double*)buffer; }	///< conversion
	inline char16*  wcharPtr ()  const {return (char16*)buffer;}	///< conversion

	int8* operator + (uint32 i);	///< \return the internal Buffer's address plus the given offset i, zero if offset is out of range
	
	int32 operator ! ()  { return buffer == nullptr; }
	
	enum swapSize 
	{
		kSwap16 = 2,
		kSwap32 = 4,
		kSwap64 = 8
	};
	bool swap (int16 swapSize);											///< swap all bytes of this Buffer by the given swapSize
	static bool swap (void* buffer, uint32 bufferSize, int16 swapSize);	///< utility, swap given number of bytes in given buffer by the given swapSize

	void take (Buffer& from);	///< takes another Buffer's memory, frees the current Buffer's memory
	int8* pass ();				///< pass the current Buffer's memory

	/**	Converts a Buffer's content to UTF-16 from a given multi-byte code page, Buffer must contain char8 of given encoding.
		\param[in] sourceCodePage : the actual code page of the Buffer's content
		\return true, if the conversion was successful, else false
	*/
	virtual bool toWideString (int32 sourceCodePage); // Buffer contains char8 of given encoding -> utf16

	/**	Converts a Buffer's content from UTF-16 to a given multi-byte code page, Buffer must contain UTF-16 encoded characters.
		\param[in] destCodePage : the desired code page to convert the Buffer's content to
		\return true, if the conversion was successful, else false
	*/
	virtual bool toMultibyteString (int32 destCodePage); // Buffer contains utf16 -> char8 of given encoding

//------------------------------------------------------------------------
protected:
	static const uint32 defaultDelta = 0x1000; // 0x1000
	
	int8* buffer;
	uint32 memSize;
	uint32 fillSize;
	uint32 delta;
};

inline bool Buffer::put (void* p, uint32 count)     { return put ((const void*)p , count ); }
inline bool Buffer::put (uint8 * p, uint32 count)   { return put ((const void*)p , count ); }
inline bool Buffer::put (char8* p, uint32 count)    { return put ((const void*)p , count ); }
inline bool Buffer::put (const uint8* p, uint32 count) { return put ((const void*)p , count ); }
inline bool Buffer::put (const char8* p, uint32 count) { return put ((const void*)p , count ); }

//------------------------------------------------------------------------
inline bool Buffer::appendString (const tchar* s)
{
#ifdef UNICODE
	return appendString16 (s);
#else
	return appendString8 (s);
#endif
}

//------------------------------------------------------------------------
inline bool Buffer::appendString (tchar* s)
{
#ifdef UNICODE
	return appendString16 (s);
#else
	return appendString8 (s);
#endif
}

//------------------------------------------------------------------------
inline bool Buffer::prependString (const tchar* s)
{
#ifdef UNICODE
	return prependString16 (s);
#else
	return prependString8 (s);
#endif
}

//------------------------------------------------------------------------
inline bool Buffer::prependString (tchar* s)
{
#ifdef UNICODE
	return prependString16 (s);
#else
	return prependString8 (s);
#endif
}

//------------------------------------------------------------------------
inline bool Buffer::prependString (tchar c)
{
#ifdef UNICODE
	return prependString16 (c);
#else
	return prependString8 (c);
#endif
}

//------------------------------------------------------------------------
} // namespace Steinberg
