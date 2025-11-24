//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 
//

#pragma once

#include <DirectXMath.h>
#include <intrin.h>

#define INLINE __forceinline

namespace Math
{
    template <typename T> __forceinline T AlignUpWithMask( T value, size_t mask )
    {
        return (T)(((size_t)value + mask) & ~mask);
    }

    template <typename T> __forceinline T AlignDownWithMask( T value, size_t mask )
    {
        return (T)((size_t)value & ~mask);
    }

    template <typename T> __forceinline T AlignUp( T value, size_t alignment )
    {
        return AlignUpWithMask(value, alignment - 1);
    }

    template <typename T> __forceinline T AlignDown( T value, size_t alignment )
    {
        return AlignDownWithMask(value, alignment - 1);
    }

    template <typename T> __forceinline bool IsAligned( T value, size_t alignment )
    {
        return 0 == ((size_t)value & (alignment - 1));
    }

    template <typename T> __forceinline T DivideByMultiple( T value, size_t alignment )
    {
        return (T)((value + alignment - 1) / alignment);
    }

    template <typename T> __forceinline bool IsPowerOfTwo(T value)
    {
        return 0 == (value & (value - 1));
    }

    template <typename T> __forceinline bool IsDivisible(T value, T divisor)
    {
        return (value / divisor) * divisor == value;
    }

    __forceinline uint8_t Log2(uint64_t value)
    {
        unsigned long mssb; // most significant set bit
        unsigned long lssb; // least significant set bit

        // If perfect power of two (only one set bit), return index of bit.  Otherwise round up
        // fractional log by adding 1 to most signicant set bit's index.
        if (_BitScanReverse64(&mssb, value) > 0 && _BitScanForward64(&lssb, value) > 0)
            return uint8_t(mssb + (mssb == lssb ? 0 : 1));
        else
            return 0;
    }

    template <typename T> __forceinline T AlignPowerOfTwo(T value)
    {
        return value == 0 ? 0 : 1 << Log2(value);
    }

    using namespace DirectX;

    INLINE XMVECTOR SplatZero()
    {
        return XMVectorZero();
    }

#if !defined(_XM_NO_INTRINSICS_) && defined(_XM_SSE_INTRINSICS_)

    INLINE XMVECTOR SplatOne( XMVECTOR zero = SplatZero() )
    {
        __m128i AllBits = _mm_castps_si128(_mm_cmpeq_ps(zero, zero));
        return _mm_castsi128_ps(_mm_slli_epi32(_mm_srli_epi32(AllBits, 25), 23));	// return 0x3F800000
        //return _mm_cvtepi32_ps(_mm_srli_epi32(SetAllBits(zero), 31));				// return (float)1;  (alternate method)
    }

#if defined(_XM_SSE4_INTRINSICS_)
    INLINE XMVECTOR CreateXUnitVector( XMVECTOR one = SplatOne() )
    {
        return _mm_insert_ps(one, one, 0x0E);
    }
    INLINE XMVECTOR CreateYUnitVector( XMVECTOR one = SplatOne() )
    {
        return _mm_insert_ps(one, one, 0x0D);
    }
    INLINE XMVECTOR CreateZUnitVector( XMVECTOR one = SplatOne() )
    {
        return _mm_insert_ps(one, one, 0x0B);
    }
    INLINE XMVECTOR CreateWUnitVector( XMVECTOR one = SplatOne() )
    {
        return _mm_insert_ps(one, one, 0x07);
    }
    INLINE XMVECTOR SetWToZero( FXMVECTOR vec )
    {
        return _mm_insert_ps(vec, vec, 0x08);
    }
    INLINE XMVECTOR SetWToOne( FXMVECTOR vec )
    {
        return _mm_blend_ps(vec, SplatOne(), 0x8);
    }
#else
    INLINE XMVECTOR CreateXUnitVector( XMVECTOR one = SplatOne() )
    {
        return _mm_castsi128_ps(_mm_srli_si128(_mm_castps_si128(one), 12));
    }
    INLINE XMVECTOR CreateYUnitVector( XMVECTOR one = SplatOne() )
    {
        XMVECTOR unitx = CreateXUnitVector(one);
        return _mm_castsi128_ps(_mm_slli_si128(_mm_castps_si128(unitx), 4));
    }
    INLINE XMVECTOR CreateZUnitVector( XMVECTOR one = SplatOne() )
    {
        XMVECTOR unitx = CreateXUnitVector(one);
        return _mm_castsi128_ps(_mm_slli_si128(_mm_castps_si128(unitx), 8));
    }
    INLINE XMVECTOR CreateWUnitVector( XMVECTOR one = SplatOne() )
    {
        return _mm_castsi128_ps(_mm_slli_si128(_mm_castps_si128(one), 12));
    }
    INLINE XMVECTOR SetWToZero( FXMVECTOR vec )
    {
        __m128i MaskOffW = _mm_srli_si128(_mm_castps_si128(_mm_cmpeq_ps(vec, vec)), 4);
        return _mm_and_ps(vec, _mm_castsi128_ps(MaskOffW));
    }
    INLINE XMVECTOR SetWToOne( FXMVECTOR vec )
    {
        return _mm_movelh_ps(vec, _mm_unpackhi_ps(vec, SplatOne()));
    }
#endif

#else // !_XM_SSE_INTRINSICS_

    INLINE XMVECTOR SplatOne() { return XMVectorSplatOne(); }
    INLINE XMVECTOR CreateXUnitVector() { return g_XMIdentityR0; }
    INLINE XMVECTOR CreateYUnitVector() { return g_XMIdentityR1; }
    INLINE XMVECTOR CreateZUnitVector() { return g_XMIdentityR2; }
    INLINE XMVECTOR CreateWUnitVector() { return g_XMIdentityR3; }
    INLINE XMVECTOR SetWToZero( FXMVECTOR vec ) { return XMVectorAndInt( vec, g_XMMask3 ); }
    INLINE XMVECTOR SetWToOne( FXMVECTOR vec ) { return XMVectorSelect( g_XMIdentityR3, vec, g_XMMask3 ); }

#endif

    enum EZeroTag { kZero, kOrigin };
    enum EIdentityTag { kOne, kIdentity };
    enum EXUnitVector { kXUnitVector };
    enum EYUnitVector { kYUnitVector };
    enum EZUnitVector { kZUnitVector };
    enum EWUnitVector { kWUnitVector };

}

// A faster version of memcopy that uses SSE instructions.  TODO:  Write an ARM variant if necessary.
inline static void SIMDMemCopy(void *__restrict _Dest, const void *__restrict _Source, size_t NumQuadwords)
{
	//ASSERT(Math::IsAligned(_Dest, 16));
	//ASSERT(Math::IsAligned(_Source, 16));

	__m128i *__restrict Dest = (__m128i *__restrict)_Dest;
	const __m128i *__restrict Source = (const __m128i *__restrict)_Source;

	// Discover how many quadwords precede a cache line boundary.  Copy them separately.
	size_t InitialQuadwordCount = (4 - ((size_t)Source >> 4) & 3) & 3;
	if (InitialQuadwordCount > NumQuadwords)
		InitialQuadwordCount = NumQuadwords;

	switch (InitialQuadwordCount) {
	case 3:
		_mm_stream_si128(Dest + 2, _mm_load_si128(Source + 2)); // Fall through
	case 2:
		_mm_stream_si128(Dest + 1, _mm_load_si128(Source + 1)); // Fall through
	case 1:
		_mm_stream_si128(Dest + 0, _mm_load_si128(Source + 0)); // Fall through
	default:
		break;
	}

	if (NumQuadwords == InitialQuadwordCount)
		return;

	Dest += InitialQuadwordCount;
	Source += InitialQuadwordCount;
	NumQuadwords -= InitialQuadwordCount;

	size_t CacheLines = NumQuadwords >> 2;

	switch (CacheLines) {
	default:
	case 10:
		_mm_prefetch((char *)(Source + 36), _MM_HINT_NTA); // Fall through
	case 9:
		_mm_prefetch((char *)(Source + 32), _MM_HINT_NTA); // Fall through
	case 8:
		_mm_prefetch((char *)(Source + 28), _MM_HINT_NTA); // Fall through
	case 7:
		_mm_prefetch((char *)(Source + 24), _MM_HINT_NTA); // Fall through
	case 6:
		_mm_prefetch((char *)(Source + 20), _MM_HINT_NTA); // Fall through
	case 5:
		_mm_prefetch((char *)(Source + 16), _MM_HINT_NTA); // Fall through
	case 4:
		_mm_prefetch((char *)(Source + 12), _MM_HINT_NTA); // Fall through
	case 3:
		_mm_prefetch((char *)(Source + 8), _MM_HINT_NTA); // Fall through
	case 2:
		_mm_prefetch((char *)(Source + 4), _MM_HINT_NTA); // Fall through
	case 1:
		_mm_prefetch((char *)(Source + 0), _MM_HINT_NTA); // Fall through

		// Do four quadwords per loop to minimize stalls.
		for (size_t i = CacheLines; i > 0; --i) {
			// If this is a large copy, start prefetching future cache lines.  This also prefetches the
			// trailing quadwords that are not part of a whole cache line.
			if (i >= 10)
				_mm_prefetch((char *)(Source + 40), _MM_HINT_NTA);

			_mm_stream_si128(Dest + 0, _mm_load_si128(Source + 0));
			_mm_stream_si128(Dest + 1, _mm_load_si128(Source + 1));
			_mm_stream_si128(Dest + 2, _mm_load_si128(Source + 2));
			_mm_stream_si128(Dest + 3, _mm_load_si128(Source + 3));

			Dest += 4;
			Source += 4;
		}

	case 0: // No whole cache lines to read
		break;
	}

	// Copy the remaining quadwords
	switch (NumQuadwords & 3) {
	case 3:
		_mm_stream_si128(Dest + 2, _mm_load_si128(Source + 2)); // Fall through
	case 2:
		_mm_stream_si128(Dest + 1, _mm_load_si128(Source + 1)); // Fall through
	case 1:
		_mm_stream_si128(Dest + 0, _mm_load_si128(Source + 0)); // Fall through
	default:
		break;
	}

	_mm_sfence();
}


inline static void SIMDMemFill(void *__restrict _Dest, __m128 FillVector, size_t NumQuadwords)
{
	// ASSERT(Math::IsAligned(_Dest, 16));

	const __m128i Source = _mm_castps_si128(FillVector);
	__m128i *__restrict Dest = (__m128i *__restrict)_Dest;

	switch (((size_t)Dest >> 4) & 3) {
	case 1:
		_mm_stream_si128(Dest++, Source);
		--NumQuadwords; // Fall through
	case 2:
		_mm_stream_si128(Dest++, Source);
		--NumQuadwords; // Fall through
	case 3:
		_mm_stream_si128(Dest++, Source);
		--NumQuadwords; // Fall through
	default:
		break;
	}

	size_t WholeCacheLines = NumQuadwords >> 2;

	// Do four quadwords per loop to minimize stalls.
	while (WholeCacheLines--) {
		_mm_stream_si128(Dest++, Source);
		_mm_stream_si128(Dest++, Source);
		_mm_stream_si128(Dest++, Source);
		_mm_stream_si128(Dest++, Source);
	}

	// Copy the remaining quadwords
	switch (NumQuadwords & 3) {
	case 3:
		_mm_stream_si128(Dest++, Source); // Fall through
	case 2:
		_mm_stream_si128(Dest++, Source); // Fall through
	case 1:
		_mm_stream_si128(Dest++, Source); // Fall through
	default:
		break;
	}

	_mm_sfence();
}
