#include "enum-wasapi.hpp"

#include <obs-module.h>
#include <obs.h>
#include <util/platform.h>
#include <util/windows/HRError.hpp>
#include <util/windows/ComPtr.hpp>
#include <util/windows/WinHandle.hpp>
#include <util/windows/CoTaskMemPtr.hpp>
#include <util/threading.h>
#include <util/util_uint64.h>

#include <atomic>
#include <cinttypes>

#include <avrt.h>

using namespace std;

#define OPT_DEVICE_ID "device_id"
#define OPT_USE_DEVICE_TIMING "use_device_timing"

static void GetWASAPIDefaults(obs_data_t *settings);

#define OBS_KSAUDIO_SPEAKER_4POINT1 \
	(KSAUDIO_SPEAKER_SURROUND | SPEAKER_LOW_FREQUENCY)

class WASAPISource {
	ComPtr<IMMNotificationClient> notify;
	ComPtr<IMMDeviceEnumerator> enumerator;
	ComPtr<IAudioClient> client;
	ComPtr<IAudioCaptureClient> capture;

	obs_source_t *source;
	wstring default_id;
	string device_id;
	string device_name;
	uint64_t lastNotifyTime = 0;
	bool isInputDevice;
	std::atomic<bool> useDeviceTiming = false;
	bool isDefaultDevice = false;

	bool previouslyFailed = false;
	WinHandle reconnectThread;

	WinHandle captureThread;
	WinHandle idleSignal;
	WinHandle stopSignal;
	WinHandle receiveSignal;
	WinHandle restartSignal;
	WinHandle exitSignal;
	WinHandle initSignal;
	DWORD reconnectDuration = 0;
	WinHandle reconnectSignal;

	speaker_layout speakers;
	audio_format format;
	uint32_t sampleRate;

	static DWORD WINAPI ReconnectThread(LPVOID param);
	static DWORD WINAPI CaptureThread(LPVOID param);

	bool ProcessCaptureData();

	void Start();
	void Stop();

	static ComPtr<IMMDevice> InitDevice(IMMDeviceEnumerator *enumerator,
					    bool isDefaultDevice,
					    bool isInputDevice,
					    const string device_id,
					    wstring &default_id);
	static ComPtr<IAudioClient> InitClient(IMMDevice *device,
					       bool isInputDevice,
					       enum speaker_layout &speakers,
					       enum audio_format &format,
					       uint32_t &sampleRate);
	static void InitFormat(const WAVEFORMATEX *wfex,
			       enum speaker_layout &speakers,
			       enum audio_format &format, uint32_t &sampleRate);
	static void ClearBuffer(IMMDevice *device);
	static ComPtr<IAudioCaptureClient> InitCapture(IAudioClient *client,
						       HANDLE receiveSignal);
	void Initialize();

	bool TryInitialize();

	void UpdateSettings(obs_data_t *settings);

public:
	WASAPISource(obs_data_t *settings, obs_source_t *source_, bool input);
	~WASAPISource();

	void Update(obs_data_t *settings);

	void SetDefaultDevice(EDataFlow flow, ERole role, LPCWSTR id);
};

class WASAPINotify : public IMMNotificationClient {
	long refs = 0; /* auto-incremented to 1 by ComPtr */
	WASAPISource *source;

public:
	WASAPINotify(WASAPISource *source_) : source(source_) {}

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

		os_atomic_inc_long(&refs);
		return S_OK;
	}

	STDMETHODIMP OnDefaultDeviceChanged(EDataFlow flow, ERole role,
					    LPCWSTR id)
	{
		source->SetDefaultDevice(flow, role, id);
		return S_OK;
	}

	STDMETHODIMP OnDeviceAdded(LPCWSTR) { return S_OK; }
	STDMETHODIMP OnDeviceRemoved(LPCWSTR) { return S_OK; }
	STDMETHODIMP OnDeviceStateChanged(LPCWSTR, DWORD) { return S_OK; }
	STDMETHODIMP OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY)
	{
		return S_OK;
	}
};

WASAPISource::WASAPISource(obs_data_t *settings, obs_source_t *source_,
			   bool input)
	: source(source_), isInputDevice(input)
{
	UpdateSettings(settings);

	idleSignal = CreateEvent(nullptr, true, false, nullptr);
	if (!idleSignal.Valid())
		throw "Could not create idle signal";

	stopSignal = CreateEvent(nullptr, true, false, nullptr);
	if (!stopSignal.Valid())
		throw "Could not create stop signal";

	receiveSignal = CreateEvent(nullptr, false, false, nullptr);
	if (!receiveSignal.Valid())
		throw "Could not create receive signal";

	restartSignal = CreateEvent(nullptr, true, false, nullptr);
	if (!restartSignal.Valid())
		throw "Could not create restart signal";

	exitSignal = CreateEvent(nullptr, true, false, nullptr);
	if (!exitSignal.Valid())
		throw "Could not create exit signal";

	initSignal = CreateEvent(nullptr, false, false, nullptr);
	if (!initSignal.Valid())
		throw "Could not create init signal";

	reconnectSignal = CreateEvent(nullptr, false, false, nullptr);
	if (!reconnectSignal.Valid())
		throw "Could not create reconnect signal";

	reconnectThread = CreateThread(
		nullptr, 0, WASAPISource::ReconnectThread, this, 0, nullptr);
	if (!reconnectThread.Valid())
		throw "Failed to create reconnect thread";

	notify = new WASAPINotify(this);
	if (!notify)
		throw "Could not create WASAPINotify";

	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
				      CLSCTX_ALL,
				      IID_PPV_ARGS(enumerator.Assign()));
	if (FAILED(hr))
		throw HRError("Failed to create enumerator", hr);

	hr = enumerator->RegisterEndpointNotificationCallback(notify);
	if (FAILED(hr))
		throw HRError("Failed to register endpoint callback", hr);

	captureThread = CreateThread(nullptr, 0, WASAPISource::CaptureThread,
				     this, 0, nullptr);
	if (!captureThread.Valid()) {
		enumerator->UnregisterEndpointNotificationCallback(notify);
		throw "Failed to create capture thread";
	}

	Start();
}

void WASAPISource::Start()
{
	SetEvent(initSignal);
}

void WASAPISource::Stop()
{
	SetEvent(stopSignal);

	blog(LOG_INFO, "WASAPI: Device '%s' Terminated", device_name.c_str());

	WaitForSingleObject(idleSignal, INFINITE);

	SetEvent(exitSignal);

	WaitForSingleObject(reconnectThread, INFINITE);

	WaitForSingleObject(captureThread, INFINITE);
}

WASAPISource::~WASAPISource()
{
	enumerator->UnregisterEndpointNotificationCallback(notify);
	Stop();
}

void WASAPISource::UpdateSettings(obs_data_t *settings)
{
	device_id = obs_data_get_string(settings, OPT_DEVICE_ID);
	useDeviceTiming = obs_data_get_bool(settings, OPT_USE_DEVICE_TIMING);
	isDefaultDevice = _strcmpi(device_id.c_str(), "default") == 0;
}

void WASAPISource::Update(obs_data_t *settings)
{
	const string newDevice = obs_data_get_string(settings, OPT_DEVICE_ID);
	const bool restart = newDevice.compare(device_id) != 0;

	UpdateSettings(settings);

	if (restart)
		SetEvent(restartSignal);
}

ComPtr<IMMDevice> WASAPISource::InitDevice(IMMDeviceEnumerator *enumerator,
					   bool isDefaultDevice,
					   bool isInputDevice,
					   const string device_id,
					   wstring &default_id)
{
	ComPtr<IMMDevice> device;

	if (isDefaultDevice) {
		HRESULT res = enumerator->GetDefaultAudioEndpoint(
			isInputDevice ? eCapture : eRender,
			isInputDevice ? eCommunications : eConsole,
			device.Assign());
		if (FAILED(res))
			throw HRError("Failed GetDefaultAudioEndpoint", res);

		CoTaskMemPtr<wchar_t> id;
		res = device->GetId(&id);
		if (FAILED(res))
			throw HRError("Failed to get default id", res);
		default_id = id;
	} else {
		wchar_t *w_id;
		os_utf8_to_wcs_ptr(device_id.c_str(), device_id.size(), &w_id);
		if (!w_id)
			throw "Failed to widen device id string";

		const HRESULT res =
			enumerator->GetDevice(w_id, device.Assign());

		bfree(w_id);

		if (FAILED(res))
			throw HRError("Failed to enumerate device", res);
	}

	return device;
}

#define BUFFER_TIME_100NS (5 * 10000000)

ComPtr<IAudioClient> WASAPISource::InitClient(IMMDevice *device,
					      bool isInputDevice,
					      enum speaker_layout &speakers,
					      enum audio_format &format,
					      uint32_t &sampleRate)
{
	ComPtr<IAudioClient> client;
	HRESULT res = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL,
				       nullptr, (void **)client.Assign());
	if (FAILED(res))
		throw HRError("Failed to activate client context", res);

	CoTaskMemPtr<WAVEFORMATEX> wfex;
	res = client->GetMixFormat(&wfex);
	if (FAILED(res))
		throw HRError("Failed to get mix format", res);

	InitFormat(wfex, speakers, format, sampleRate);

	DWORD flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
	if (!isInputDevice)
		flags |= AUDCLNT_STREAMFLAGS_LOOPBACK;

	res = client->Initialize(AUDCLNT_SHAREMODE_SHARED, flags,
				 BUFFER_TIME_100NS, 0, wfex, nullptr);
	if (FAILED(res))
		throw HRError("Failed to initialize audio client", res);

	return client;
}

void WASAPISource::ClearBuffer(IMMDevice *device)
{
	CoTaskMemPtr<WAVEFORMATEX> wfex;
	HRESULT res;
	LPBYTE buffer;
	UINT32 frames;
	ComPtr<IAudioClient> client;

	res = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
			       (void **)client.Assign());
	if (FAILED(res))
		throw HRError("Failed to activate client context", res);

	res = client->GetMixFormat(&wfex);
	if (FAILED(res))
		throw HRError("Failed to get mix format", res);

	res = client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, BUFFER_TIME_100NS,
				 0, wfex, nullptr);
	if (FAILED(res))
		throw HRError("Failed to initialize audio client", res);

	/* Silent loopback fix. Prevents audio stream from stopping and */
	/* messing up timestamps and other weird glitches during silence */
	/* by playing a silent sample all over again. */

	res = client->GetBufferSize(&frames);
	if (FAILED(res))
		throw HRError("Failed to get buffer size", res);

	ComPtr<IAudioRenderClient> render;
	res = client->GetService(IID_PPV_ARGS(render.Assign()));
	if (FAILED(res))
		throw HRError("Failed to get render client", res);

	res = render->GetBuffer(frames, &buffer);
	if (FAILED(res))
		throw HRError("Failed to get buffer", res);

	memset(buffer, 0, (size_t)frames * (size_t)wfex->nBlockAlign);

	render->ReleaseBuffer(frames, 0);
}

static speaker_layout ConvertSpeakerLayout(DWORD layout, WORD channels)
{
	switch (layout) {
	case KSAUDIO_SPEAKER_2POINT1:
		return SPEAKERS_2POINT1;
	case KSAUDIO_SPEAKER_SURROUND:
		return SPEAKERS_4POINT0;
	case OBS_KSAUDIO_SPEAKER_4POINT1:
		return SPEAKERS_4POINT1;
	case KSAUDIO_SPEAKER_5POINT1_SURROUND:
		return SPEAKERS_5POINT1;
	case KSAUDIO_SPEAKER_7POINT1_SURROUND:
		return SPEAKERS_7POINT1;
	}

	return (speaker_layout)channels;
}

void WASAPISource::InitFormat(const WAVEFORMATEX *wfex,
			      enum speaker_layout &speakers,
			      enum audio_format &format, uint32_t &sampleRate)
{
	DWORD layout = 0;

	if (wfex->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
		WAVEFORMATEXTENSIBLE *ext = (WAVEFORMATEXTENSIBLE *)wfex;
		layout = ext->dwChannelMask;
	}

	/* WASAPI is always float */
	speakers = ConvertSpeakerLayout(layout, wfex->nChannels);
	format = AUDIO_FORMAT_FLOAT;
	sampleRate = wfex->nSamplesPerSec;
}

ComPtr<IAudioCaptureClient> WASAPISource::InitCapture(IAudioClient *client,
						      HANDLE receiveSignal)
{
	ComPtr<IAudioCaptureClient> capture;
	HRESULT res = client->GetService(IID_PPV_ARGS(capture.Assign()));
	if (FAILED(res))
		throw HRError("Failed to create capture context", res);

	res = client->SetEventHandle(receiveSignal);
	if (FAILED(res))
		throw HRError("Failed to set event handle", res);

	res = client->Start();
	if (FAILED(res))
		throw HRError("Failed to start capture client", res);

	return capture;
}

void WASAPISource::Initialize()
{
	ComPtr<IMMDevice> device = InitDevice(enumerator, isDefaultDevice,
					      isInputDevice, device_id,
					      default_id);

	device_name = GetDeviceName(device);

	ResetEvent(receiveSignal);

	ComPtr<IAudioClient> temp_client =
		InitClient(device, isInputDevice, speakers, format, sampleRate);
	if (!isInputDevice)
		ClearBuffer(device);
	ComPtr<IAudioCaptureClient> temp_capture =
		InitCapture(temp_client, receiveSignal);

	client = std::move(temp_client);
	capture = std::move(temp_capture);

	blog(LOG_INFO, "WASAPI: Device '%s' [%" PRIu32 " Hz] initialized",
	     device_name.c_str(), sampleRate);
}

bool WASAPISource::TryInitialize()
{
	bool success = false;
	try {
		Initialize();
		success = true;

	} catch (HRError &error) {
		if (!previouslyFailed) {
			blog(LOG_WARNING,
			     "[WASAPISource::TryInitialize]:[%s] %s: %lX",
			     device_name.empty() ? device_id.c_str()
						 : device_name.c_str(),
			     error.str, error.hr);
		}
	} catch (const char *error) {
		if (!previouslyFailed) {
			blog(LOG_WARNING,
			     "[WASAPISource::TryInitialize]:[%s] %s",
			     device_name.empty() ? device_id.c_str()
						 : device_name.c_str(),
			     error);
		}
	}

	previouslyFailed = !success;
	return success;
}

DWORD WINAPI WASAPISource::ReconnectThread(LPVOID param)
{
	os_set_thread_name("win-wasapi: reconnect thread");

	WASAPISource *source = (WASAPISource *)param;

	const HANDLE sigs[] = {
		source->exitSignal,
		source->reconnectSignal,
	};

	bool exit = false;
	while (!exit) {
		const DWORD ret = WaitForMultipleObjects(_countof(sigs), sigs,
							 false, INFINITE);
		switch (ret) {
		case WAIT_OBJECT_0:
			exit = true;
			break;
		default:
			assert(ret == (WAIT_OBJECT_0 + 1));
			if (source->reconnectDuration > 0) {
				WaitForSingleObject(source->stopSignal,
						    source->reconnectDuration);
			}
			source->Start();
		}
	}

	return 0;
}

bool WASAPISource::ProcessCaptureData()
{
	HRESULT res;
	LPBYTE buffer;
	UINT32 frames;
	DWORD flags;
	UINT64 pos, ts;
	UINT captureSize = 0;

	while (true) {
		res = capture->GetNextPacketSize(&captureSize);
		if (FAILED(res)) {
			if (res != AUDCLNT_E_DEVICE_INVALIDATED)
				blog(LOG_WARNING,
				     "[WASAPISource::ProcessCaptureData]"
				     " capture->GetNextPacketSize"
				     " failed: %lX",
				     res);
			return false;
		}

		if (!captureSize)
			break;

		res = capture->GetBuffer(&buffer, &frames, &flags, &pos, &ts);
		if (FAILED(res)) {
			if (res != AUDCLNT_E_DEVICE_INVALIDATED)
				blog(LOG_WARNING,
				     "[WASAPISource::ProcessCaptureData]"
				     " capture->GetBuffer"
				     " failed: %lX",
				     res);
			return false;
		}

		obs_source_audio data = {};
		data.data[0] = (const uint8_t *)buffer;
		data.frames = (uint32_t)frames;
		data.speakers = speakers;
		data.samples_per_sec = sampleRate;
		data.format = format;
		data.timestamp = useDeviceTiming ? ts * 100 : os_gettime_ns();

		if (!useDeviceTiming)
			data.timestamp -= util_mul_div64(frames, 1000000000ULL,
							 sampleRate);

		obs_source_output_audio(source, &data);

		capture->ReleaseBuffer(frames);
	}

	return true;
}

#define RECONNECT_INTERVAL 3000

DWORD WINAPI WASAPISource::CaptureThread(LPVOID param)
{
	os_set_thread_name("win-wasapi: capture thread");

	const HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
	const bool com_initialized = SUCCEEDED(hr);
	if (!com_initialized) {
		blog(LOG_ERROR,
		     "[WASAPISource::CaptureThread]"
		     " CoInitializeEx failed: 0x%08X",
		     hr);
	}

	DWORD unused = 0;
	const HANDLE handle = AvSetMmThreadCharacteristics(L"Audio", &unused);

	WASAPISource *source = (WASAPISource *)param;

	const HANDLE inactive_sigs[] = {
		source->exitSignal,
		source->stopSignal,
		source->initSignal,
	};

	const HANDLE active_sigs[] = {
		source->exitSignal,
		source->stopSignal,
		source->receiveSignal,
		source->restartSignal,
	};

	DWORD sig_count = _countof(inactive_sigs);
	const HANDLE *sigs = inactive_sigs;

	bool exit = false;
	while (!exit) {
		bool idle = false;
		bool stop = false;
		bool reconnect = false;
		do {
			/* Windows 7 does not seem to wake up for LOOPBACK */
			const DWORD dwMilliseconds = ((sigs == active_sigs) &&
						      !source->isInputDevice)
							     ? 10
							     : INFINITE;

			const DWORD ret = WaitForMultipleObjects(
				sig_count, sigs, false, dwMilliseconds);
			switch (ret) {
			case WAIT_OBJECT_0: {
				exit = true;
				stop = true;
				idle = true;
				break;
			}

			case WAIT_OBJECT_0 + 1:
				stop = true;
				idle = true;
				break;

			case WAIT_OBJECT_0 + 2:
			case WAIT_TIMEOUT:
				if (sigs == inactive_sigs) {
					assert(ret != WAIT_TIMEOUT);

					if (source->TryInitialize()) {
						sig_count =
							_countof(active_sigs);
						sigs = active_sigs;
					} else {
						blog(LOG_INFO,
						     "WASAPI: Device '%s' failed to start",
						     source->device_id.c_str());
						stop = true;
						reconnect = true;
						source->reconnectDuration =
							RECONNECT_INTERVAL;
					}
				} else {
					stop = !source->ProcessCaptureData();
					if (stop) {
						blog(LOG_INFO,
						     "Device '%s' invalidated.  Retrying",
						     source->device_name
							     .c_str());
						stop = true;
						reconnect = true;
						source->reconnectDuration =
							RECONNECT_INTERVAL;
					}
				}
				break;

			default:
				assert(sigs == active_sigs);
				assert(ret == WAIT_OBJECT_0 + 3);
				stop = true;
				reconnect = true;
				source->reconnectDuration = 0;
				ResetEvent(source->restartSignal);
			}
		} while (!stop);

		sig_count = _countof(inactive_sigs);
		sigs = inactive_sigs;

		if (source->client) {
			source->client->Stop();

			source->capture.Clear();
			source->client.Clear();
		}

		if (idle) {
			SetEvent(source->idleSignal);
		} else if (reconnect) {
			blog(LOG_INFO, "Device '%s' invalidated.  Retrying",
			     source->device_name.c_str());
			SetEvent(source->reconnectSignal);
		}
	}

	if (handle)
		AvRevertMmThreadCharacteristics(handle);

	if (com_initialized)
		CoUninitialize();

	return 0;
}

void WASAPISource::SetDefaultDevice(EDataFlow flow, ERole role, LPCWSTR id)
{
	if (!isDefaultDevice)
		return;

	EDataFlow expectedFlow = isInputDevice ? eCapture : eRender;
	ERole expectedRole = isInputDevice ? eCommunications : eConsole;

	if (flow != expectedFlow || role != expectedRole)
		return;
	if (id && default_id.compare(id) == 0)
		return;

	blog(LOG_INFO, "WASAPI: Default %s device changed",
	     isInputDevice ? "input" : "output");

	/* reset device only once every 300ms */
	uint64_t t = os_gettime_ns();
	if (t - lastNotifyTime < 300000000)
		return;

	lastNotifyTime = t;

	SetEvent(restartSignal);
}

/* ------------------------------------------------------------------------- */

static const char *GetWASAPIInputName(void *)
{
	return obs_module_text("AudioInput");
}

static const char *GetWASAPIOutputName(void *)
{
	return obs_module_text("AudioOutput");
}

static void GetWASAPIDefaultsInput(obs_data_t *settings)
{
	obs_data_set_default_string(settings, OPT_DEVICE_ID, "default");
	obs_data_set_default_bool(settings, OPT_USE_DEVICE_TIMING, false);
}

static void GetWASAPIDefaultsOutput(obs_data_t *settings)
{
	obs_data_set_default_string(settings, OPT_DEVICE_ID, "default");
	obs_data_set_default_bool(settings, OPT_USE_DEVICE_TIMING, true);
}

static void *CreateWASAPISource(obs_data_t *settings, obs_source_t *source,
				bool input)
{
	try {
		return new WASAPISource(settings, source, input);
	} catch (const char *error) {
		blog(LOG_ERROR, "[CreateWASAPISource] %s", error);
	}

	return nullptr;
}

static void *CreateWASAPIInput(obs_data_t *settings, obs_source_t *source)
{
	return CreateWASAPISource(settings, source, true);
}

static void *CreateWASAPIOutput(obs_data_t *settings, obs_source_t *source)
{
	return CreateWASAPISource(settings, source, false);
}

static void DestroyWASAPISource(void *obj)
{
	delete static_cast<WASAPISource *>(obj);
}

static void UpdateWASAPISource(void *obj, obs_data_t *settings)
{
	static_cast<WASAPISource *>(obj)->Update(settings);
}

static obs_properties_t *GetWASAPIProperties(bool input)
{
	obs_properties_t *props = obs_properties_create();
	vector<AudioDeviceInfo> devices;

	obs_property_t *device_prop = obs_properties_add_list(
		props, OPT_DEVICE_ID, obs_module_text("Device"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	GetWASAPIAudioDevices(devices, input);

	if (devices.size())
		obs_property_list_add_string(
			device_prop, obs_module_text("Default"), "default");

	for (size_t i = 0; i < devices.size(); i++) {
		AudioDeviceInfo &device = devices[i];
		obs_property_list_add_string(device_prop, device.name.c_str(),
					     device.id.c_str());
	}

	obs_properties_add_bool(props, OPT_USE_DEVICE_TIMING,
				obs_module_text("UseDeviceTiming"));

	return props;
}

static obs_properties_t *GetWASAPIPropertiesInput(void *)
{
	return GetWASAPIProperties(true);
}

static obs_properties_t *GetWASAPIPropertiesOutput(void *)
{
	return GetWASAPIProperties(false);
}

void RegisterWASAPIInput()
{
	obs_source_info info = {};
	info.id = "wasapi_input_capture";
	info.type = OBS_SOURCE_TYPE_INPUT;
	info.output_flags = OBS_SOURCE_AUDIO | OBS_SOURCE_DO_NOT_DUPLICATE;
	info.get_name = GetWASAPIInputName;
	info.create = CreateWASAPIInput;
	info.destroy = DestroyWASAPISource;
	info.update = UpdateWASAPISource;
	info.get_defaults = GetWASAPIDefaultsInput;
	info.get_properties = GetWASAPIPropertiesInput;
	info.icon_type = OBS_ICON_TYPE_AUDIO_INPUT;
	obs_register_source(&info);
}

void RegisterWASAPIOutput()
{
	obs_source_info info = {};
	info.id = "wasapi_output_capture";
	info.type = OBS_SOURCE_TYPE_INPUT;
	info.output_flags = OBS_SOURCE_AUDIO | OBS_SOURCE_DO_NOT_DUPLICATE |
			    OBS_SOURCE_DO_NOT_SELF_MONITOR;
	info.get_name = GetWASAPIOutputName;
	info.create = CreateWASAPIOutput;
	info.destroy = DestroyWASAPISource;
	info.update = UpdateWASAPISource;
	info.get_defaults = GetWASAPIDefaultsOutput;
	info.get_properties = GetWASAPIPropertiesOutput;
	info.icon_type = OBS_ICON_TYPE_AUDIO_OUTPUT;
	obs_register_source(&info);
}
