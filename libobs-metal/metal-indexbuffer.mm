#include "metal-subsystem.hpp"

static inline MTLIndexType ConvertGSIndexType(gs_index_type type)
{
	switch (type) {
		case GS_UNSIGNED_SHORT: return MTLIndexTypeUInt16;
		case GS_UNSIGNED_LONG:  return MTLIndexTypeUInt32;
	}
	
	throw "Failed to initialize index buffer";
}

static inline size_t ConvertGSIndexTypeToSize(gs_index_type type)
{
	switch (type) {
		case GS_UNSIGNED_SHORT: return 2;
		case GS_UNSIGNED_LONG:  return 4;
	}
	
	throw "Failed to initialize index buffer";
}

void gs_index_buffer::PrepareBuffer(void *new_indices)
{
	assert(isDynamic);
	
	indexBuffer = device->GetBuffer(new_indices, len);
#if _DEBUG
	indexBuffer.label = @"index";
#endif
}

void gs_index_buffer::InitBuffer()
{
	NSUInteger         length  = len;
	MTLResourceOptions options = MTLResourceCPUCacheModeWriteCombined |
			MTLResourceStorageModeShared;
	
	indexBuffer = [device->device newBufferWithBytes:&indices
			length:length options:options];
	if (indexBuffer == nil)
		throw "Failed to create index buffer";
	
#ifdef _DEBUG
	indexBuffer.label = @"index";
#endif
}

void gs_index_buffer::Rebuild()
{
	if (!isDynamic)
		InitBuffer();
}

gs_index_buffer::gs_index_buffer(gs_device_t *device, enum gs_index_type type,
		void *indices, size_t num, uint32_t flags)
	: gs_obj    (device, gs_type::gs_index_buffer),
	  type      (type),
	  isDynamic ((flags & GS_DYNAMIC) != 0),
	  indices   (indices, bfree),
	  num       (num),
	  len       (ConvertGSIndexTypeToSize(type) * num),
	  indexType (ConvertGSIndexType(type))
{
	if (!isDynamic)
		InitBuffer();
}
