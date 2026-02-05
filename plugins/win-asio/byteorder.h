/******************************************************************************
    Copyright (C) 2022-2026 pkv <pkv@obsproject.com>

    This file is part of win-asio.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
#ifndef BYTEORDER_H
#define BYTEORDER_H

#include <stdint.h>
#include <windows.h>

#define ByteOrder_swap16(x) _byteswap_ushort(x)
#define ByteOrder_swap32(x) _byteswap_ulong(x)

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

static inline uint16_t ByteOrder_swapIfBigEndian16(uint16_t v)
{
	return v;
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
