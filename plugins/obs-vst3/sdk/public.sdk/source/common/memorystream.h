//-----------------------------------------------------------------------------
// Project     : SDK Core
//
// Category    : Common Classes
// Filename    : public.sdk/source/common/memorystream.h
// Created by  : Steinberg, 03/2008
// Description : IBStream Implementation for memory blocks
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2024, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
//   * Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation 
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this 
//     software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/ibstream.h"

namespace Steinberg {

//------------------------------------------------------------------------
/** Memory based Stream for IBStream implementation (using malloc).
\ingroup sdkBase
*/
class MemoryStream : public IBStream
{
public:
	//------------------------------------------------------------------------
	MemoryStream ();
	MemoryStream (void* memory, TSize memorySize); 	///< reuse a given memory without getting ownership
	virtual ~MemoryStream ();

	//---IBStream---------------------------------------
	tresult PLUGIN_API read  (void* buffer, int32 numBytes, int32* numBytesRead) SMTG_OVERRIDE;
	tresult PLUGIN_API write (void* buffer, int32 numBytes, int32* numBytesWritten) SMTG_OVERRIDE;
	tresult PLUGIN_API seek  (int64 pos, int32 mode, int64* result) SMTG_OVERRIDE;
	tresult PLUGIN_API tell  (int64* pos) SMTG_OVERRIDE;

	TSize getSize () const;		///< returns the current memory size
	void setSize (TSize size);	///< set the memory size, a realloc will occur if memory already used
	char* getData () const;		///< returns the memory pointer
	char* detachData ();	///< returns the memory pointer and give up ownership
	bool truncate ();		///< realloc to the current use memory size if needed
	bool truncateToCursor ();	///< truncate memory at current cursor position

	//------------------------------------------------------------------------
	DECLARE_FUNKNOWN_METHODS
protected:
	char* memory;				// memory block
	TSize memorySize;			// size of the memory block
	TSize size;					// size of the stream
	int64 cursor;				// stream pointer
	bool ownMemory;				// stream has allocated memory itself
	bool allocationError;       // stream invalid
};

} // namespace
