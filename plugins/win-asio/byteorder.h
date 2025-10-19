/* Copyright (c) 2022-2025 pkv <pkv@obsproject.com>
 * This file is part of win-asio.
 *
 * This file is a c-adaptation and rewriting of juce_ByteOrder.h from JUCE SDK, which is licensed under the GNU GPL v3.
 * see: https://github.com/juce-framework/JUCE/modules/juce_core/memory/juce_ByteOrder.h
 * 
 * This file and all modifications by pkv <pkv@obsproject.com> are licensed under
 * the GNU General Public License, version 3 or later, to comply with the SDK license.
 */
#ifndef BYTEORDER_H
#define BYTEORDER_H

#include <stdint.h>
#include <windows.h>

// Swap 16-bit and 32-bit using Windows intrinsics
#define ByteOrder_swap16(x) _byteswap_ushort(x)
#define ByteOrder_swap32(x) _byteswap_ulong(x)

// 16-bit reads
static inline int16_t ByteOrder_littleEndianShort(const void *p)
{
	uint16_t v;
	memcpy(&v, p, sizeof(v));
	return (int16_t)v;
}

static inline int16_t ByteOrder_bigEndianShort(const void *p)
{
	uint16_t v;
	memcpy(&v, p, sizeof(v));
	return (int16_t)ByteOrder_swap16(v);
}

// 32-bit reads
static inline int32_t ByteOrder_littleEndianInt(const void *p)
{
	uint32_t v;
	memcpy(&v, p, sizeof(v));
	return (int32_t)v;
}

static inline int32_t ByteOrder_bigEndianInt(const void *p)
{
	uint32_t v;
	memcpy(&v, p, sizeof(v));
	return (int32_t)ByteOrder_swap32(v);
}

// 24-bit reads (signed)
static inline int32_t ByteOrder_littleEndian24Bit(const void *p)
{
	const uint8_t *b = (const uint8_t *)p;
	return (int32_t)((b[2] << 24) | (b[1] << 16) | (b[0] << 8)) >> 8;
}

static inline int32_t ByteOrder_bigEndian24Bit(const void *p)
{
	const uint8_t *b = (const uint8_t *)p;
	return (int32_t)((b[0] << 24) | (b[1] << 16) | (b[2] << 8)) >> 8;
}

// 24-bit write
static inline void ByteOrder_littleEndian24BitToChars(uint32_t val, void *p)
{
	uint8_t *b = (uint8_t *)p;
	b[0] = (uint8_t)(val & 0xFF);
	b[1] = (uint8_t)((val >> 8) & 0xFF);
	b[2] = (uint8_t)((val >> 16) & 0xFF);
}

static inline void ByteOrder_bigEndian24BitToChars(uint32_t val, void *p)
{
	uint8_t *b = (uint8_t *)p;
	b[0] = (uint8_t)((val >> 16) & 0xFF);
	b[1] = (uint8_t)((val >> 8) & 0xFF);
	b[2] = (uint8_t)(val & 0xFF);
}

// Conditional endian swap macros
static inline uint16_t ByteOrder_swapIfBigEndian16(uint16_t v)
{
	return v; // system is little-endian
}

static inline uint16_t ByteOrder_swapIfLittleEndian16(uint16_t v)
{
	return ByteOrder_swap16(v);
}

static inline uint32_t ByteOrder_swapIfBigEndian32(uint32_t v)
{
	return v;
}

static inline uint32_t ByteOrder_swapIfLittleEndian32(uint32_t v)
{
	return ByteOrder_swap32(v);
}

#endif // BYTEORDER_H
