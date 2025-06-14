//-------------------------------------------------------------------------------------
// Project     : SDK Base
// Version     : 1.0
//
// Category	   : Helpers
// Filename	   : base/source/hexbinary.h
// Created by  : Steinberg, 1/2012
// Description : HexBinary encoding and decoding
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2024, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
// This Software Development Kit may not be distributed in parts or its entirety
// without prior written agreement by Steinberg Media Technologies GmbH.
// This SDK must not be used to re-engineer or manipulate any technology used
// in any Steinberg or Third-party application or software module,
// unless permitted by law.
// Neither the name of the Steinberg Media Technologies nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// THIS SDK IS PROVIDED BY STEINBERG MEDIA TECHNOLOGIES GMBH "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL STEINBERG MEDIA TECHNOLOGIES GMBH BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//----------------------------------------------------------------------------------

//----------------------------------------------------------------------------------
/** @file base/source/hexbinary.h
 HexBinary encoding and decoding. */
//----------------------------------------------------------------------------------
#pragma once

#include "base/source/fbuffer.h"

namespace Steinberg {

//------------------------------------------------------------------------------
namespace HexBinary
{
	//------------------------------------------------------------------------------
	/** convert the HexBinary input buffer to binary. Note that it is appended to the result buffer. */
	//------------------------------------------------------------------------------
	inline bool decode (const void* input, int32 inputSize, Buffer& result)
	{
		if ((inputSize & 1) == 1)
			return false;

		result.grow (result.getSize () + inputSize / 2);

		const char8* ptr = (const char8*)input;
		uint8 current = 0;
		for (int32 i = 0; i < inputSize; i++, ptr++)
		{
			current *= 16;
			if (*ptr >= 48 && *ptr <= 57) // 0, 1, 2, .., 9
			{
				current += *ptr - 48;
			}
			else if (*ptr >= 65 && *ptr <= 70) // A, B, .., F
			{
				current += *ptr - 55;
			}
			else if (*ptr >= 97 && *ptr <= 102) // a, b, .., f
			{
				current += *ptr - 87;
			}
			else
			{
				// malformed
				return false;
			}
			if (i % 2)
			{
				if (result.put (current) == false)
					return false;
				current = 0;
			}
		}
		return true;
	}

	//------------------------------------------------------------------------------
	/** convert the binary input buffer to HexBinary. Note that it is appended to the result buffer. */
	//------------------------------------------------------------------------------
	inline bool encode (const void* input, int32 inputSize, Buffer& result)
	{
		result.grow (result.getSize () + inputSize * 2);

		const char8* ptr = (const char8*)input;
		for (int32 i = 0; i < inputSize; i++, ptr++)
		{
			char8 high = (*ptr & 0xF0) >> 4;
			char8 low  = (*ptr & 0x0F);
			if (high > 9)
			{
				if (result.put ((char8)(high + 55)) == false)
					return false;
			}
			else
			{
				if (result.put ((char8)(high + 48)) == false)
					return false;
			}
			if (low > 9)
			{
				if (result.put ((char8)(low + 55)) == false)
					return false;
			}
			else
			{
				if (result.put ((char8)(low + 48)) == false)
					return false;
			}
		}
		return true;
	}
//------------------------------------------------------------------------
} // namespace HexBinary
} // namespace Steinberg
