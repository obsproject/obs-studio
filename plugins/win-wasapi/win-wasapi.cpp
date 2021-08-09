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

#include <thread>
#include <mutex>

using namespace std;

#define OPT_DEVICE_ID "device_id"
#define OPT_USE_DEVICE_TIMING "use_device_timing"

static void GetWASAPIDefaults(obs_data_t *settings);

#define OBS_KSAUDIO_SPEAKER_4POINT1 \
	(KSAUDIO_SPEAKER_SURROUND | SPEAKER_LOW_FREQUENCY)

class WASAPISource {
	ComPtr<IMMDevice> device;
	ComPtr<IAudioClient> client;
	ComPtr<IAudioCaptureClient> capture;
	ComPtr<IAudioRenderClient> render;
	ComPtr<IMMDeviceEnumerator> enumerator;
	ComPtr<IMMNotificationClient> notify;

	static const int MAX_RETRY_INIT_DEVICE_COUNTER = 3;

	obs_source_t *source;
	wstring default_id;
	string device_id;
	string device_name;
	string device_sample = "-";
	bool isInputDevice;
	bool useDeviceTiming = false;
	bool isDefaultDevice = false;

	recursive_mutex state_mutex;
	bool initInterrupted = false;
	bool initializing = false;
	bool reconnecting = false;
	bool previouslyFailed = false;
	WinHandle reconnectThread;

	bool active = false;
	WinHandle captureThread;

	WinHandle stopSignal;
	WinHandle receiveSignal;

	speaker_layout speakers;
	audio_format format;
	uint32_t sampleRate;

	static DWORD WINAPI ReconnectThread(LPVOID param);
	static DWORD WINAPI CaptureThread(LPVOID param);

	bool ProcessCaptureData();

	inline void Start();
	inline void Stop();
	void Reconnect();

	HRESULT _InitDevice(IMMDeviceEnumerator *enumerator, bool defaultDevice);
	HRESULT InitDevice(IMMDeviceEnumerator *enumerator);
	void InitName();
	void InitClient();
	void InitRender();
	void InitFormat(WAVEFORMATEX *wfex);
	void InitCapture();
	void Initialize();

	bool TryInitialize();
	void UpdateSettings(obs_data_t *settings);
	void ThrowOnInterrupt(std::string message);

public:
	WASAPISource(obs_data_t *settings, obs_source_t *source_, bool input);
	virtual ~WASAPISource();

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
	: source          (source_),
	  isInputDevice   (input)
{
	blog(LOG_INFO, "[WASAPISource][%08X] WASAPI Source constructor", this);
	UpdateSettings(settings);

	stopSignal = CreateEvent(nullptr, true, false, nullptr);
	if (!stopSignal.Valid())
		throw "[WASAPISource] Could not create stop signal";

	receiveSignal = CreateEvent(nullptr, false, false, nullptr);
	if (!receiveSignal.Valid())
		throw "[WASAPISource] Could not create receive signal";

	Start();
}

inline void WASAPISource::Start()
{
	if (!TryInitialize()) {
		blog(LOG_INFO,
		     "[WASAPISource::Start][%08X] "
		     "Device '%s' not found.  Waiting for device",
		     this, device_id.c_str());
		Reconnect();
	}
}

inline void WASAPISource::Stop()
{
	blog(LOG_INFO, "[WASAPISource::Stop][%08X] Device '%s' Stop called", this,
		device_id.c_str());

	SetEvent(stopSignal);

	if (active || captureThread.Valid()) {
		blog(LOG_INFO, "[WASAPISource::Stop][%08X] Device '%s' Wait for captureThread to stop", this,
		     device_id.c_str());
		WaitForSingleObject(captureThread, INFINITE);
	}

	if (reconnecting || reconnectThread.Valid()) {
		blog(LOG_INFO, "[WASAPISource::Stop][%08X] Device '%s' Wait for reconnectThread to stop", this,
		     device_id.c_str());

		WaitForSingleObject(reconnectThread, INFINITE);
	}

	ResetEvent(stopSignal);
}

WASAPISource::~WASAPISource()
{
	if (enumerator.Get() != nullptr && notify.Get() != nullptr)
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
	string newDevice = obs_data_get_string(settings, OPT_DEVICE_ID);
	bool restart = newDevice.compare(device_id) != 0;

	if (restart)
		Stop();

	UpdateSettings(settings);

	if (restart)
		Start();
}

HRESULT WASAPISource::_InitDevice(IMMDeviceEnumerator *enumerator, bool defaultDevice)
{
	HRESULT res;

	if (isDefaultDevice) {
		res = enumerator->GetDefaultAudioEndpoint(
			isInputDevice ? eCapture : eRender,
			isInputDevice ? eCommunications : eConsole,
			device.Assign());
		if (FAILED(res))
			return false;

		CoTaskMemPtr<wchar_t> id;
		res = device->GetId(&id);
		default_id = id;
	} else {
		wchar_t *w_id;
		os_utf8_to_wcs_ptr(device_id.c_str(), device_id.size(), &w_id);

		res = enumerator->GetDevice(w_id, device.Assign());

		bfree(w_id);
	}

	blog(LOG_INFO, "[WASAPISource::_InitDevice][%08X]: Returning device pointer = %08x",
		this, device.Get());
	return res;
}

HRESULT WASAPISource::InitDevice(IMMDeviceEnumerator *enumerator)
{
	HRESULT res = -1;
	std::vector<AudioDeviceInfo> devices;
	res = _InitDevice(enumerator, isDefaultDevice);

	if (device_name.empty())
		device_name = GetDeviceName(device);

	if (SUCCEEDED(res))
		return res;

	if (!device_name.empty()) {
		blog(LOG_INFO, "[WASAPISource::InitDevice][%08X]: Failed to init device and device name not empty '%s'",
		     this, device_name.c_str());
		devices.clear();
		GetWASAPIAudioDevices(devices, isInputDevice, device_name);
		if (devices.size()) {
			blog(LOG_INFO, "[WASAPISource::InitDevice][%08X]: Use divice from GetWASAPIAudioDevices, name '%s'",
			     this, device_name.c_str());

			this->device = devices[0].device;
			this->device_id = devices[0].id;
			res = 0;
			return res;
		}
	}
	return res;
}

#define BUFFER_TIME_100NS (5 * 10000000)

void WASAPISource::ThrowOnInterrupt(std::string message)
{
	std::lock_guard<std::recursive_mutex> guard(state_mutex);
	if (initInterrupted) 
		throw message.c_str();
}

void WASAPISource::InitClient()
{
	CoTaskMemPtr<WAVEFORMATEX> wfex;
	HRESULT res;
	DWORD flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;

	ThrowOnInterrupt("[WASAPISource::InitClient]: Interrupted before device Activate");
	res = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
			       (void **)client.Assign());
	if (FAILED(res))
		throw HRError(
			"[WASAPISource::InitClient] Failed to activate client context",
			res);

	ThrowOnInterrupt("[WASAPISource::InitClient]: Interrupted before GetMixFormat");
	res = client->GetMixFormat(&wfex);
	if (FAILED(res))
		throw HRError("[WASAPISource::InitClient] Failed to get mix format", res);

	InitFormat(wfex);

	if (!isInputDevice)
		flags |= AUDCLNT_STREAMFLAGS_LOOPBACK;

	ThrowOnInterrupt("[WASAPISource::InitClient]: Interrupted before client Initialize");
	res = client->Initialize(AUDCLNT_SHAREMODE_SHARED, flags,
				 BUFFER_TIME_100NS, 0, wfex, nullptr);
	if (FAILED(res))
		throw HRError("[WASAPISource::InitClient] Failed to get initialize audio client", res);
}

void WASAPISource::InitRender()
{
	CoTaskMemPtr<WAVEFORMATEX> wfex;
	HRESULT res;
	LPBYTE buffer;
	UINT32 frames;
	ComPtr<IAudioClient> client;

	ThrowOnInterrupt("[WASAPISource::InitRender]: Interrupted before device Activate");
	res = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
			       (void **)client.Assign());
	if (FAILED(res))
		throw HRError("[WASAPISource::InitRender] Failed to activate client context", res);

	ThrowOnInterrupt("[WASAPISource::InitRender]: Interrupted before GetMixFormat");
	res = client->GetMixFormat(&wfex);
	if (FAILED(res))
		throw HRError("[WASAPISource::InitRender] Failed to get mix format", res);

	ThrowOnInterrupt("[WASAPISource::InitRender]: Interrupted client Initialize");
	res = client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, BUFFER_TIME_100NS,
				 0, wfex, nullptr);
	if (FAILED(res))
		throw HRError("[WASAPISource::InitRender] Failed to get initialize audio client", res);

	/* Silent loopback fix. Prevents audio stream from stopping and */
	/* messing up timestamps and other weird glitches during silence */
	/* by playing a silent sample all over again. */

	res = client->GetBufferSize(&frames);
	if (FAILED(res))
		throw HRError("[WASAPISource::InitRender] Failed to get buffer size", res);

	ThrowOnInterrupt("[WASAPISource::InitRender]: Interrupted client GetService");
	res = client->GetService(__uuidof(IAudioRenderClient),
				 (void **)render.Assign());
	if (FAILED(res))
		throw HRError("[WASAPISource::InitRender] Failed to get render client", res);

	ThrowOnInterrupt("[WASAPISource::InitRender]: Interrupted client GetBuffer");
	res = render->GetBuffer(frames, &buffer);
	if (FAILED(res))
		throw HRError("[WASAPISource::InitRender] Failed to get buffer", res);

	memset(buffer, 0, frames * wfex->nBlockAlign);

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

void WASAPISource::InitFormat(WAVEFORMATEX *wfex)
{
	DWORD layout = 0;

	if (wfex->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
		WAVEFORMATEXTENSIBLE *ext = (WAVEFORMATEXTENSIBLE *)wfex;
		layout = ext->dwChannelMask;
	}

	/* WASAPI is always float */
	sampleRate = wfex->nSamplesPerSec;
	format = AUDIO_FORMAT_FLOAT;
	speakers = ConvertSpeakerLayout(layout, wfex->nChannels);
}

void WASAPISource::InitCapture()
{
	if (client.Get() == nullptr)
		throw "[WASAPISource::InitCapture] Failed to create a valid client";

	ThrowOnInterrupt("[WASAPISource::InitCapture]: Interrupted before client GetService");
	HRESULT res = client->GetService(__uuidof(IAudioCaptureClient),
					 (void **)capture.Assign());
	if (FAILED(res))
		throw HRError("[WASAPISource::InitCapture] Failed to create capture context", res);

	ThrowOnInterrupt("[WASAPISource::InitCapture]: Interrupted before client SetEventHandle");
	res = client->SetEventHandle(receiveSignal);
	if (FAILED(res))
		throw HRError("[WASAPISource::InitCapture] Failed to set event handle", res);

	{
		std::lock_guard<std::recursive_mutex> guard(state_mutex);
		ThrowOnInterrupt("[WASAPISource::InitCapture]: Interrupted before client start");
		initializing = false;

		captureThread = CreateThread(nullptr, 0, WASAPISource::CaptureThread,
					this, 0, nullptr);
		if (!captureThread.Valid())
			throw "[WASAPISource::InitCapture] Failed to create capture thread";

		client->Start();
		active = true;
	}

	blog(LOG_INFO, "[WASAPISource::InitCapture][%08X] Device '%s' [%s Hz] initialized",
	     this, device_name.c_str(), device_sample.c_str());
}

void WASAPISource::Initialize()
{
	blog(LOG_INFO, "[WASAPISource::Initialize][%08X] Device initialize called", this);
	HRESULT res;

	res = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
			       CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
			       (void **)enumerator.Assign());
	if (FAILED(res))
		throw HRError("[WASAPISource::Initialize] Failed to create enumerator", res);

	res = InitDevice(enumerator);

	if (FAILED(res) || device.Get() == nullptr) {
		// fail early
		blog(LOG_ERROR, "[WASAPISource::Initialize][%08X] Device pointer is %p res is %d", this, device.Get(), res);
		throw HRError("[WASAPISource::Initialize] Failed to init device", res);
	}

	if (!notify) {
		notify = new WASAPINotify(this);
		enumerator->RegisterEndpointNotificationCallback(notify);
	}

	HRESULT resSample;
	IPropertyStore *store = nullptr;
	PWAVEFORMATEX deviceFormatProperties;
	PROPVARIANT prop;

	blog(LOG_INFO, "[WASAPISource::Initialize][%08X] Device '%s':'%08x' Initialization ready to call OpenPropertyStore",
	     this, device_name.c_str(), device.Get());

	ThrowOnInterrupt("[WASAPISource::Initialize]: Interrupted before device OpenPropertyStore");
	resSample = device->OpenPropertyStore(STGM_READ, &store);
	if (!FAILED(resSample)) {
		resSample =
			store->GetValue(PKEY_AudioEngine_DeviceFormat, &prop);
		if (!FAILED(resSample)) {
			if (prop.vt != VT_EMPTY && prop.blob.pBlobData) {
				deviceFormatProperties =
					(PWAVEFORMATEX)prop.blob.pBlobData;
				device_sample = std::to_string(
					deviceFormatProperties->nSamplesPerSec);
			}
		}

		store->Release();
	}

	blog(LOG_INFO, "[WASAPISource::Initialize][%08X] Device '%s':'%08x' Initialization ready to init Client",
	     this, device_name.c_str(), device.Get());

	InitClient();
	if (!isInputDevice)
		InitRender();
	InitCapture();
}

bool WASAPISource::TryInitialize()
{
	//active is true after client started capture in capturethread
	try {
		{
			std::lock_guard<std::recursive_mutex> guard(state_mutex);
			initializing = true;
		}

		Initialize();
	} catch (HRError &error) {
		if (previouslyFailed) {
			return active;
		}

		blog(LOG_WARNING, "[WASAPISource::TryInitialize][%08X]:[%s] %s: %lX", this,
				device_name.empty() ? device_id.c_str(): device_name.c_str(),
		     error.str, error.hr);
	} catch (const char *error) {
		if (previouslyFailed) {
			return active;
		}

		blog(LOG_WARNING, "[WASAPISource::TryInitialize][%08X]:[%s] %s", this, 
		     device_name.empty() ? device_id.c_str()
					 : device_name.c_str(),
		     error);
	} catch (...) {
		if (previouslyFailed) {
			return active;
		}
		blog(LOG_WARNING, "[WASAPISource::TryInitialize][%08X] Catch [%s] failed", this,
		     device_name.empty() ? device_id.c_str() : device_name.c_str());
	}
	{
		std::lock_guard<std::recursive_mutex> guard(state_mutex);	
		initializing = false;
		initInterrupted = false;
	}
	previouslyFailed = !active;
	return active;
}

void WASAPISource::Reconnect()
{
	reconnecting = true;
	reconnectThread = CreateThread(
		nullptr, 0, WASAPISource::ReconnectThread, this, 0, nullptr);

	if (!reconnectThread.Valid()) {
		blog(LOG_WARNING,
		     "[WASAPISource::Reconnect][%08X] "
		     "Failed to initialize reconnect thread: %lu",
		     this, GetLastError());
		     reconnecting = false;
	}
}

//returns false for wait timed out
static inline bool WaitForSignal(HANDLE handle, DWORD time)
{
	return WaitForSingleObject(handle, time) != WAIT_TIMEOUT;
}

#define RECONNECT_INTERVAL 3000

DWORD WINAPI WASAPISource::ReconnectThread(LPVOID param)
{
	WASAPISource *source = (WASAPISource *)param;

	os_set_thread_name("win-wasapi: reconnect thread");

	const HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
	const bool com_initialized = SUCCEEDED(hr);
	if (!com_initialized) {
		blog(LOG_ERROR,
		     "[WASAPISource::ReconnectThread][%08X]"
		     " CoInitializeEx failed: 0x%08X",
		     source, hr);
	}

	obs_monitoring_type type =
		obs_source_get_monitoring_type(source->source);
	obs_source_set_monitoring_type(source->source,
				       OBS_MONITORING_TYPE_NONE);

	while (!WaitForSignal(source->stopSignal, RECONNECT_INTERVAL)) {
		if (source->TryInitialize())
			break;
	}

	obs_source_set_monitoring_type(source->source, type);

	if (com_initialized)
		CoUninitialize();

	source->reconnectThread = nullptr;
	source->reconnecting = false;
	blog(LOG_WARNING,
		     "[WASAPISource::ReconnectThread][%08X] "
		     "Thread finished", source);
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

	while (active) {
		res = capture->GetNextPacketSize(&captureSize);

		if (FAILED(res)) {
			if (res != AUDCLNT_E_DEVICE_INVALIDATED)
				blog(LOG_WARNING,
				     "[WASAPISource::GetCaptureData][%08X]"
				     " capture->GetNextPacketSize"
				     " failed: %lX",
				     this, res);
			return false;
		}

		if (!captureSize)
			break;

		res = capture->GetBuffer(&buffer, &frames, &flags, &pos, &ts);
		if (FAILED(res)) {
			if (res != AUDCLNT_E_DEVICE_INVALIDATED)
				blog(LOG_WARNING,
				     "[WASAPISource::GetCaptureData][%08X]"
				     " capture->GetBuffer"
				     " failed: %lX",
				     this, res);
			return false;
		}

		obs_source_audio data = {0};
		data.data[0]          = (const uint8_t*)buffer;
		data.frames           = (uint32_t)frames;
		data.speakers         = speakers;
		data.samples_per_sec  = sampleRate;
		data.format           = format;
		data.timestamp        = useDeviceTiming ?
			ts*100 : os_gettime_ns();


		if (!useDeviceTiming)
			data.timestamp -= util_mul_div64(frames, 1000000000ULL,
							 sampleRate);

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
	WASAPISource *source = (WASAPISource *)param;
	bool reconnect = false;
	blog(LOG_INFO, "[WASAPISource::CaptureThread][%08X] Device '%s' capture thread started",
	     source, source->device_name.c_str());

	/* Output devices don't signal, so just make it check every 10 ms */
	DWORD dur = source->isInputDevice ? RECONNECT_INTERVAL : 10;

	HANDLE sigs[2] = {source->receiveSignal, source->stopSignal};

	os_set_thread_name("win-wasapi: capture thread");

	while (WaitForCaptureSignal(2, sigs, dur)) {
		if (!source->ProcessCaptureData()) {
			reconnect = true;
			break;
		}
	}

	source->client->Stop();

	source->captureThread = nullptr;
	source->active = false;

	if (reconnect) {
		blog(LOG_INFO, "[WASAPISource::CaptureThread][%08X] Device '%s' invalidated. Reconnect",
		     source, source->device_name.c_str());
		source->Reconnect();
	} else {
		blog(LOG_INFO, "[WASAPISource::CaptureThread][%08X] Device '%s' invalidated. Finish",
		     source, source->device_name.c_str());
	}

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

	blog(LOG_INFO, "[WASAPISource::SetDefaultDevice][%08X] Default %s device changed, name was '%s'",
	     this, isInputDevice ? "input" : "output",
	     device_name.empty() ? device_id.c_str() : device_name.c_str());

	std::lock_guard<std::recursive_mutex> guard(state_mutex);	

	if (initializing) {
		initInterrupted = true;
	} else {
		std::thread([this]() {
			Stop();
			Start();
		}).detach();
	}
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
		blog(LOG_ERROR, "[WASAPISource][CreateWASAPISource] Catch %s", error);
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
		AudioDeviceInfo &device_i = devices[i];
		obs_property_list_add_string(device_prop, device_i.name.c_str(),
					     device_i.id.c_str());
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
