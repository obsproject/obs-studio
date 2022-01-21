#pragma once
#include "audio-notify-client.h"
#include <util/windows/ComPtr.hpp>
#include <qobject.h>

class DetectDefaultDevice : public QObject, public IWASNotifyCallback {
	Q_OBJECT

public:
	~DetectDefaultDevice();

	static void Create();
	static DetectDefaultDevice *Instance();
	static void Destroy();

	void Start();
	void Stop();

protected:
	DetectDefaultDevice();

	// IWASNotifyCallback
	virtual void NotifyDefaultDeviceChanged(EDataFlow flow, ERole role,
						LPCWSTR id);
private slots:
	void OnDefaultOutputChanged();

private:
	ComPtr<IMMDeviceEnumerator> enumerator;
	ComPtr<WASAPINotify> notify;
};
