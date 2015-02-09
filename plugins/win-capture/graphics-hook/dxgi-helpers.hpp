#pragma once

static inline DXGI_FORMAT fix_dxgi_format(DXGI_FORMAT format)
{
	switch ((unsigned long)format) {
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			return DXGI_FORMAT_B8G8R8A8_UNORM;
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
	}

	return format;
}

