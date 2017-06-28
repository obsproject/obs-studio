#include <util/base.h>

#include "metal-subsystem.hpp"

void gs_texture_2d::GenerateMipmap()
{
	assert(device->commandBuffer == nil);

	if (levels == 1)
		return;

	@autoreleasepool {
		id<MTLCommandBuffer> buf = [device->commandQueue commandBuffer];
		id<MTLBlitCommandEncoder> blit = [buf blitCommandEncoder];
		[blit generateMipmapsForTexture:texture];
		[blit endEncoding];
		[buf commit];
		[buf waitUntilCompleted];
	}
}

void gs_texture_2d::BackupTexture(const uint8_t **data)
{
	this->data.resize(levels);

	uint32_t w = width;
	uint32_t h = height;
	uint32_t bpp = gs_get_format_bpp(format);

	for (uint32_t i = 0; i < levels; i++) {
		if (!data[i])
			break;

		uint32_t texSize = bpp * w * h / 8;
		this->data[i].resize(texSize);

		auto &subData = this->data[i];
		memcpy(&subData[0], data[i], texSize);

		w /= 2;
		h /= 2;
	}
}

void gs_texture_2d::UploadTexture()
{
	const uint32_t bpp = gs_get_format_bpp(format) / 8;
	uint32_t w = width;
	uint32_t h = height;

	for (uint32_t i = 0; i < levels; i++) {
		if (i >= data.size())
			break;

		const uint32_t rowSizeBytes = w * bpp;
		const uint32_t texSizeBytes = h * rowSizeBytes;
		MTLRegion region = MTLRegionMake2D(0, 0, w, h);
		[texture replaceRegion:region mipmapLevel:i slice:0
				withBytes:data[i].data()
				bytesPerRow:rowSizeBytes
				bytesPerImage:texSizeBytes];

		w /= 2;
		h /= 2;
	}
}

void gs_texture_2d::InitTexture()
{
	assert(!isShared);

	texture = [device->device newTextureWithDescriptor:textureDesc];
	if (texture == nil)
		throw "Failed to create 2D texture";
}

void gs_texture_2d::Rebuild()
{
	if (isShared) {
		texture = nil;
		return;
	}

	InitTexture();
}

gs_texture_2d::gs_texture_2d(gs_device_t *device, uint32_t width,
		uint32_t height, gs_color_format colorFormat, uint32_t levels,
		const uint8_t **data, uint32_t flags, gs_texture_type type)
	: gs_texture      (device, gs_type::gs_texture_2d, type, levels,
	                   colorFormat),
	  width           (width),
	  height          (height),
	  bytePerRow      (width * gs_get_format_bpp(colorFormat) / 8),
	  isRenderTarget  ((flags & GS_RENDER_TARGET) != 0),
	  isDynamic       ((flags & GS_DYNAMIC) != 0),
	  genMipmaps      ((flags & GS_BUILD_MIPMAPS) != 0),
	  isShared        (false),
	  mtlPixelFormat  (ConvertGSTextureFormat(format))
{
	if (type == GS_TEXTURE_CUBE) {
		NSUInteger size = 6 * width * height;
		textureDesc = [MTLTextureDescriptor
				textureCubeDescriptorWithPixelFormat:
				mtlPixelFormat size:size
				mipmapped:genMipmaps ? YES : NO];
	} else {
		textureDesc = [MTLTextureDescriptor
				texture2DDescriptorWithPixelFormat:
				mtlPixelFormat width:width height:height
				mipmapped:genMipmaps ? YES : NO];
	}

	switch (type) {
	case GS_TEXTURE_3D:
		textureDesc.textureType = MTLTextureType3D;
		break;
	case GS_TEXTURE_CUBE:
		textureDesc.textureType = MTLTextureTypeCube;
		break;
	case GS_TEXTURE_2D:
	default:
		break;
	}
	if (genMipmaps)
		textureDesc.mipmapLevelCount = levels;
	textureDesc.arrayLength              = type == GS_TEXTURE_CUBE ? 6 : 1;
	textureDesc.cpuCacheMode             = MTLCPUCacheModeWriteCombined;
	textureDesc.storageMode              = MTLStorageModeManaged;
	textureDesc.usage                    = MTLTextureUsageShaderRead;
	if (isRenderTarget)
		textureDesc.usage |= MTLTextureUsageRenderTarget;

	InitTexture();

	if (data) {
		BackupTexture(data);
		UploadTexture();
		if (genMipmaps)
			GenerateMipmap();
	}
}

gs_texture_2d::gs_texture_2d(gs_device_t *device, id<MTLTexture> texture)
	: gs_texture      (device, gs_type::gs_texture_2d,
			   GS_TEXTURE_2D,
			   texture.mipmapLevelCount,
			   ConvertMTLTextureFormat(texture.pixelFormat)),
	  width           (texture.width),
	  height          (texture.height),
	  bytePerRow      (width * gs_get_format_bpp(format) / 8),
	  isRenderTarget  (false),
	  isDynamic       (false),
	  genMipmaps      (false),
	  isShared        (true),
	  mtlPixelFormat  (texture.pixelFormat),
	  texture         (texture)
{
}
