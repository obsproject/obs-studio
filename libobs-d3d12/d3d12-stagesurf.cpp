#include "d3d12-subsystem.hpp"

gs_stage_surface::gs_stage_surface(gs_device_t *device, uint32_t width, uint32_t height, gs_color_format colorFormat)
	: gs_obj(device, gs_type::gs_stage_surface),
	  D3D12Graphics::ReadbackBuffer(device->d3d12Instance),
	  width(width),
	  height(height),
	  format(colorFormat),
	  dxgiFormat(ConvertGSTextureFormatView(colorFormat))
{
	Create(L"surface", (UINT)GetTotalBytes() / D3D12Graphics::BytesPerPixel(dxgiFormat),
	       D3D12Graphics::BytesPerPixel(dxgiFormat));
}

gs_stage_surface::gs_stage_surface(gs_device_t *device, uint32_t width, uint32_t height, bool p010)
	: gs_obj(device, gs_type::gs_stage_surface),
	  D3D12Graphics::ReadbackBuffer(device->d3d12Instance),
	  width(width),
	  height(height),
	  format(GS_UNKNOWN),
	  dxgiFormat(p010 ? DXGI_FORMAT_P010 : DXGI_FORMAT_NV12)
{
	Create(L"surface",
	       (UINT)GetTotalBytes() / D3D12Graphics::BytesPerPixel(p010 ? DXGI_FORMAT_P010 : DXGI_FORMAT_NV12),
	       D3D12Graphics::BytesPerPixel(dxgiFormat));
}

UINT gs_stage_surface::GetLineSize()
{
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedTextureDesc;
	UINT NumRows;
	UINT64 RowLength;
	UINT64 TotalBytes = 0;
	auto textureDesc = GetTextureDesc();
	device->d3d12Instance->GetDevice()->GetCopyableFootprints(&textureDesc, 0, 1, 0, &placedTextureDesc, &NumRows,
								  &RowLength, &TotalBytes);
	return placedTextureDesc.Footprint.RowPitch;
}

D3D12_RESOURCE_DESC gs_stage_surface::GetTextureDesc()
{
	D3D12_RESOURCE_DESC texDesc = {};

	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.DepthOrArraySize = 1;

	texDesc.MipLevels = 1;

	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	texDesc.Format = dxgiFormat;
	if (texDesc.Format == DXGI_FORMAT_NV12 || texDesc.Format == DXGI_FORMAT_P010) {
		texDesc.Width = (texDesc.Width + 1) & ~1;
		texDesc.Height = (texDesc.Height + 1) & ~1;
	}

	return texDesc;
}

UINT64 gs_stage_surface::GetTotalBytes()
{
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedTextureDesc;
	UINT NumRows;
	UINT64 RowLength;
	UINT64 TotalBytes = 0;
	auto textureDesc = GetTextureDesc();
	device->d3d12Instance->GetDevice()->GetCopyableFootprints(&textureDesc, 0, 1, 0, &placedTextureDesc, &NumRows,
								  &RowLength, &TotalBytes);
	return TotalBytes;
}
