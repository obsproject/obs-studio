#include "metal-subsystem.hpp"

static inline MTLPixelFormat ConvertGSZStencilFormat(gs_zstencil_format format)
{
	switch (format) {
	case GS_ZS_NONE:    return MTLPixelFormatInvalid;
	case GS_Z16:        return MTLPixelFormatDepth16Unorm;
	case GS_Z24_S8:     return MTLPixelFormatDepth24Unorm_Stencil8;
	case GS_Z32F:       return MTLPixelFormatDepth32Float;
	case GS_Z32F_S8X24: return MTLPixelFormatDepth32Float_Stencil8;
	default:            throw "Failed to initialize zstencil buffer";
	}

	return MTLPixelFormatInvalid;
}

void gs_zstencil_buffer::InitBuffer()
{
	texture = [device->device newTextureWithDescriptor:textureDesc];
	if (texture == nil)
		throw "Failed to create depth stencil texture";

#if _DEBUG
	texture.label = @"zstencil";
#endif
}

gs_zstencil_buffer::gs_zstencil_buffer(gs_device_t *device,
		uint32_t width, uint32_t height,
		gs_zstencil_format format)
	: gs_obj (device, gs_type::gs_zstencil_buffer),
	  width  (width),
	  height (height),
	  format (format)
{
	textureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:
			ConvertGSZStencilFormat(format)
			width:width height:height mipmapped:NO];
	textureDesc.cpuCacheMode = MTLCPUCacheModeWriteCombined;
	textureDesc.storageMode  = MTLStorageModeManaged;

	InitBuffer();
}
