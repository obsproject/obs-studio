#include "device-enum.h"
#include "../dstr.h"

#include <dxgi.h>

void enum_graphics_device_luids(device_luid_cb device_luid, void *param)
{
	IDXGIFactory1 *factory;
	IDXGIAdapter1 *adapter;
	HRESULT hr;

	hr = CreateDXGIFactory1(&IID_IDXGIFactory1, (void **)&factory);
	if (FAILED(hr))
		return;

	for (UINT i = 0;
	     factory->lpVtbl->EnumAdapters1(factory, i, &adapter) == S_OK;
	     i++) {
		DXGI_ADAPTER_DESC desc;

		hr = adapter->lpVtbl->GetDesc(adapter, &desc);
		adapter->lpVtbl->Release(adapter);
		if (FAILED(hr))
			continue;

		uint64_t luid64 = *(uint64_t *)&desc.AdapterLuid;
		if (!device_luid(param, i, luid64))
			break;
	}

	factory->lpVtbl->Release(factory);
}
