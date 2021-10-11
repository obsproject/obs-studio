#pragma once
#ifdef WIN32
#include <was-notify/audio-device-event.h>
#include <qobject.h>

class OBSDetectResetAudioMonitor : public QObject, public IWASNotifyCallback {
	Q_OBJECT

public:
	OBSDetectResetAudioMonitor();
	~OBSDetectResetAudioMonitor();

	void Start();
	void Stop();

protected:
	// IWASNotifyCallback
	virtual void NotifyDefaultDeviceChanged(EDataFlow flow, ERole role,
						LPCWSTR id);
private slots:
	void OnDefaultOutputChanged();

private:
	IMMDeviceEnumerator *enumerator = NULL;
	WASAPINotify *notify = NULL;
};

//-------------------------------------------------------------------------
#endif // WIN32
