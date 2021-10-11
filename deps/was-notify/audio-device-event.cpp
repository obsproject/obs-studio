#ifdef WIN32
#include "audio-device-event.h"
#include <windows.h>
#include <assert.h>

WASAPINotify::WASAPINotify(IWASNotifyCallback *cb_) : cb(cb_)
{
	assert(cb);
}

STDMETHODIMP_(ULONG) WASAPINotify::AddRef()
{
	return InterlockedIncrement(&refs);
}

STDMETHODIMP_(ULONG) STDMETHODCALLTYPE WASAPINotify::Release()
{
	long temp = InterlockedDecrement(&refs); 
	if (0 == temp)
		delete this;
	return temp;
}

STDMETHODIMP WASAPINotify::QueryInterface(REFIID riid, void **ptr)
{
	if (riid == IID_IUnknown) {
		*ptr = (IUnknown *)this;
	} else if (riid == __uuidof(IMMNotificationClient)) {
		*ptr = (IMMNotificationClient *)this;
	} else {
		*ptr = nullptr;
		return E_NOINTERFACE;
	}

	InterlockedIncrement(&refs);
	return S_OK;
}

STDMETHODIMP WASAPINotify::OnDeviceAdded(LPCWSTR)
{
	return S_OK;
}

STDMETHODIMP WASAPINotify::OnDeviceRemoved(LPCWSTR)
{
	return S_OK;
}

STDMETHODIMP WASAPINotify::OnDeviceStateChanged(LPCWSTR, DWORD)
{
	return S_OK;
}

STDMETHODIMP WASAPINotify::OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY)
{
	return S_OK;
}

STDMETHODIMP WASAPINotify::OnDefaultDeviceChanged(EDataFlow flow, ERole role,
						  LPCWSTR pwstrDeviceId)
{
	if (pwstrDeviceId)
		cb->NotifyDefaultDeviceChanged(flow, role, pwstrDeviceId);
	return S_OK;
}

//-------------------------------------------------------------------------
#endif // WIN32
