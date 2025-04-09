#include "d3d12-subsystem.hpp"

gs_zstencil_buffer::gs_zstencil_buffer(gs_device_t *device, uint32_t width, uint32_t height, gs_zstencil_format format)
	: gs_obj(device, gs_type::gs_zstencil_buffer),
	  DepthBuffer(device->d3d12Instance),
	  format(format)
{
	Create(L"zstencil buffer", width, height, ConvertGSZStencilFormat(format));
}
