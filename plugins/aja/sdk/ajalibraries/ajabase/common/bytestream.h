/* SPDX-License-Identifier: MIT */
/**
	@file		bytestream.h
	@brief		Declaration of AJAByteStream class.
	@copyright	(C) 2012-2021 AJA Video Systems, Inc.  All rights reserved.
**/
 
#ifndef AJA_BYTESTREAM_H
#define AJA_BYTESTREAM_H

#include <stddef.h>
#include <string.h>
#include "ajabase/common/types.h"

class AJAByteStream {
	public:
		AJAByteStream(void *buf = NULL, size_t pos = 0);

		size_t Pos() const;
		void *Buffer() const;
		
		void Reset();
		void Seek(size_t val); 		// Seeks to an absolute position
		void SeekFwd(size_t val); 	// Seeks forward n bytes
		void SeekRev(size_t val); 	// Seeks backwards n bytes

		void Set(uint8_t val, size_t bytes);
		void Write(const void *buf, size_t bytes);
		void Write8(uint8_t val);
		void Write16LE(uint16_t val);
		void Write16BE(uint16_t val);
		void Write32LE(uint32_t val);
		void Write32BE(uint32_t val);
		void Write64LE(uint64_t val);
		void Write64BE(uint64_t val);

		void Read(void *buf, size_t LEn);
		uint8_t Read8();
		uint16_t Read16LE();
		uint16_t Read16BE();
		uint32_t Read32LE();
		uint32_t Read32BE();
		uint64_t Read64LE();
		uint64_t Read64BE();

	private:
		uint8_t 	*b;	// Beginning buffer position
		uint8_t		*c;	// Current buffer position
};

inline AJAByteStream::AJAByteStream(void *buf, size_t pos) : b((uint8_t *)buf) {
	Seek(pos);
}

inline size_t AJAByteStream::Pos() const {
	return (size_t)(c - b);
}

inline void *AJAByteStream::Buffer() const {
	return b;
}

inline void AJAByteStream::Reset() {
	c = b;
	return;
}

inline void AJAByteStream::Seek(size_t val) {
	c = b + val;
	return;
}

inline void AJAByteStream::SeekFwd(size_t val) {
	c += val;
	return;
}

inline void AJAByteStream::SeekRev(size_t val) {
	c -= val;
	return;
}

inline void AJAByteStream::Set(uint8_t val, size_t bytes) {
	uint8_t *last = c + bytes;
	while(c != last) *c++ = val;
	return;
}

inline void AJAByteStream::Write(const void *buf, size_t bytes) {
	memcpy(c, buf, bytes);
	c += bytes;
	return;
}

inline void AJAByteStream::Write8(uint8_t val) {
	*c++ = val;
	return;
}

inline void AJAByteStream::Write16LE(uint16_t val) {
	c[0] = val & 0xFF;
	c[1] = (val >> 8) & 0xFF;
	c += 2;
	return;
}

inline void AJAByteStream::Write16BE(uint16_t val) {
	c[0] = (val >> 8) & 0xFF;
	c[1] = val & 0xFF;
	c += 2;
	return;
}

inline void AJAByteStream::Write32LE(uint32_t val) {
	c[0] = val & 0xFF;
	c[1] = (val >> 8) & 0xFF;
	c[2] = (val >> 16) & 0xFF;
	c[3] = (val >> 24) & 0xFF;
	c += 4;
	return;
}

inline void AJAByteStream::Write32BE(uint32_t val) {
	c[0] = (val >> 24) & 0xFF;
	c[1] = (val >> 16) & 0xFF;
	c[2] = (val >> 8) & 0xFF;
	c[3] = val & 0xFF;
	c += 4;
	return;
}

inline void AJAByteStream::Write64LE(uint64_t val) {
	c[0] = val & 0xFF;
	c[1] = (val >> 8) & 0xFF;
	c[2] = (val >> 16) & 0xFF;
	c[3] = (val >> 24) & 0xFF;
	c[4] = (val >> 32) & 0xFF;
	c[5] = (val >> 40) & 0xFF;
	c[6] = (val >> 48) & 0xFF;
	c[7] = (val >> 56) & 0xFF;
	c += 8;
	return;
}

inline void AJAByteStream::Write64BE(uint64_t val) {
	c[0] = (val >> 56) & 0xFF;
	c[1] = (val >> 48) & 0xFF;
	c[2] = (val >> 40) & 0xFF;
	c[3] = (val >> 32) & 0xFF;
	c[4] = (val >> 24) & 0xFF;
	c[5] = (val >> 16) & 0xFF;
	c[6] = (val >> 8) & 0xFF;
	c[7] = val & 0xFF;
	c += 8;
	return;
}

inline void AJAByteStream::Read(void *buf, size_t bytes) {
	uint8_t *last = c + bytes, *o = (uint8_t *)buf;
	while(c != last) *o++ = *c++;
	return;
}

inline uint8_t AJAByteStream::Read8() {
	return *c++;
}

inline uint16_t AJAByteStream::Read16LE() {
	uint16_t ret;
	ret = (uint16_t)c[0];
	ret |= (uint16_t)c[1] << 8;
	c += 2;
	return ret;
}

inline uint16_t AJAByteStream::Read16BE() {
	uint16_t ret;
	ret = (uint16_t)c[0] << 8;
	ret |= (uint16_t)c[1];
	c += 2;
	return ret;
}

inline uint32_t AJAByteStream::Read32LE() {
	uint32_t ret;
	ret = (uint32_t)c[0];
	ret |= (uint32_t)c[1] << 8;
	ret |= (uint32_t)c[2] << 16;
	ret |= (uint32_t)c[3] << 24;
	c += 4;
	return ret;
}

inline uint32_t AJAByteStream::Read32BE() {
	uint32_t ret;
	ret = (uint32_t)c[0] << 24;
	ret |= (uint32_t)c[1] << 16;
	ret |= (uint32_t)c[2] << 8;
	ret |= (uint32_t)c[3];
	c += 4;
	return ret;
}

inline uint64_t AJAByteStream::Read64LE() {
	uint64_t ret;
	ret = (uint64_t)c[0];
	ret |= (uint64_t)c[1] << 8;
	ret |= (uint64_t)c[2] << 16;
	ret |= (uint64_t)c[3] << 24;
	ret |= (uint64_t)c[4] << 32;
	ret |= (uint64_t)c[5] << 40;
	ret |= (uint64_t)c[6] << 48;
	ret |= (uint64_t)c[7] << 56;
	c += 8;
	return ret;
}

inline uint64_t AJAByteStream::Read64BE() {
	uint64_t ret;
	ret = (uint64_t)c[0] << 56;
	ret |= (uint64_t)c[1] << 48;
	ret |= (uint64_t)c[2] << 40;
	ret |= (uint64_t)c[3] << 32;
	ret |= (uint64_t)c[4] << 24;
	ret |= (uint64_t)c[5] << 16;
	ret |= (uint64_t)c[6] << 8;
	ret |= (uint64_t)c[7];
	c += 8;
	return ret;
}

#endif /* ifndef AJA_BYTESTREAM_H */
