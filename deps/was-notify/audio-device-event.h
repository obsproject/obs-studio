#pragma once
#ifdef WIN32
#include <Mmdeviceapi.h>

class IWASNotifyCallback {
public:
	virtual ~IWASNotifyCallback() {}
	virtual void NotifyDefaultDeviceChanged(EDataFlow flow, ERole role,
						LPCWSTR id) = 0;
};

class WASAPINotify : public IMMNotificationClient {
	long refs = 1;
	IWASNotifyCallback *cb = NULL;

public:
	WASAPINotify(IWASNotifyCallback *cb_);

	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) STDMETHODCALLTYPE Release();
	STDMETHODIMP QueryInterface(REFIID riid, void **ptr);

	STDMETHODIMP OnDeviceAdded(LPCWSTR);
	STDMETHODIMP OnDeviceRemoved(LPCWSTR);
	STDMETHODIMP OnDeviceStateChanged(LPCWSTR, DWORD);
	STDMETHODIMP OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY);
	STDMETHODIMP OnDefaultDeviceChanged(EDataFlow flow, ERole role,
					    LPCWSTR pwstrDeviceId);
};

//-------------------------------------------------------------------------
#endif // WIN32
