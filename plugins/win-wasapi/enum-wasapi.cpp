#include "enum-wasapi.hpp"

#include <util/base.h>
#include <util/bmem.h>
#include <util/platform.h>
#include <util/windows/HRError.hpp>
#include <util/windows/ComPtr.hpp>
#include <util/windows/CoTaskMemPtr.hpp>

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

std::string GetWASAPIDefaultDeviceName(bool input)
{
	ComPtr<IMMDeviceEnumerator> enumerator;
	ComPtr<IMMDevice> device;

	HRESULT res = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
				       (void **)enumerator.Assign());
	if (FAILED(res)) {
		return "";
	}

	res = enumerator->GetDefaultAudioEndpoint(input ? eCapture : eRender, input ? eCommunications : eConsole,
						  device.Assign());
	if (FAILED(res)) {
		return "";
	}

	return GetDeviceName(device);
}

static void GetWASAPIAudioDevices_(vector<AudioDeviceInfo> &devices, bool input)
{
	ComPtr<IMMDeviceEnumerator> enumerator;
	ComPtr<IMMDeviceCollection> collection;
	UINT count;
	HRESULT res;

	res = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
			       (void **)enumerator.Assign());
	if (FAILED(res))
		throw HRError("Failed to create enumerator", res);

	res = enumerator->EnumAudioEndpoints(input ? eCapture : eRender, DEVICE_STATE_ACTIVE, collection.Assign());
	if (FAILED(res))
		throw HRError("Failed to enumerate devices", res);

	res = collection->GetCount(&count);
	if (FAILED(res))
		throw HRError("Failed to get device count", res);

	for (UINT i = 0; i < count; i++) {
		ComPtr<IMMDevice> device;
		CoTaskMemPtr<WCHAR> w_id;
		AudioDeviceInfo info;
		size_t len, size;

		res = collection->Item(i, device.Assign());
		if (FAILED(res))
			continue;

		res = device->GetId(&w_id);
		if (FAILED(res) || !w_id || !*w_id)
			continue;

		info.name = GetDeviceName(device);

		len = wcslen(w_id);
		size = os_wcs_to_utf8(w_id, len, nullptr, 0) + 1;
		info.id.resize(size);
		os_wcs_to_utf8(w_id, len, &info.id[0], size);

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

int GetWASAPIDeviceInputChannels(const char *device_id)
{
	ComPtr<IMMDeviceEnumerator> enumerator;
	ComPtr<IMMDevice> device;
	ComPtr<IAudioClient> client;
	CoTaskMemPtr<WAVEFORMATEX> wfex;

	HRESULT res =
		CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(enumerator.Assign()));
	if (FAILED(res)) {
		return 0;
	}

	if (strcmp(device_id, "default") == 0) {
		res = enumerator->GetDefaultAudioEndpoint(eCapture, eCommunications, device.Assign());
	} else {
		wchar_t *w_id = nullptr;
		os_utf8_to_wcs_ptr(device_id, strlen(device_id), &w_id);
		if (!w_id) {
			return 0;
		}
		res = enumerator->GetDevice(w_id, device.Assign());
		bfree(w_id);
	}

	if (FAILED(res) || !device) {
		return 0;
	}

	res = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void **)client.Assign());
	if (FAILED(res)) {
		return 0;
	}

	res = client->GetMixFormat(&wfex);
	if (FAILED(res) || !wfex) {
		return 0;
	}

	return wfex->nChannels;
}
