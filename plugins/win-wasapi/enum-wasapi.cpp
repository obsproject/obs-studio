#include "enum-wasapi.hpp"

#include <util/base.h>
#include <util/bmem.h>
#include <util/platform.h>
#include <util/windows/HRError.hpp>
#include <util/windows/ComPtr.hpp>
#include <util/windows/device-enum.h>

using namespace std;

string GetDeviceName(IMMDevice *device)
{
	string device_name;
	ComPtr<IPropertyStore> store;
	HRESULT res;

	if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, store.Assign()))) {
		PROPVARIANT nameVar;

		PropVariantInit(&nameVar);
		res = store->GetValue(PKEY_Device_FriendlyName, &nameVar);

		if (SUCCEEDED(res) && nameVar.pwszVal && *nameVar.pwszVal) {
			size_t len = wcslen(nameVar.pwszVal);
			size_t size;

			size = os_wcs_to_utf8(nameVar.pwszVal, len, nullptr, 0) + 1;
			device_name.resize(size);
			os_wcs_to_utf8(nameVar.pwszVal, len, &device_name[0], size);
			PropVariantClear(&nameVar);
		}
	}

	return device_name;
}

static void GetWASAPIAudioDevices_(vector<AudioDeviceInfo> &devices, bool input)
{
	ComPtr<IMMDeviceEnumerator> enumerator;
	ComPtr<IMMDeviceCollection> collection;
	UINT count;
	HRESULT res;

	res = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
			       (void **)enumerator.Assign());
	if (FAILED(res)) {
		throw HRError("Failed to create enumerator", res);
	}

	res = enumerator->EnumAudioEndpoints(input ? eCapture : eRender, DEVICE_STATE_ACTIVE, collection.Assign());
	if (FAILED(res)) {
		throw HRError("Failed to enumerate devices", res);
	}

	res = collection->GetCount(&count);
	if (FAILED(res)) {
		throw HRError("Failed to get device count", res);
	}

	for (UINT i = 0; i < count; i++) {
		ComPtr<IMMDevice> device;
		AudioDeviceInfo info;

		res = collection->Item(i, device.Assign());
		if (FAILED(res)) {
			continue;
		}

		char *id = get_audio_device_id(device);
		if (!id) {
			continue;
		}

		info.name = GetDeviceName(device);

		info.id = id;
		bfree(id);

		devices.push_back(info);
	}
}

void GetWASAPIAudioDevices(vector<AudioDeviceInfo> &devices, bool input)
{
	devices.clear();

	try {
		GetWASAPIAudioDevices_(devices, input);

	} catch (HRError &error) {
		blog(LOG_WARNING, "[GetWASAPIAudioDevices] %s: %lX", error.str, error.hr);
	}
}
