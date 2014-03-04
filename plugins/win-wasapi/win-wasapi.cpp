#include "enum-wasapi.hpp"

#include <obs.h>
#include <util/platform.h>
#include <util/windows/HRError.hpp>
#include <util/windows/ComPtr.hpp>
#include <util/windows/WinHandle.hpp>
#include <util/windows/CoTaskMemPtr.hpp>

using namespace std;

#define KSAUDIO_SPEAKER_4POINT1 (KSAUDIO_SPEAKER_QUAD|SPEAKER_LOW_FREQUENCY)
#define KSAUDIO_SPEAKER_2POINT1 (KSAUDIO_SPEAKER_STEREO|SPEAKER_LOW_FREQUENCY)

class WASAPISource {
	ComPtr<IMMDevice>           device;
	ComPtr<IAudioClient>        client;
	ComPtr<IAudioCaptureClient> capture;

	obs_source_t                source;
	string                      device_id;
	string                      device_name;
	bool                        isInputDevice;
	bool                        useDeviceTiming;
	bool                        isDefaultDevice;

	bool                        reconnecting;
	WinHandle                   reconnectThread;

	bool                        active;
	WinHandle                   captureThread;

	WinHandle                   stopSignal;
	WinHandle                   receiveSignal;

	speaker_layout              speakers;
	audio_format                format;
	uint32_t                    sampleRate;

	static DWORD WINAPI ReconnectThread(LPVOID param);
	static DWORD WINAPI CaptureThread(LPVOID param);

	bool ProcessCaptureData();

	void Reconnect();

	bool InitDevice(IMMDeviceEnumerator *enumerator);
	void InitName();
	void InitClient();
	void InitFormat(WAVEFORMATEX *wfex);
	void InitCapture();
	void Initialize();

	bool TryInitialize();

public:
	WASAPISource(obs_data_t settings, obs_source_t source_, bool input);
	inline ~WASAPISource();

	void UpdateSettings(obs_data_t settings);
};

WASAPISource::WASAPISource(obs_data_t settings, obs_source_t source_,
		bool input)
	: reconnecting    (false),
	  active          (false),
	  reconnectThread (nullptr),
	  captureThread   (nullptr),
	  source          (source_),
	  isInputDevice   (input)
{
	obs_data_set_default_string(settings, "device_id", "default");
	obs_data_set_default_bool(settings, "use_device_timing", true);
	UpdateSettings(settings);

	stopSignal = CreateEvent(nullptr, true, false, nullptr);
	if (!stopSignal.Valid())
		throw "Could not create stop signal";

	receiveSignal = CreateEvent(nullptr, false, false, nullptr);
	if (!receiveSignal.Valid())
		throw "Could not create receive signal";

	if (!TryInitialize()) {
		blog(LOG_INFO, "[WASAPISource::WASAPISource] "
		               "Device '%s' not found.  Waiting for device",
		               device_id.c_str());
		Reconnect();
	}
}

inline WASAPISource::~WASAPISource()
{
	SetEvent(stopSignal);

	if (active)
		WaitForSingleObject(captureThread, INFINITE);
	if (reconnecting)
		WaitForSingleObject(reconnectThread, INFINITE);
}

void WASAPISource::UpdateSettings(obs_data_t settings)
{
	device_id       = obs_data_getstring(settings, "device_id");
	useDeviceTiming = obs_data_getbool(settings, "useDeviceTiming");
	isDefaultDevice = _strcmpi(device_id.c_str(), "default") == 0;
}

bool WASAPISource::InitDevice(IMMDeviceEnumerator *enumerator)
{
	HRESULT res;

	if (isDefaultDevice) {
		res = enumerator->GetDefaultAudioEndpoint(
				isInputDevice ? eCapture        : eRender,
				isInputDevice ? eCommunications : eConsole,
				device.Assign());
	} else {
		wchar_t *w_id;
		os_utf8_to_wcs_ptr(device_id.c_str(), device_id.size(), &w_id);

		res = enumerator->GetDevice(w_id, device.Assign());

		bfree(w_id);
	}

	return SUCCEEDED(res);
}

#define BUFFER_TIME_100NS (5*10000000)

void WASAPISource::InitClient()
{
	CoTaskMemPtr<WAVEFORMATEX> wfex;
	HRESULT                    res;
	DWORD                      flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;

	res = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL,
			nullptr, (void**)client.Assign());
	if (FAILED(res))
		throw HRError("Failed to activate client context", res);

	res = client->GetMixFormat(&wfex);
	if (FAILED(res))
		throw HRError("Failed to get mix format", res);

	InitFormat(wfex);

	if (!isInputDevice)
		flags |= AUDCLNT_STREAMFLAGS_LOOPBACK;

	res = client->Initialize(
			AUDCLNT_SHAREMODE_SHARED, flags,
			BUFFER_TIME_100NS, 0, wfex, nullptr);
	if (FAILED(res))
		throw HRError("Failed to get initialize audio client", res);
}

static speaker_layout ConvertSpeakerLayout(DWORD layout, WORD channels)
{
	switch (layout) {
	case KSAUDIO_SPEAKER_QUAD:             return SPEAKERS_QUAD;
	case KSAUDIO_SPEAKER_2POINT1:          return SPEAKERS_2POINT1;
	case KSAUDIO_SPEAKER_4POINT1:          return SPEAKERS_4POINT1;
	case KSAUDIO_SPEAKER_SURROUND:         return SPEAKERS_SURROUND;
	case KSAUDIO_SPEAKER_5POINT1:          return SPEAKERS_5POINT1;
	case KSAUDIO_SPEAKER_5POINT1_SURROUND: return SPEAKERS_5POINT1_SURROUND;
	case KSAUDIO_SPEAKER_7POINT1:          return SPEAKERS_7POINT1;
	case KSAUDIO_SPEAKER_7POINT1_SURROUND: return SPEAKERS_7POINT1_SURROUND;
	}

	return (speaker_layout)channels;
}

void WASAPISource::InitFormat(WAVEFORMATEX *wfex)
{
	DWORD layout = 0;

	if (wfex->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
		WAVEFORMATEXTENSIBLE *ext = (WAVEFORMATEXTENSIBLE*)wfex;
		layout = ext->dwChannelMask;
	}

	/* WASAPI is always float */
	sampleRate = wfex->nSamplesPerSec;
	format     = AUDIO_FORMAT_FLOAT;
	speakers   = ConvertSpeakerLayout(layout, wfex->nChannels);
}

void WASAPISource::InitCapture()
{
	HRESULT res = client->GetService(__uuidof(IAudioCaptureClient),
			(void**)capture.Assign());
	if (FAILED(res))
		throw HRError("Failed to create capture context", res);

	res = client->SetEventHandle(receiveSignal);
	if (FAILED(res))
		throw HRError("Failed to set event handle", res);

	captureThread = CreateThread(nullptr, 0,
			WASAPISource::CaptureThread, this,
			0, nullptr);
	if (!captureThread.Valid())
		throw "Failed to create capture thread";

	client->Start();
	active = true;

	blog(LOG_INFO, "WASAPI: Device '%s' initialized", device_name.c_str());
}

void WASAPISource::Initialize()
{
	ComPtr<IMMDeviceEnumerator> enumerator;
	HRESULT res;

	res = CoCreateInstance(__uuidof(MMDeviceEnumerator),
			nullptr, CLSCTX_ALL,
			__uuidof(IMMDeviceEnumerator),
			(void**)enumerator.Assign());
	if (FAILED(res))
		throw HRError("Failed to create enumerator", res);

	if (!InitDevice(enumerator))
		return;

	device_name = GetDeviceName(device);

	InitClient();
	InitCapture();
}

bool WASAPISource::TryInitialize()
{
	try {
		Initialize();

	} catch (HRError error) {
		blog(LOG_ERROR, "[WASAPISource::TryInitialize]:[%s] %s: %lX",
				device_name.empty() ?
					device_id.c_str() : device_name.c_str(),
				error.str, error.hr);

	} catch (const char *error) {
		blog(LOG_ERROR, "[WASAPISource::TryInitialize]:[%s] %s",
				device_name.empty() ?
					device_id.c_str() : device_name.c_str(),
				error);
	}

	return active;
}

void WASAPISource::Reconnect()
{
	reconnecting = true;
	reconnectThread = CreateThread(nullptr, 0,
			WASAPISource::ReconnectThread, this,
			0, nullptr);

	if (!reconnectThread.Valid())
		blog(LOG_ERROR, "[WASAPISource::Reconnect] "
		                "Failed to intiialize reconnect thread: %d",
		                 GetLastError());
}

static inline bool WaitForSignal(HANDLE handle, DWORD time)
{
	return WaitForSingleObject(handle, time) == WAIT_TIMEOUT;
}

#define RECONNECT_INTERVAL 3000

DWORD WINAPI WASAPISource::ReconnectThread(LPVOID param)
{
	WASAPISource *source = (WASAPISource*)param;

	while (!WaitForSignal(source->stopSignal, RECONNECT_INTERVAL)) {
		if (source->TryInitialize())
			break;
	}

	source->reconnectThread = nullptr;
	source->reconnecting = false;
	return 0;
}

bool WASAPISource::ProcessCaptureData()
{
	HRESULT res;
	LPBYTE  buffer;
	UINT32  frames;
	DWORD   flags;
	UINT64  pos, ts;
	UINT    captureSize = 0;

	while (true) {
		res = capture->GetNextPacketSize(&captureSize);

		if (FAILED(res)) {
			if (res != AUDCLNT_E_DEVICE_INVALIDATED)
				blog(LOG_ERROR, "[WASAPISource::GetCaptureData]"
				                " capture->GetNextPacketSize"
				                " failed: %lX", res);
			return false;
		}

		if (!captureSize)
			break;

		res = capture->GetBuffer(&buffer, &frames, &flags, &pos, &ts);
		if (FAILED(res)) {
			if (res != AUDCLNT_E_DEVICE_INVALIDATED)
				blog(LOG_ERROR, "[WASAPISource::GetCaptureData]"
						" capture->GetBuffer"
						" failed: %lX", res);
			return false;
		}

		source_audio data    = {};
		data.data[0]         = (const uint8_t*)buffer;
		data.frames          = (uint32_t)frames;
		data.speakers        = speakers;
		data.samples_per_sec = sampleRate;
		data.format          = format;
		data.timestamp       = useDeviceTiming ?
			ts*100 : os_gettime_ns();

		obs_source_output_audio(source, &data);

		capture->ReleaseBuffer(frames);
	}

	return true;
}

static inline bool WaitForCaptureSignal(DWORD numSignals, const HANDLE *signals,
		DWORD duration)
{
	DWORD ret;
	ret = WaitForMultipleObjects(numSignals, signals, false, duration);

	return ret == WAIT_OBJECT_0 || ret == WAIT_TIMEOUT;
}

DWORD WINAPI WASAPISource::CaptureThread(LPVOID param)
{
	WASAPISource *source   = (WASAPISource*)param;
	bool         reconnect = false;

	/* Output devices don't signal, so just make it check every 10 ms */
	DWORD        dur       = source->isInputDevice ? INFINITE : 10;

	HANDLE sigs[2] = {
		source->receiveSignal,
		source->stopSignal
	};

	while (WaitForCaptureSignal(2, sigs, dur)) {
		if (!source->ProcessCaptureData()) {
			reconnect = true;
			break;
		}
	}

	source->client->Stop();

	source->captureThread = nullptr;
	source->active        = false;

	if (reconnect) {
		blog(LOG_INFO, "Device '%s' invalidated.  Retrying",
				source->device_name.c_str());
		source->Reconnect();
	}

	return 0;
}

/* ------------------------------------------------------------------------- */

static const char *GetWASAPIInputName(const char *locale)
{
	/* TODO: translate */
	return "Audio Input Capture (WASAPI)";
}

static const char *GetWASAPIOutputName(const char *locale)
{
	/* TODO: translate */
	return "Audio Output Capture (WASAPI)";
}

static void *CreateWASAPISource(obs_data_t settings, obs_source_t source,
		bool input)
{
	try {
		return new WASAPISource(settings, source, input);
	} catch (const char *error) {
		blog(LOG_ERROR, "[CreateWASAPISource] %s", error);
	}

	return nullptr;
}

static void *CreateWASAPIInput(obs_data_t settings, obs_source_t source)
{
	return CreateWASAPISource(settings, source, true);
}

static void *CreateWASAPIOutput(obs_data_t settings, obs_source_t source)
{
	return CreateWASAPISource(settings, source, false);
}

static void DestroyWASAPISource(void *obj)
{
	delete static_cast<WASAPISource*>(obj);
}

static obs_properties_t GetWASAPIProperties(const char *locale, bool input)
{
	obs_properties_t props = obs_properties_create();
	vector<AudioDeviceInfo> devices;

	/* TODO: translate */
	obs_property_t device_prop = obs_properties_add_list(props,
			"device_id", "Device",
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	GetWASAPIAudioDevices(devices, input);

	for (size_t i = 0; i < devices.size(); i++) {
		AudioDeviceInfo &device = devices[i];
		obs_property_list_add_item(device_prop,
				device.name.c_str(), device.id.c_str());
	}

	obs_properties_add_bool(props, "use_device_timing",
			"Use Device Timing");

	return props;
}

static obs_properties_t GetWASAPIPropertiesInput(const char *locale)
{
	return GetWASAPIProperties(locale, true);
}

static obs_properties_t GetWASAPIPropertiesOutput(const char *locale)
{
	return GetWASAPIProperties(locale, false);
}

struct obs_source_info wasapiInput {
	"wasapi_input_capture",
	OBS_SOURCE_TYPE_INPUT,
	OBS_SOURCE_AUDIO,
	GetWASAPIInputName,
	CreateWASAPIInput,
	DestroyWASAPISource,
	GetWASAPIPropertiesInput,	
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
};

struct obs_source_info wasapiOutput {
	"wasapi_output_capture",
	OBS_SOURCE_TYPE_INPUT,
	OBS_SOURCE_AUDIO,
	GetWASAPIOutputName,
	CreateWASAPIOutput,
	DestroyWASAPISource,
	GetWASAPIPropertiesOutput,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
};
