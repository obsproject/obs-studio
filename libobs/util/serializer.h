/******************************************************************************
  Copyright (c) 2013 by Hugh Bailey <obs.jim@gmail.com>

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

     1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

     2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

     3. This notice may not be removed or altered from any source
     distribution.
******************************************************************************/

#ifndef SERIALIZER_H
#define SERIALIZER_H

/*
 *   General programmable serialization functions.  (A shared interface to
 * various reading/writing to/from different inputs/outputs)
 *
 * TODO: Not currently implemented
 */

#ifdef __cplusplus
extern "C" {
#endif

enum serialize_seek_type {
	SERIALIZE_SEEK_START,
	SERIALIZE_SEEK_CURRENT,
	SERIALIZE_SEEK_END
};

struct serializer {
	void *param;
	size_t (*serialize)(struct serializer, void *, size_t);
	uint64_t (*seek)(struct serializer, int64_t, enum serialize_seek_type);
	uint64_t (*getpos)(struct serializer);
};

static inline size_t serialize(struct serializer *s, void *data, size_t len)
{
	if (s->serialize)
		return s->serialize(s, data, len);

	return 0;
}

static inline uint64_t serializer_seek(struct serializer *s, int64_t offset,
		enum serialize_seek_type seek_type)
{
	if (s->seek)
		return s->seek(s, offset, seek_type);
	return 0;
}

static inline uint64_t serializer_getpos(struct serializer *s)
{
	if (s->getpos)
		return s->getpos(s);
	return 0;
}

static inline void serializer_write_u8(struct serializer *s, uint8_t u8)
{
	serialize(s, &u8, sizeof(uint8_t));
}

static inline void serializer_write_i8(struct serializer *s, int8_t i8)
{
	serialize(s, &i8, sizeof(int8_t));
}

static inline void serializer_write_u16(struct serializer *s, uint16_t u16)
{
	serialize(s, &u16, sizeof(uint16_t));
}

static inline void serializer_write_i16(struct serializer *s, int16_t i16)
{
	serialize(s, &i16, sizeof(int16_t));
}

static inline void serializer_write_u32(struct serializer *s, uint32_t u32)
{
	serialize(s, &u32, sizeof(uint32_t));
}

static inline void serializer_write_i32(struct serializer *s, int32_t i32)
{
	serialize(s, &i32, sizeof(int32_t));
}

static inline void serializer_write_u64(struct serializer *s, uint32_t u64)
{
	serialize(s, &u64, sizeof(uint64_t));
}

static inline void serializer_write_i64(struct serializer *s, int32_t i64)
{
	serialize(s, &i64, sizeof(int64_t));
}

static inline void serializer_write_float(struct serializer *s, float f)
{
	serialize(s, &f, sizeof(float));
}

static inline void serializer_write_double(struct serializer *s, double d)
{
	serialize(s, &d, sizeof(double));
}

#ifdef __cplusplus
}
#endif

#endif
