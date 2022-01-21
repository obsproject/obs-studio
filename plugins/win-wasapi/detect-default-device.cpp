#include "detect-default-device.h"
#include <obs.h>
#include <mutex>

std::mutex lockDetect;
DetectDefaultDevice *detectDevice = nullptr;

void DetectDefaultDevice::Create()
{
	if (!detectDevice) {
		std::lock_guard<std::mutex> auto_lock(lockDetect);
		if (!detectDevice)
			detectDevice = new DetectDefaultDevice();
	}
}

DetectDefaultDevice *DetectDefaultDevice::Instance()
{
	Create();
	return detectDevice;
}

void DetectDefaultDevice::Destroy()
{
	std::lock_guard<std::mutex> auto_lock(lockDetect);
	if (detectDevice) {
		delete detectDevice;
		detectDevice = nullptr;
	}
}

DetectDefaultDevice::DetectDefaultDevice() {}

DetectDefaultDevice::~DetectDefaultDevice()
{
	Stop();
}

void DetectDefaultDevice::Start()
{
	HRESULT res = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
				       CLSCTX_ALL,
				       __uuidof(IMMDeviceEnumerator),
				       (LPVOID *)enumerator.Assign());
	if (SUCCEEDED(res)) {
		notify = new WASAPINotify(this);
		notify->Release(); /* Default ref is 1, but ComPtr will call AddRef() */
		enumerator->RegisterEndpointNotificationCallback(notify);
	} else {
		enumerator.Clear();
		assert(false);
	}
}

void DetectDefaultDevice::Stop()
{
	if (enumerator) {
		enumerator->UnregisterEndpointNotificationCallback(notify);
		enumerator.Clear();
	}

	notify.Clear();
}

void DetectDefaultDevice::NotifyDefaultDeviceChanged(EDataFlow flow, ERole role,
						     LPCWSTR id)
{
	if (flow == eRender && role == eConsole) // output device
	{
		// Post event into UI thread for resetting audio monitor
		QMetaObject::invokeMethod(this, "OnDefaultOutputChanged",
					  Qt::QueuedConnection);
	}

	UNUSED_PARAMETER(id);
}

void DetectDefaultDevice::OnDefaultOutputChanged()
{
	blog(LOG_INFO,
	     "WASAPI: Default monitor device is changed. Try to reset monitor modules.");
	obs_default_output_audio_device_changed();
}
