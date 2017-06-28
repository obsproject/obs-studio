#include "metal-subsystem.hpp"

void gs_stage_surface::DownloadTexture()
{
	MTLRegion from = MTLRegionMake2D(0, 0, width, height);
	[texture getBytes:data.data() bytesPerRow:bytePerRow
		fromRegion:from mipmapLevel:0];
}

void gs_stage_surface::InitTexture()
{
	texture = [device->device newTextureWithDescriptor:textureDesc];
	if (texture == nil)
		throw "Failed to create staging surface";
}

gs_stage_surface::gs_stage_surface(gs_device_t *device, uint32_t width,
		uint32_t height, gs_color_format colorFormat)
	: gs_obj     (device, gs_type::gs_stage_surface),
	  width      (width),
	  height     (height),
	  bytePerRow (width * gs_get_format_bpp(colorFormat) / 8),
	  format     (colorFormat)
{
	textureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:
			ConvertGSTextureFormat(colorFormat)
			width:width height:height mipmapped:NO];
	textureDesc.storageMode = MTLStorageModeManaged;

	data.resize(height * bytePerRow);

	InitTexture();
}
