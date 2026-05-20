#include "d3d12-subsystem.hpp"

gs_zstencil_buffer::gs_zstencil_buffer(gs_device_t *device, uint32_t width_, uint32_t height_,
				       gs_zstencil_format format_)
	: gs_obj(device, gs_type::gs_zstencil_buffer),
	  DepthBuffer(device->d3d12Instance),
	  width(width_),
	  height(height_),
	  format(format_)
{
	Create(L"zstencil buffer", width, height, ConvertGSZStencilFormat(format));
}
