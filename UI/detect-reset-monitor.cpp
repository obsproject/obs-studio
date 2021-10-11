#ifdef WIN32
#include "detect-reset-monitor.h"
#include <obs.h>

OBSDetectResetAudioMonitor::OBSDetectResetAudioMonitor() {}

OBSDetectResetAudioMonitor::~OBSDetectResetAudioMonitor()
{
	Stop();
}

void OBSDetectResetAudioMonitor::Start()
{
	HRESULT res = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
				       CLSCTX_ALL,
				       __uuidof(IMMDeviceEnumerator),
				       (LPVOID *)&enumerator);
	if (SUCCEEDED(res)) {
		notify = new WASAPINotify(this);
		enumerator->RegisterEndpointNotificationCallback(notify);
	} else {
		enumerator = NULL;
		assert(false);
	}
}

void OBSDetectResetAudioMonitor::Stop()
{
	if (enumerator) {
		enumerator->UnregisterEndpointNotificationCallback(notify);
		enumerator->Release();
		enumerator = NULL;
	}

	if (notify) {
		notify->Release();
		notify = NULL;
	}
}

void OBSDetectResetAudioMonitor::NotifyDefaultDeviceChanged(EDataFlow flow,
							    ERole role,
							    LPCWSTR id)
{
	if (flow == eRender && role == eConsole) // output device
	{
		QMetaObject::invokeMethod(this, "OnDefaultOutputChanged",
					  Qt::QueuedConnection);
	}
}

void OBSDetectResetAudioMonitor::OnDefaultOutputChanged()
{
	obs_default_output_audio_device_changed();
}

//-------------------------------------------------------------------------
#endif // WIN32
