#pragma once

#include <obs-module.h>
#include <util/darray.h>

#include <va/va.h>

#include "vaapi-display.h"

#define VA_LOG(level, format, ...) \
	blog(level, "[VAAPI encoder]: " format, ##__VA_ARGS__)

#define VA_LOG_STR(level, str) \
	blog(level, "[VAAPI encoder]: %s", str)

#define VA_LOG_STATUS(level, x, status) \
	VA_LOG(LOG_ERROR, "%s: %s", #x, vaErrorStr(status));

#define CHECK_STATUS_(x, y) \
	do { \
		status = (x); \
		if (status != VA_STATUS_SUCCESS) { \
			VA_LOG_STATUS(LOG_ERROR, #x, status); \
			y; \
		} \
	} while (false)

#define CHECK_STATUS_FAIL(x) \
	CHECK_STATUS_(x, goto fail)

#define CHECK_STATUS_FAILN(x, n) \
	CHECK_STATUS_(x, goto fail ## n)

#define CHECK_STATUS_FALSE(x) \
	CHECK_STATUS_(x, return false)

static inline size_t round_up_to_power_of_2(size_t value, size_t alignment)
{
	return ((value + (alignment - 1)) & ~(alignment - 1));
}

typedef struct darray buffer_list_t;

enum vaapi_slice_type
{
	VAAPI_SLICE_TYPE_P,
	VAAPI_SLICE_TYPE_B,
	VAAPI_SLICE_TYPE_I
};
typedef enum vaapi_slice_type vaapi_slice_type_t;

struct coded_block_entry {
	DARRAY(uint8_t) data;
	uint64_t pts;
	vaapi_slice_type_t type;
};
typedef struct coded_block_entry coded_block_entry_t;

bool vaapi_initialize();
void vaapi_shutdown();

size_t vaapi_get_display_cnt();
vaapi_display_t *vaapi_get_display(size_t index);
vaapi_display_t *vaapi_find_display(vaapi_display_type_t type,
		const char *path);

static const char * va_rc_to_str(uint32_t rc)
{
#define RC_CASE(x) case VA_RC_ ## x: return #x
	switch (rc)
	{
	RC_CASE(NONE);
	RC_CASE(CBR);
	RC_CASE(VBR);
	RC_CASE(VCM);
	RC_CASE(CQP);
	RC_CASE(VBR_CONSTRAINED);
	default: return "Invalid RC";
	}
#undef RC_CASE
}
