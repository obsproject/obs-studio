#include "wasapi-notify.hpp"

#include <windows.h>
#include <assert.h>

#include <util/threading.h>

class NotificationClient : public IMMNotificationClient {
	volatile long refs = 1;
	WASAPINotifyDefaultDeviceChangedCallback cb;

public:
	NotificationClient(WASAPINotifyDefaultDeviceChangedCallback cb) : cb(cb)
	{
		assert(cb);
	}

	STDMETHODIMP_(ULONG) AddRef()
	{
		return (ULONG)os_atomic_inc_long(&refs);
	}

	STDMETHODIMP_(ULONG) STDMETHODCALLTYPE Release()
	{
		long val = os_atomic_dec_long(&refs);
		if (val == 0)
			delete this;
		return (ULONG)val;
	}

	STDMETHODIMP QueryInterface(REFIID riid, void **ptr)
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

	STDMETHODIMP OnDeviceAdded(LPCWSTR) { return S_OK; }

	STDMETHODIMP OnDeviceRemoved(LPCWSTR) { return S_OK; }

	STDMETHODIMP OnDeviceStateChanged(LPCWSTR, DWORD) { return S_OK; }

	STDMETHODIMP OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY)
	{
		return S_OK;
	}

	STDMETHODIMP OnDefaultDeviceChanged(EDataFlow flow, ERole role,
					    LPCWSTR id)
	{
		if (cb && id)
			cb(flow, role, id);

		return S_OK;
	}
};

WASAPINotify::WASAPINotify()
{
	HRESULT res = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
				       CLSCTX_ALL,
				       __uuidof(IMMDeviceEnumerator),
				       (LPVOID *)enumerator.Assign());
	if (SUCCEEDED(res)) {
		notificationClient = new NotificationClient(
			std::bind(&WASAPINotify::OnDefaultDeviceChanged, this,
				  std::placeholders::_1, std::placeholders::_2,
				  std::placeholders::_3));
		enumerator->RegisterEndpointNotificationCallback(
			notificationClient);
	} else {
		enumerator.Clear();
	}
}

WASAPINotify::~WASAPINotify()
{
	if (enumerator) {
		enumerator->UnregisterEndpointNotificationCallback(
			notificationClient);
		enumerator.Clear();
	}

	notificationClient.Clear();
}

void WASAPINotify::AddDefaultDeviceChangedCallback(
	void *handle, WASAPINotifyDefaultDeviceChangedCallback cb)
{
	if (!handle)
		return;

	std::lock_guard<std::mutex> l(mutex);
	defaultDeviceChangedCallbacks[handle] = cb;
}

void WASAPINotify::RemoveDefaultDeviceChangedCallback(void *handle)
{
	if (!handle)
		return;

	std::lock_guard<std::mutex> l(mutex);
	defaultDeviceChangedCallbacks.erase(handle);
}

void WASAPINotify::OnDefaultDeviceChanged(EDataFlow flow, ERole role,
					  LPCWSTR id)
{
	std::lock_guard<std::mutex> l(mutex);
	for (const auto &cb : defaultDeviceChangedCallbacks)
		cb.second(flow, role, id);
}
