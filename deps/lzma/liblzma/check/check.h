///////////////////////////////////////////////////////////////////////////////
//
/// \file       check.h
/// \brief      Internal API to different integrity check functions
//
//  Author:     Lasse Collin
//
//  This file has been put into the public domain.
//  You can do whatever you want with this file.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef LZMA_CHECK_H
#define LZMA_CHECK_H

#include "common.h"


// Index hashing needs the best possible hash function (preferably
// a cryptographic hash) for maximum reliability.
#if defined(HAVE_CHECK_SHA256)
#	define LZMA_CHECK_BEST LZMA_CHECK_SHA256
#elif defined(HAVE_CHECK_CRC64)
#	define LZMA_CHECK_BEST LZMA_CHECK_CRC64
#else
#	define LZMA_CHECK_BEST LZMA_CHECK_CRC32
#endif


/// \brief      Structure to hold internal state of the check being calculated
///
/// \note       This is not in the public API because this structure may
///             change in future if new integrity check algorithms are added.
typedef struct {
	/// Buffer to hold the final result and a temporary buffer for SHA256.
	union {
		uint8_t u8[64];
		uint32_t u32[16];
		uint64_t u64[8];
	} buffer;

	/// Check-specific data
	union {
		uint32_t crc32;
		uint64_t crc64;

		struct {
			/// Internal state
			uint32_t state[8];

			/// Size of the message excluding padding
			uint64_t size;
		} sha256;
	} state;

} lzma_check_state;


/// lzma_crc32_table[0] is needed by LZ encoder so we need to keep
/// the array two-dimensional.
#ifdef HAVE_SMALL
extern uint32_t lzma_crc32_table[1][256];
extern void lzma_crc32_init(void);
#else
extern const uint32_t lzma_crc32_table[8][256];
extern const uint64_t lzma_crc64_table[4][256];
#endif


/// \brief      Initialize *check depending on type
///
/// \return     LZMA_OK on success. LZMA_UNSUPPORTED_CHECK if the type is not
///             supported by the current version or build of liblzma.
///             LZMA_PROG_ERROR if type > LZMA_CHECK_ID_MAX.
extern void lzma_check_init(lzma_check_state *check, lzma_check type);

/// Update the check state
extern void lzma_check_update(lzma_check_state *check, lzma_check type,
		const uint8_t *buf, size_t size);

/// Finish the check calculation and store the result to check->buffer.u8.
extern void lzma_check_finish(lzma_check_state *check, lzma_check type);


/// Prepare SHA-256 state for new input.
extern void lzma_sha256_init(lzma_check_state *check);

/// Update the SHA-256 hash state
extern void lzma_sha256_update(
		const uint8_t *buf, size_t size, lzma_check_state *check);

/// Finish the SHA-256 calculation and store the result to check->buffer.u8.
extern void lzma_sha256_finish(lzma_check_state *check);

#endif
