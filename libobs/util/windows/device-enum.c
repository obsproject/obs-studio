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

bool get_audio_device_ids(const char *id, const char *fallback_id, char **device_id, char **stable_id)
{
	static const PROPERTYKEY stable_id_key = {
		{0x1da5d803, 0xd492, 0x4edd, {0x8c, 0x23, 0xe0, 0xc0, 0xff, 0xee, 0x7f, 0x0e}},
		12};
	IMMDeviceEnumerator *enumerator = NULL;
	IMMDevice *device = NULL;
	IPropertyStore *store = NULL;
	PROPVARIANT value;
	WCHAR *wide_id = NULL;
	WCHAR *endpoint_id = NULL;
	HRESULT hr;

	*device_id = NULL;
	*stable_id = NULL;
	if (!id || !*id)
		return false;

	if (strcmp(id, "default") == 0) {
		*device_id = bstrdup(id);
		return true;
	}

	hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator,
			      (void **)&enumerator);
	if (FAILED(hr))
		return false;

	os_utf8_to_wcs_ptr(id, 0, &wide_id);
	hr = enumerator->lpVtbl->GetDevice(enumerator, wide_id, &device);
	bfree(wide_id);
	if (FAILED(hr) && fallback_id && *fallback_id) {
		os_utf8_to_wcs_ptr(fallback_id, 0, &wide_id);
		hr = enumerator->lpVtbl->GetDevice(enumerator, wide_id, &device);
		bfree(wide_id);
	}
	enumerator->lpVtbl->Release(enumerator);
	if (FAILED(hr))
		return false;

	if (SUCCEEDED(device->lpVtbl->GetId(device, &endpoint_id))) {
		os_wcs_to_utf8_ptr(endpoint_id, 0, device_id);
		CoTaskMemFree(endpoint_id);
	}
	if (SUCCEEDED(device->lpVtbl->OpenPropertyStore(device, STGM_READ, &store))) {
		PropVariantInit(&value);
		hr = store->lpVtbl->GetValue(store, &stable_id_key, &value);
		if (SUCCEEDED(hr) && value.vt == VT_LPWSTR && value.pwszVal && *value.pwszVal)
			os_wcs_to_utf8_ptr(value.pwszVal, 0, stable_id);

		PropVariantClear(&value);
		store->lpVtbl->Release(store);
	}
	device->lpVtbl->Release(device);

	return *device_id != NULL;
}
