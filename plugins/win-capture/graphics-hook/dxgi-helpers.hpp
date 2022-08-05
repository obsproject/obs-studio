#pragma once

#include "graphics-hook.h"

static inline DXGI_FORMAT strip_dxgi_format_srgb(DXGI_FORMAT format)
{
	switch (format) {
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		return DXGI_FORMAT_B8G8R8A8_UNORM;
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	}

	return format;
}

static inline DXGI_FORMAT apply_dxgi_format_typeless(DXGI_FORMAT format,
						     bool allow_srgb_alias)
{
	if (allow_srgb_alias) {
		switch (format) {
		case DXGI_FORMAT_B8G8R8A8_UNORM:
			return DXGI_FORMAT_B8G8R8A8_TYPELESS;
		case DXGI_FORMAT_R8G8B8A8_UNORM:
			return DXGI_FORMAT_R8G8B8A8_TYPELESS;
		}
	}

	return format;
}

static void print_swap_desc(const DXGI_SWAP_CHAIN_DESC *desc)
{
	hlog("DXGI_SWAP_CHAIN_DESC:\n"
	     "    BufferDesc.Width: %u\n"
	     "    BufferDesc.Height: %u\n"
	     "    BufferDesc.RefreshRate.Numerator: %u\n"
	     "    BufferDesc.RefreshRate.Denominator: %u\n"
	     "    BufferDesc.Format: %u\n"
	     "    BufferDesc.ScanlineOrdering: %u\n"
	     "    BufferDesc.Scaling: %u\n"
	     "    SampleDesc.Count: %u\n"
	     "    SampleDesc.Quality: %u\n"
	     "    BufferUsage: %u\n"
	     "    BufferCount: %u\n"
	     "    Windowed: %u\n"
	     "    SwapEffect: %u\n"
	     "    Flags: %u",
	     desc->BufferDesc.Width, desc->BufferDesc.Height,
	     desc->BufferDesc.RefreshRate.Numerator,
	     desc->BufferDesc.RefreshRate.Denominator, desc->BufferDesc.Format,
	     desc->BufferDesc.ScanlineOrdering, desc->BufferDesc.Scaling,
	     desc->SampleDesc.Count, desc->SampleDesc.Quality,
	     desc->BufferUsage, desc->BufferCount, desc->Windowed,
	     desc->SwapEffect, desc->Flags);
}
