#include "device-enum.h"
#include "../dstr.h"
#include "../platform.h"

#include <dxgi.h>
#include <mmdeviceapi.h>

void enum_graphics_device_luids(device_luid_cb device_luid, void *param)
{
	IDXGIFactory1 *factory;
	IDXGIAdapter1 *adapter;
	HRESULT hr;

	hr = CreateDXGIFactory1(&IID_IDXGIFactory1, (void **)&factory);
	if (FAILED(hr))
		return;

	for (UINT i = 0; factory->lpVtbl->EnumAdapters1(factory, i, &adapter) == S_OK; i++) {
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

char *get_audio_device_id(IMMDevice *device)
{
	static const PROPERTYKEY stable_id_key = {
		{0x1da5d803, 0xd492, 0x4edd, {0x8c, 0x23, 0xe0, 0xc0, 0xff, 0xee, 0x7f, 0x0e}},
		12};
	IPropertyStore *store = NULL;
	PROPVARIANT value;
	char *id = NULL;
	WCHAR *endpoint_id = NULL;
	HRESULT hr;

	if (SUCCEEDED(device->lpVtbl->OpenPropertyStore(device, STGM_READ, &store))) {
		PropVariantInit(&value);
		hr = store->lpVtbl->GetValue(store, &stable_id_key, &value);
		if (SUCCEEDED(hr) && value.vt == VT_LPWSTR && value.pwszVal && *value.pwszVal)
			os_wcs_to_utf8_ptr(value.pwszVal, 0, &id);
		PropVariantClear(&value);
		store->lpVtbl->Release(store);
	}

	if (!id && SUCCEEDED(device->lpVtbl->GetId(device, &endpoint_id))) {
		os_wcs_to_utf8_ptr(endpoint_id, 0, &id);
		CoTaskMemFree(endpoint_id);
	}

	return id;
}

char *get_audio_device_id_from_id(const char *id)
{
	IMMDeviceEnumerator *enumerator = NULL;
	IMMDevice *device = NULL;
	WCHAR *wide_id = NULL;
	char *resolved_id = NULL;
	HRESULT hr;

	if (!id || !*id || strcmp(id, "default") == 0)
		return NULL;

	hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator,
			      (void **)&enumerator);
	if (FAILED(hr))
		return NULL;

	os_utf8_to_wcs_ptr(id, 0, &wide_id);
	if (SUCCEEDED(enumerator->lpVtbl->GetDevice(enumerator, wide_id, &device))) {
		resolved_id = get_audio_device_id(device);
		device->lpVtbl->Release(device);
	}
	bfree(wide_id);
	enumerator->lpVtbl->Release(enumerator);

	return resolved_id;
}
