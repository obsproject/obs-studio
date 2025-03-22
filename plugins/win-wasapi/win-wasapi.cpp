#include "wasapi-notify.hpp"
#include "enum-wasapi.hpp"

#include <obs-module.h>
#include <obs.h>
#include <util/dstr.h>
#include <util/platform.h>
#include <util/windows/HRError.hpp>
#include <util/windows/ComPtr.hpp>
#include <util/windows/WinHandle.hpp>
#include <util/windows/CoTaskMemPtr.hpp>
#include <util/windows/win-version.h>
#include <util/windows/window-helpers.h>
#include <util/threading.h>
#include <util/util_uint64.h>

#include <atomic>
#include <cinttypes>

#include <audioclientactivationparams.h>
#include <avrt.h>
#include <RTWorkQ.h>
#include <wrl/implements.h>

using namespace std;

#define OPT_DEVICE_ID "device_id"
#define OPT_USE_DEVICE_TIMING "use_device_timing"
#define OPT_WINDOW "window"
#define OPT_PRIORITY "priority"
#define OPT_ASYNC_COMPENSATION "async_compensation"

WASAPINotify *GetNotify();
static void GetWASAPIDefaults(obs_data_t *settings);

#define OBS_KSAUDIO_SPEAKER_4POINT1 (KSAUDIO_SPEAKER_SURROUND | SPEAKER_LOW_FREQUENCY)

typedef HRESULT(STDAPICALLTYPE *PFN_ActivateAudioInterfaceAsync)(LPCWSTR, REFIID, PROPVARIANT *,
								 IActivateAudioInterfaceCompletionHandler *,
								 IActivateAudioInterfaceAsyncOperation **);

typedef HRESULT(STDAPICALLTYPE *PFN_RtwqUnlockWorkQueue)(DWORD);
typedef HRESULT(STDAPICALLTYPE *PFN_RtwqLockSharedWorkQueue)(PCWSTR usageClass, LONG basePriority, DWORD *taskId,
							     DWORD *id);
typedef HRESULT(STDAPICALLTYPE *PFN_RtwqCreateAsyncResult)(IUnknown *, IRtwqAsyncCallback *, IUnknown *,
							   IRtwqAsyncResult **);
typedef HRESULT(STDAPICALLTYPE *PFN_RtwqPutWorkItem)(DWORD, LONG, IRtwqAsyncResult *);
typedef HRESULT(STDAPICALLTYPE *PFN_RtwqPutWaitingWorkItem)(HANDLE, LONG, IRtwqAsyncResult *, RTWQWORKITEM_KEY *);

class WASAPIActivateAudioInterfaceCompletionHandler
	: public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
					      Microsoft::WRL::FtmBase, IActivateAudioInterfaceCompletionHandler> {
	IUnknown *unknown;
	HRESULT activationResult;
	WinHandle activationSignal;

public:
	WASAPIActivateAudioInterfaceCompletionHandler();
	HRESULT GetActivateResult(IAudioClient **client);

private:
	virtual HRESULT STDMETHODCALLTYPE
	ActivateCompleted(IActivateAudioInterfaceAsyncOperation *activateOperation) override final;
};

WASAPIActivateAudioInterfaceCompletionHandler::WASAPIActivateAudioInterfaceCompletionHandler()
{
	activationSignal = CreateEvent(nullptr, false, false, nullptr);
	if (!activationSignal.Valid())
		throw "Could not create receive signal";
}

HRESULT
WASAPIActivateAudioInterfaceCompletionHandler::GetActivateResult(IAudioClient **client)
{
	WaitForSingleObject(activationSignal, INFINITE);
	*client = static_cast<IAudioClient *>(unknown);
	return activationResult;
}

HRESULT
WASAPIActivateAudioInterfaceCompletionHandler::ActivateCompleted(
	IActivateAudioInterfaceAsyncOperation *activateOperation)
{
	HRESULT hr, hr_activate;
	hr = activateOperation->GetActivateResult(&hr_activate, &unknown);
	hr = SUCCEEDED(hr) ? hr_activate : hr;
	activationResult = hr;

	SetEvent(activationSignal);
	return hr;
}

enum class SourceType {
	Input,
	DeviceOutput,
	ProcessOutput,
};

class ARtwqAsyncCallback : public IRtwqAsyncCallback {
protected:
	ARtwqAsyncCallback(void *source) : source(source) {}

public:
	STDMETHOD_(ULONG, AddRef)() { return ++refCount; }

	STDMETHOD_(ULONG, Release)() { return --refCount; }

	STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject)
	{
		HRESULT hr = E_NOINTERFACE;

		if (riid == __uuidof(IRtwqAsyncCallback) || riid == __uuidof(IUnknown)) {
			*ppvObject = this;
			AddRef();
			hr = S_OK;
		} else {
			*ppvObject = NULL;
		}

		return hr;
	}

	STDMETHOD(GetParameters)
	(DWORD *pdwFlags, DWORD *pdwQueue)
	{
		*pdwFlags = 0;
		*pdwQueue = queue_id;
		return S_OK;
	}

	STDMETHOD(Invoke)
	(IRtwqAsyncResult *) override = 0;

	DWORD GetQueueId() const { return queue_id; }
	void SetQueueId(DWORD id) { queue_id = id; }

protected:
	std::atomic<ULONG> refCount = 1;
	void *source;
	DWORD queue_id = 0;
};

class WASAPISource {
	ComPtr<IMMDeviceEnumerator> enumerator;
	ComPtr<IAudioClient> client;
	ComPtr<IAudioCaptureClient> capture;

	obs_source_t *source;
	obs_weak_source_t *reroute_target = nullptr;
	wstring default_id;
	string device_id;
	string device_name;
	WinModule mmdevapi_module;
	PFN_ActivateAudioInterfaceAsync activate_audio_interface_async = NULL;
	PFN_RtwqUnlockWorkQueue rtwq_unlock_work_queue = NULL;
	PFN_RtwqLockSharedWorkQueue rtwq_lock_shared_work_queue = NULL;
	PFN_RtwqCreateAsyncResult rtwq_create_async_result = NULL;
	PFN_RtwqPutWorkItem rtwq_put_work_item = NULL;
	PFN_RtwqPutWaitingWorkItem rtwq_put_waiting_work_item = NULL;
	bool rtwq_supported = false;
	window_priority priority;
	string window_class;
	string title;
	string executable;
	HWND hwnd = NULL;
	DWORD process_id = 0;
	const SourceType sourceType;
	std::atomic<bool> useDeviceTiming = false;
	std::atomic<bool> isDefaultDevice = false;
	std::atomic<bool> sawBadTimestamp = false;
	bool hooked = false;

	bool previouslyFailed = false;
	WinHandle reconnectThread = NULL;

	class CallbackStartCapture : public ARtwqAsyncCallback {
	public:
		CallbackStartCapture(WASAPISource *source) : ARtwqAsyncCallback(source) {}

		STDMETHOD(Invoke)
		(IRtwqAsyncResult *) override
		{
			((WASAPISource *)source)->OnStartCapture();
			return S_OK;
		}

	} startCapture;
	ComPtr<IRtwqAsyncResult> startCaptureAsyncResult;

	class CallbackSampleReady : public ARtwqAsyncCallback {
	public:
		CallbackSampleReady(WASAPISource *source) : ARtwqAsyncCallback(source) {}

		STDMETHOD(Invoke)
		(IRtwqAsyncResult *) override
		{
			((WASAPISource *)source)->OnSampleReady();
			return S_OK;
		}
	} sampleReady;
	ComPtr<IRtwqAsyncResult> sampleReadyAsyncResult;

	class CallbackRestart : public ARtwqAsyncCallback {
	public:
		CallbackRestart(WASAPISource *source) : ARtwqAsyncCallback(source) {}

		STDMETHOD(Invoke)
		(IRtwqAsyncResult *) override
		{
			((WASAPISource *)source)->OnRestart();
			return S_OK;
		}
	} restart;
	ComPtr<IRtwqAsyncResult> restartAsyncResult;

	WinHandle captureThread;
	WinHandle idleSignal;
	WinHandle stopSignal;
	WinHandle receiveSignal;
	WinHandle restartSignal;
	WinHandle reconnectExitSignal;
	WinHandle exitSignal;
	WinHandle initSignal;
	DWORD reconnectDuration = 0;
	WinHandle reconnectSignal;

	speaker_layout speakers;
	audio_format format;
	uint32_t sampleRate;

	vector<BYTE> silence;

	static DWORD WINAPI ReconnectThread(LPVOID param);
	static DWORD WINAPI CaptureThread(LPVOID param);

	bool ProcessCaptureData();

	void Start();
	void Stop();

	static ComPtr<IMMDevice> InitDevice(IMMDeviceEnumerator *enumerator, bool isDefaultDevice, SourceType type,
					    const string device_id);
	static ComPtr<IAudioClient> InitClient(IMMDevice *device, SourceType type, DWORD process_id,
					       PFN_ActivateAudioInterfaceAsync activate_audio_interface_async,
					       speaker_layout &speakers, audio_format &format, uint32_t &sampleRate);
	static void InitFormat(const WAVEFORMATEX *wfex, enum speaker_layout &speakers, enum audio_format &format,
			       uint32_t &sampleRate);
	static void ClearBuffer(IMMDevice *device);
	static ComPtr<IAudioCaptureClient> InitCapture(IAudioClient *client, HANDLE receiveSignal);
	void Initialize();

	bool TryInitialize();

	struct UpdateParams {
		string device_id;
		bool useDeviceTiming;
		bool isDefaultDevice;
		bool async_compensation;
		window_priority priority;
		string window_class;
		string title;
		string executable;
	};

	UpdateParams BuildUpdateParams(obs_data_t *settings);
	void UpdateSettings(UpdateParams &&params);
	void LogSettings();

public:
	WASAPISource(obs_data_t *settings, obs_source_t *source_, SourceType type);
	~WASAPISource();

	void Update(obs_data_t *settings);
	void OnWindowChanged(obs_data_t *settings);

	void Activate();
	void Deactivate();

	void SetDefaultDevice(EDataFlow flow, ERole role, LPCWSTR id);

	void OnStartCapture();
	void OnSampleReady();
	void OnRestart();

	bool GetHooked();
	HWND GetHwnd();

	void SetRerouteTarget(obs_source_t *target)
	{
		obs_weak_source_release(reroute_target);
		reroute_target = obs_source_get_weak_source(target);
	}
};

WASAPISource::WASAPISource(obs_data_t *settings, obs_source_t *source_, SourceType type)
	: source(source_),
	  sourceType(type),
	  startCapture(this),
	  sampleReady(this),
	  restart(this)
{
	mmdevapi_module = LoadLibrary(L"Mmdevapi");
	if (mmdevapi_module) {
		activate_audio_interface_async =
			(PFN_ActivateAudioInterfaceAsync)GetProcAddress(mmdevapi_module, "ActivateAudioInterfaceAsync");
	}

	UpdateSettings(BuildUpdateParams(settings));
	LogSettings();

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

	reconnectExitSignal = CreateEvent(nullptr, true, false, nullptr);
	if (!reconnectExitSignal.Valid())
		throw "Could not create reconnect exit signal";

	exitSignal = CreateEvent(nullptr, true, false, nullptr);
	if (!exitSignal.Valid())
		throw "Could not create exit signal";

	initSignal = CreateEvent(nullptr, false, false, nullptr);
	if (!initSignal.Valid())
		throw "Could not create init signal";

	reconnectSignal = CreateEvent(nullptr, false, false, nullptr);
	if (!reconnectSignal.Valid())
		throw "Could not create reconnect signal";

	HRESULT hr =
		CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(enumerator.Assign()));
	if (FAILED(hr))
		throw HRError("Failed to create enumerator", hr);

	/* OBS will already load DLL on startup if it exists */
	const HMODULE rtwq_module = GetModuleHandle(L"RTWorkQ.dll");

	// while RTWQ was introduced in Win 8.1, it silently fails
	// to capture Desktop Audio for some reason. Disable for now.
	struct win_version_info win1703 = {};
	win1703.major = 10;
	win1703.minor = 0;
	win1703.build = 15063;
	win1703.revis = 0;
	struct win_version_info ver;
	get_win_ver(&ver);
	if (win_version_compare(&ver, &win1703) >= 0)
		rtwq_supported = rtwq_module != NULL;

	if (rtwq_supported) {
		rtwq_unlock_work_queue = (PFN_RtwqUnlockWorkQueue)GetProcAddress(rtwq_module, "RtwqUnlockWorkQueue");
		rtwq_lock_shared_work_queue =
			(PFN_RtwqLockSharedWorkQueue)GetProcAddress(rtwq_module, "RtwqLockSharedWorkQueue");
		rtwq_create_async_result =
			(PFN_RtwqCreateAsyncResult)GetProcAddress(rtwq_module, "RtwqCreateAsyncResult");
		rtwq_put_work_item = (PFN_RtwqPutWorkItem)GetProcAddress(rtwq_module, "RtwqPutWorkItem");
		rtwq_put_waiting_work_item =
			(PFN_RtwqPutWaitingWorkItem)GetProcAddress(rtwq_module, "RtwqPutWaitingWorkItem");

		try {
			hr = rtwq_create_async_result(nullptr, &startCapture, nullptr, &startCaptureAsyncResult);
			if (FAILED(hr)) {
				throw HRError("Could not create startCaptureAsyncResult", hr);
			}

			hr = rtwq_create_async_result(nullptr, &sampleReady, nullptr, &sampleReadyAsyncResult);
			if (FAILED(hr)) {
				throw HRError("Could not create sampleReadyAsyncResult", hr);
			}

			hr = rtwq_create_async_result(nullptr, &restart, nullptr, &restartAsyncResult);
			if (FAILED(hr)) {
				throw HRError("Could not create restartAsyncResult", hr);
			}

			DWORD taskId = 0;
			DWORD id = 0;
			hr = rtwq_lock_shared_work_queue(L"Capture", 0, &taskId, &id);
			if (FAILED(hr)) {
				throw HRError("RtwqLockSharedWorkQueue failed", hr);
			}

			startCapture.SetQueueId(id);
			sampleReady.SetQueueId(id);
			restart.SetQueueId(id);
		} catch (HRError &err) {
			blog(LOG_ERROR, "RTWQ setup failed: %s (0x%08X)", err.str, err.hr);
			rtwq_supported = false;
		}
	}

	if (!rtwq_supported) {
		captureThread = CreateThread(nullptr, 0, WASAPISource::CaptureThread, this, 0, nullptr);
		if (!captureThread.Valid()) {
			throw "Failed to create capture thread";
		}
	}

	auto notify = GetNotify();
	if (notify) {
		notify->AddDefaultDeviceChangedCallback(this, std::bind(&WASAPISource::SetDefaultDevice, this,
									std::placeholders::_1, std::placeholders::_2,
									std::placeholders::_3));
	}

	Start();
}

void WASAPISource::Start()
{
	if (rtwq_supported) {
		rtwq_put_work_item(startCapture.GetQueueId(), 0, startCaptureAsyncResult);
	} else {
		SetEvent(initSignal);
	}
}

void WASAPISource::Stop()
{
	SetEvent(stopSignal);

	blog(LOG_INFO, "WASAPI: Device '%s' Terminated", device_name.c_str());

	if (rtwq_supported)
		SetEvent(receiveSignal);

	if (reconnectThread.Valid()) {
		WaitForSingleObject(idleSignal, INFINITE);
	} else {
		const HANDLE sigs[] = {reconnectSignal, idleSignal};
		WaitForMultipleObjects(_countof(sigs), sigs, false, INFINITE);
	}

	SetEvent(exitSignal);

	if (reconnectThread.Valid()) {
		SetEvent(reconnectExitSignal);
		WaitForSingleObject(reconnectThread, INFINITE);
	}

	if (rtwq_supported)
		rtwq_unlock_work_queue(sampleReady.GetQueueId());
	else
		WaitForSingleObject(captureThread, INFINITE);

	obs_weak_source_release(reroute_target);
}

WASAPISource::~WASAPISource()
{
	auto notify = GetNotify();
	if (notify) {
		notify->RemoveDefaultDeviceChangedCallback(this);
	}

	Stop();
}

WASAPISource::UpdateParams WASAPISource::BuildUpdateParams(obs_data_t *settings)
{
	WASAPISource::UpdateParams params;
	params.device_id = obs_data_get_string(settings, OPT_DEVICE_ID);
	params.useDeviceTiming = obs_data_get_bool(settings, OPT_USE_DEVICE_TIMING);
	params.isDefaultDevice = _strcmpi(params.device_id.c_str(), "default") == 0;
	params.async_compensation = obs_data_get_bool(settings, OPT_ASYNC_COMPENSATION);
	params.priority = (window_priority)obs_data_get_int(settings, "priority");
	params.window_class.clear();
	params.title.clear();
	params.executable.clear();
	if (sourceType != SourceType::Input) {
		const char *const window = obs_data_get_string(settings, OPT_WINDOW);
		char *window_class = nullptr;
		char *title = nullptr;
		char *executable = nullptr;
		ms_build_window_strings(window, &window_class, &title, &executable);
		if (window_class) {
			params.window_class = window_class;
			bfree(window_class);
		}
		if (title) {
			params.title = title;
			bfree(title);
		}
		if (executable) {
			params.executable = executable;
			bfree(executable);
		}
	}

	return params;
}

void WASAPISource::UpdateSettings(UpdateParams &&params)
{
	device_id = std::move(params.device_id);
	useDeviceTiming = params.useDeviceTiming;
	isDefaultDevice = params.isDefaultDevice;
	priority = params.priority;
	window_class = std::move(params.window_class);
	title = std::move(params.title);
	executable = std::move(params.executable);

	obs_source_set_async_compensation(source, params.async_compensation & !params.useDeviceTiming);
}

void WASAPISource::LogSettings()
{
	if (sourceType == SourceType::ProcessOutput) {
		blog(LOG_INFO,
		     "[win-wasapi: '%s'] update settings:\n"
		     "\texecutable: %s\n"
		     "\ttitle: %s\n"
		     "\tclass: %s\n"
		     "\tpriority: %d",
		     obs_source_get_name(source), executable.c_str(), title.c_str(), window_class.c_str(),
		     (int)priority);
	} else {
		blog(LOG_INFO,
		     "[win-wasapi: '%s'] update settings:\n"
		     "\tdevice id: %s\n"
		     "\tuse device timing: %d",
		     obs_source_get_name(source), device_id.c_str(), (int)useDeviceTiming);
	}
}

void WASAPISource::Update(obs_data_t *settings)
{
	UpdateParams params = BuildUpdateParams(settings);

	const bool restart = (sourceType == SourceType::ProcessOutput)
				     ? ((priority != params.priority) || (window_class != params.window_class) ||
					(title != params.title) || (executable != params.executable))
				     : (device_id.compare(params.device_id) != 0);

	UpdateSettings(std::move(params));
	LogSettings();

	if (restart)
		SetEvent(restartSignal);
}

void WASAPISource::OnWindowChanged(obs_data_t *settings)
{
	UpdateParams params = BuildUpdateParams(settings);

	const bool restart = (sourceType == SourceType::ProcessOutput)
				     ? ((priority != params.priority) || (window_class != params.window_class) ||
					(title != params.title) || (executable != params.executable))
				     : (device_id.compare(params.device_id) != 0);

	UpdateSettings(std::move(params));

	if (restart)
		SetEvent(restartSignal);
}

void WASAPISource::Activate()
{
	if (!reconnectThread.Valid()) {
		ResetEvent(reconnectExitSignal);
		reconnectThread = CreateThread(nullptr, 0, WASAPISource::ReconnectThread, this, 0, nullptr);
	}
}

void WASAPISource::Deactivate()
{
	if (reconnectThread.Valid()) {
		SetEvent(reconnectExitSignal);
		WaitForSingleObject(reconnectThread, INFINITE);
		reconnectThread = NULL;
	}
}

ComPtr<IMMDevice> WASAPISource::InitDevice(IMMDeviceEnumerator *enumerator, bool isDefaultDevice, SourceType type,
					   const string device_id)
{
	ComPtr<IMMDevice> device;

	if (isDefaultDevice) {
		const bool input = type == SourceType::Input;
		HRESULT res = enumerator->GetDefaultAudioEndpoint(input ? eCapture : eRender,
								  input ? eCommunications : eConsole, device.Assign());
		if (FAILED(res))
			throw HRError("Failed GetDefaultAudioEndpoint", res);
	} else {
		wchar_t *w_id;
		os_utf8_to_wcs_ptr(device_id.c_str(), device_id.size(), &w_id);
		if (!w_id)
			throw "Failed to widen device id string";

		const HRESULT res = enumerator->GetDevice(w_id, device.Assign());

		bfree(w_id);

		if (FAILED(res))
			throw HRError("Failed to enumerate device", res);
	}

	return device;
}

#define BUFFER_TIME_100NS (5 * 10000000)

static DWORD GetSpeakerChannelMask(speaker_layout layout)
{
	switch (layout) {
	case SPEAKERS_STEREO:
		return KSAUDIO_SPEAKER_STEREO;
	case SPEAKERS_2POINT1:
		return KSAUDIO_SPEAKER_2POINT1;
	case SPEAKERS_4POINT0:
		return KSAUDIO_SPEAKER_SURROUND;
	case SPEAKERS_4POINT1:
		return OBS_KSAUDIO_SPEAKER_4POINT1;
	case SPEAKERS_5POINT1:
		return KSAUDIO_SPEAKER_5POINT1_SURROUND;
	case SPEAKERS_7POINT1:
		return KSAUDIO_SPEAKER_7POINT1_SURROUND;
	}

	return (DWORD)layout;
}

ComPtr<IAudioClient> WASAPISource::InitClient(IMMDevice *device, SourceType type, DWORD process_id,
					      PFN_ActivateAudioInterfaceAsync activate_audio_interface_async,
					      speaker_layout &speakers, audio_format &format, uint32_t &samples_per_sec)
{
	WAVEFORMATEXTENSIBLE wfextensible;
	CoTaskMemPtr<WAVEFORMATEX> wfex;
	const WAVEFORMATEX *pFormat;
	HRESULT res;
	ComPtr<IAudioClient> client;

	if (type == SourceType::ProcessOutput) {
		if (activate_audio_interface_async == NULL)
			throw "ActivateAudioInterfaceAsync is not available";

		struct obs_audio_info oai;
		obs_get_audio_info(&oai);

		const WORD nChannels = (WORD)get_audio_channels(oai.speakers);
		const DWORD nSamplesPerSec = oai.samples_per_sec;
		constexpr WORD wBitsPerSample = 32;
		const WORD nBlockAlign = nChannels * wBitsPerSample / 8;

		WAVEFORMATEX &wf = wfextensible.Format;
		wf.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		wf.nChannels = nChannels;
		wf.nSamplesPerSec = nSamplesPerSec;
		wf.nAvgBytesPerSec = nSamplesPerSec * nBlockAlign;
		wf.nBlockAlign = nBlockAlign;
		wf.wBitsPerSample = wBitsPerSample;
		wf.cbSize = sizeof(wfextensible) - sizeof(wf);
		wfextensible.Samples.wValidBitsPerSample = wBitsPerSample;
		wfextensible.dwChannelMask = GetSpeakerChannelMask(oai.speakers);
		wfextensible.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

		AUDIOCLIENT_ACTIVATION_PARAMS audioclientActivationParams;
		audioclientActivationParams.ActivationType = AUDIOCLIENT_ACTIVATION_TYPE_PROCESS_LOOPBACK;
		audioclientActivationParams.ProcessLoopbackParams.TargetProcessId = process_id;
		audioclientActivationParams.ProcessLoopbackParams.ProcessLoopbackMode =
			PROCESS_LOOPBACK_MODE_INCLUDE_TARGET_PROCESS_TREE;
		PROPVARIANT activateParams{};
		activateParams.vt = VT_BLOB;
		activateParams.blob.cbSize = sizeof(audioclientActivationParams);
		activateParams.blob.pBlobData = reinterpret_cast<BYTE *>(&audioclientActivationParams);

		{
			Microsoft::WRL::ComPtr<WASAPIActivateAudioInterfaceCompletionHandler> handler =
				Microsoft::WRL::Make<WASAPIActivateAudioInterfaceCompletionHandler>();
			ComPtr<IActivateAudioInterfaceAsyncOperation> asyncOp;
			res = activate_audio_interface_async(VIRTUAL_AUDIO_DEVICE_PROCESS_LOOPBACK,
							     __uuidof(IAudioClient), &activateParams, handler.Get(),
							     &asyncOp);
			if (FAILED(res))
				throw HRError("Failed to get activate audio client", res);

			res = handler->GetActivateResult(client.Assign());
			if (FAILED(res))
				throw HRError("Async activation failed", res);
		}

		pFormat = &wf;
	} else {
		res = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void **)client.Assign());
		if (FAILED(res))
			throw HRError("Failed to activate client context", res);

		res = client->GetMixFormat(&wfex);
		if (FAILED(res))
			throw HRError("Failed to get mix format", res);

		pFormat = wfex.Get();
	}

	InitFormat(pFormat, speakers, format, samples_per_sec);

	DWORD flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
	if (type != SourceType::Input)
		flags |= AUDCLNT_STREAMFLAGS_LOOPBACK;
	res = client->Initialize(AUDCLNT_SHAREMODE_SHARED, flags, BUFFER_TIME_100NS, 0, pFormat, nullptr);
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

	res = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void **)client.Assign());
	if (FAILED(res))
		throw HRError("Failed to activate client context", res);

	res = client->GetMixFormat(&wfex);
	if (FAILED(res))
		throw HRError("Failed to get mix format", res);

	res = client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, BUFFER_TIME_100NS, 0, wfex, nullptr);
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

void WASAPISource::InitFormat(const WAVEFORMATEX *wfex, enum speaker_layout &speakers, enum audio_format &format,
			      uint32_t &sampleRate)
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

ComPtr<IAudioCaptureClient> WASAPISource::InitCapture(IAudioClient *client, HANDLE receiveSignal)
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
	ComPtr<IMMDevice> device;
	if (sourceType == SourceType::ProcessOutput) {
		device_name = "[VIRTUAL_AUDIO_DEVICE_PROCESS_LOOPBACK]";

		hwnd = ms_find_window(INCLUDE_MINIMIZED, priority, window_class.c_str(), title.c_str(),
				      executable.c_str());
		if (!hwnd)
			throw "Failed to find window";

		DWORD dwProcessId = 0;
		if (!GetWindowThreadProcessId(hwnd, &dwProcessId)) {
			hwnd = NULL;
			throw "Failed to get process id of window";
		}

		process_id = dwProcessId;
	} else {
		device = InitDevice(enumerator, isDefaultDevice, sourceType, device_id);

		device_name = GetDeviceName(device);
	}

	ResetEvent(receiveSignal);

	ComPtr<IAudioClient> temp_client = InitClient(device, sourceType, process_id, activate_audio_interface_async,
						      speakers, format, sampleRate);
	if (sourceType == SourceType::DeviceOutput)
		ClearBuffer(device);
	ComPtr<IAudioCaptureClient> temp_capture = InitCapture(temp_client, receiveSignal);

	client = std::move(temp_client);
	capture = std::move(temp_capture);

	if (rtwq_supported) {
		HRESULT hr = rtwq_put_waiting_work_item(receiveSignal, 0, sampleReadyAsyncResult, nullptr);
		if (FAILED(hr)) {
			capture.Clear();
			client.Clear();
			throw HRError("RtwqPutWaitingWorkItem failed", hr);
		}

		hr = rtwq_put_waiting_work_item(restartSignal, 0, restartAsyncResult, nullptr);
		if (FAILED(hr)) {
			capture.Clear();
			client.Clear();
			throw HRError("RtwqPutWaitingWorkItem failed", hr);
		}
	}

	blog(LOG_INFO, "WASAPI: Device '%s' [%" PRIu32 " Hz] initialized (source: %s)", device_name.c_str(), sampleRate,
	     obs_source_get_name(source));

	if (sourceType == SourceType::ProcessOutput && !hooked) {
		hooked = true;

		signal_handler_t *sh = obs_source_get_signal_handler(source);
		calldata_t data = {0};
		struct dstr title = {0};
		struct dstr window_class = {0};
		struct dstr executable = {0};

		ms_get_window_title(&title, hwnd);
		ms_get_window_class(&window_class, hwnd);
		ms_get_window_exe(&executable, hwnd);

		calldata_set_ptr(&data, "source", source);
		calldata_set_string(&data, "title", title.array);
		calldata_set_string(&data, "class", window_class.array);
		calldata_set_string(&data, "executable", executable.array);
		signal_handler_signal(sh, "hooked", &data);

		dstr_free(&title);
		dstr_free(&window_class);
		dstr_free(&executable);
		calldata_free(&data);
	}
}

bool WASAPISource::TryInitialize()
{
	bool success = false;
	try {
		Initialize();
		success = true;

	} catch (HRError &error) {
		if (!previouslyFailed) {
			blog(LOG_WARNING, "[WASAPISource::TryInitialize]:[%s] %s: %lX",
			     device_name.empty() ? device_id.c_str() : device_name.c_str(), error.str, error.hr);
		}
	} catch (const char *error) {
		if (!previouslyFailed) {
			blog(LOG_WARNING, "[WASAPISource::TryInitialize]:[%s] %s",
			     device_name.empty() ? device_id.c_str() : device_name.c_str(), error);
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
		source->reconnectExitSignal,
		source->reconnectSignal,
	};

	const HANDLE reconnect_sigs[] = {
		source->reconnectExitSignal,
		source->stopSignal,
	};

	bool exit = false;
	while (!exit) {
		const DWORD ret = WaitForMultipleObjects(_countof(sigs), sigs, false, INFINITE);
		switch (ret) {
		case WAIT_OBJECT_0:
			exit = true;
			break;
		default:
			assert(ret == (WAIT_OBJECT_0 + 1));
			if (source->reconnectDuration > 0) {
				WaitForMultipleObjects(_countof(reconnect_sigs), reconnect_sigs, false,
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
		if ((sourceType == SourceType::ProcessOutput) && !IsWindow(hwnd)) {
			blog(LOG_WARNING, "[WASAPISource::ProcessCaptureData] window disappeared");
			return false;
		}

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

		if (!sawBadTimestamp && flags & AUDCLNT_BUFFERFLAGS_TIMESTAMP_ERROR) {
			blog(LOG_WARNING, "[WASAPISource::ProcessCaptureData]"
					  " Timestamp error!");
			sawBadTimestamp = true;
		}

		if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
			/* buffer size = frame size * number of frames
			 * frame size = channels * sample size
			 * sample size = 4 bytes (always float per InitFormat) */
			uint32_t requiredBufSize = get_audio_channels(speakers) * frames * 4;
			if (silence.size() < requiredBufSize)
				silence.resize(requiredBufSize);

			buffer = silence.data();
		}

		obs_source_audio data = {};
		data.data[0] = buffer;
		data.frames = frames;
		data.speakers = speakers;
		data.samples_per_sec = sampleRate;
		data.format = format;
		if (sourceType == SourceType::ProcessOutput) {
			data.timestamp = ts * 100;
		} else {
			data.timestamp = useDeviceTiming ? ts * 100 : os_gettime_ns();

			if (!useDeviceTiming)
				data.timestamp -= util_mul_div64(frames, UINT64_C(1000000000), sampleRate);
		}

		if (reroute_target) {
			obs_source_t *target = obs_weak_source_get_source(reroute_target);

			if (target) {
				obs_source_output_audio(target, &data);
				obs_source_release(target);
			}
		} else {
			obs_source_output_audio(source, &data);
		}

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
			const DWORD dwMilliseconds =
				((sigs == active_sigs) && (source->sourceType != SourceType::Input)) ? 10 : INFINITE;

			const DWORD ret = WaitForMultipleObjects(sig_count, sigs, false, dwMilliseconds);
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
						sig_count = _countof(active_sigs);
						sigs = active_sigs;
					} else {
						if (source->reconnectDuration == 0) {
							blog(LOG_INFO,
							     "WASAPI: Device '%s' failed to start (source: %s)",
							     source->device_id.c_str(),
							     obs_source_get_name(source->source));
						}
						stop = true;
						reconnect = true;
						source->reconnectDuration = RECONNECT_INTERVAL;
					}
				} else {
					stop = !source->ProcessCaptureData();
					if (stop) {
						blog(LOG_INFO, "Device '%s' invalidated.  Retrying (source: %s)",
						     source->device_name.c_str(), obs_source_get_name(source->source));
						if (source->sourceType == SourceType::ProcessOutput && source->hooked) {
							source->hooked = false;
							signal_handler_t *sh =
								obs_source_get_signal_handler(source->source);
							calldata_t data = {0};
							calldata_set_ptr(&data, "source", source->source);
							signal_handler_signal(sh, "unhooked", &data);
							calldata_free(&data);
						}
						stop = true;
						reconnect = true;
						source->reconnectDuration = RECONNECT_INTERVAL;
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
			blog(LOG_INFO, "Device '%s' invalidated.  Retrying (source: %s)", source->device_name.c_str(),
			     obs_source_get_name(source->source));
			if (source->sourceType == SourceType::ProcessOutput && source->hooked) {
				source->hooked = false;
				signal_handler_t *sh = obs_source_get_signal_handler(source->source);
				calldata_t data = {0};
				calldata_set_ptr(&data, "source", source->source);
				signal_handler_signal(sh, "unhooked", &data);
				calldata_free(&data);
			}
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

	const bool input = sourceType == SourceType::Input;
	const EDataFlow expectedFlow = input ? eCapture : eRender;
	const ERole expectedRole = input ? eCommunications : eConsole;
	if (flow != expectedFlow || role != expectedRole)
		return;

	if (id) {
		if (default_id.compare(id) == 0)
			return;
		default_id = id;
	} else {
		if (default_id.empty())
			return;
		default_id.clear();
	}

	blog(LOG_INFO, "WASAPI: Default %s device changed", input ? "input" : "output");

	SetEvent(restartSignal);
}

void WASAPISource::OnStartCapture()
{
	const DWORD ret = WaitForSingleObject(stopSignal, 0);
	switch (ret) {
	case WAIT_OBJECT_0:
		SetEvent(idleSignal);
		break;

	default:
		assert(ret == WAIT_TIMEOUT);

		if (!TryInitialize()) {
			if (reconnectDuration == 0) {
				blog(LOG_INFO, "WASAPI: Device '%s' failed to start (source: %s)", device_id.c_str(),
				     obs_source_get_name(source));
			}
			reconnectDuration = RECONNECT_INTERVAL;
			SetEvent(reconnectSignal);
		}
	}
}

void WASAPISource::OnSampleReady()
{
	bool stop = false;
	bool reconnect = false;

	if (!ProcessCaptureData()) {
		stop = true;
		reconnect = true;
		reconnectDuration = RECONNECT_INTERVAL;
	}

	if (WaitForSingleObject(restartSignal, 0) == WAIT_OBJECT_0) {
		stop = true;
		reconnect = true;
		reconnectDuration = 0;

		ResetEvent(restartSignal);
		rtwq_put_waiting_work_item(restartSignal, 0, restartAsyncResult, nullptr);
	}

	if (WaitForSingleObject(stopSignal, 0) == WAIT_OBJECT_0) {
		stop = true;
		reconnect = false;
	}

	if (!stop) {
		if (FAILED(rtwq_put_waiting_work_item(receiveSignal, 0, sampleReadyAsyncResult, nullptr))) {
			blog(LOG_ERROR, "Could not requeue sample receive work");
			stop = true;
			reconnect = true;
			reconnectDuration = RECONNECT_INTERVAL;
		}
	}

	if (stop) {
		client->Stop();

		capture.Clear();
		client.Clear();

		if (reconnect) {
			blog(LOG_INFO, "Device '%s' invalidated.  Retrying (source: %s)", device_name.c_str(),
			     obs_source_get_name(source));
			SetEvent(reconnectSignal);
			if (sourceType == SourceType::ProcessOutput && hooked) {
				hooked = false;
				signal_handler_t *sh = obs_source_get_signal_handler(source);
				calldata_t data = {0};
				calldata_set_ptr(&data, "source", source);
				signal_handler_signal(sh, "unhooked", &data);
				calldata_free(&data);
			}
		} else {
			SetEvent(idleSignal);
		}
	}
}

void WASAPISource::OnRestart()
{
	SetEvent(receiveSignal);
}

bool WASAPISource::GetHooked()
{
	return hooked;
}

HWND WASAPISource::GetHwnd()
{
	return hwnd;
}

/* ------------------------------------------------------------------------- */

static const char *GetWASAPIInputName(void *)
{
	return obs_module_text("AudioInput");
}

static const char *GetWASAPIDeviceOutputName(void *)
{
	return obs_module_text("AudioOutput");
}

static const char *GetWASAPIProcessOutputName(void *)
{
	return obs_module_text("ApplicationAudioCapture");
}

static void GetWASAPIDefaultsInput(obs_data_t *settings)
{
	obs_data_set_default_string(settings, OPT_DEVICE_ID, "default");
	obs_data_set_default_bool(settings, OPT_USE_DEVICE_TIMING, false);
	obs_data_set_default_bool(settings, OPT_ASYNC_COMPENSATION, true);
}

static void GetWASAPIDefaultsDeviceOutput(obs_data_t *settings)
{
	obs_data_set_default_string(settings, OPT_DEVICE_ID, "default");
	obs_data_set_default_bool(settings, OPT_USE_DEVICE_TIMING, true);
}

static void GetWASAPIDefaultsProcessOutput(obs_data_t *) {}

static void wasapi_get_hooked(void *data, calldata_t *cd)
{
	WASAPISource *wasapi_source = reinterpret_cast<WASAPISource *>(data);

	if (!wasapi_source)
		return;

	bool hooked = wasapi_source->GetHooked();
	HWND hwnd = wasapi_source->GetHwnd();

	if (hooked && hwnd) {
		calldata_set_bool(cd, "hooked", true);
		struct dstr title = {0};
		struct dstr window_class = {0};
		struct dstr executable = {0};
		ms_get_window_title(&title, hwnd);
		ms_get_window_class(&window_class, hwnd);
		ms_get_window_exe(&executable, hwnd);
		calldata_set_string(cd, "title", title.array);
		calldata_set_string(cd, "class", window_class.array);
		calldata_set_string(cd, "executable", executable.array);
		dstr_free(&title);
		dstr_free(&window_class);
		dstr_free(&executable);
	} else {
		calldata_set_bool(cd, "hooked", false);
		calldata_set_string(cd, "title", "");
		calldata_set_string(cd, "class", "");
		calldata_set_string(cd, "executable", "");
	}
}

static void wasapi_reroute_audio(void *data, calldata_t *cd)
{
	auto wasapi_source = static_cast<WASAPISource *>(data);
	if (!wasapi_source)
		return;

	obs_source_t *target = nullptr;
	calldata_get_ptr(cd, "target", &target);
	wasapi_source->SetRerouteTarget(target);
}

static void *CreateWASAPISource(obs_data_t *settings, obs_source_t *source, SourceType type)
{
	try {
		if (type != SourceType::ProcessOutput) {
			return new WASAPISource(settings, source, type);
		} else {
			WASAPISource *wasapi_source = new WASAPISource(settings, source, type);

			if (wasapi_source) {
				signal_handler_t *sh = obs_source_get_signal_handler(source);
				signal_handler_add(sh, "void unhooked(ptr source)");
				signal_handler_add(
					sh, "void hooked(ptr source, string title, string class, string executable)");

				proc_handler_t *ph = obs_source_get_proc_handler(source);
				proc_handler_add(
					ph,
					"void get_hooked(out bool hooked, out string title, out string class, out string executable)",
					wasapi_get_hooked, wasapi_source);
				proc_handler_add(ph, "void reroute_audio(in ptr target)", wasapi_reroute_audio,
						 wasapi_source);
			}
			return wasapi_source;
		}
	} catch (const char *error) {
		blog(LOG_ERROR, "[CreateWASAPISource] %s", error);
	}

	return nullptr;
}

static void *CreateWASAPIInput(obs_data_t *settings, obs_source_t *source)
{
	return CreateWASAPISource(settings, source, SourceType::Input);
}

static void *CreateWASAPIDeviceOutput(obs_data_t *settings, obs_source_t *source)
{
	return CreateWASAPISource(settings, source, SourceType::DeviceOutput);
}

static void *CreateWASAPIProcessOutput(obs_data_t *settings, obs_source_t *source)
{
	return CreateWASAPISource(settings, source, SourceType::ProcessOutput);
}

static void DestroyWASAPISource(void *obj)
{
	delete static_cast<WASAPISource *>(obj);
}

static void UpdateWASAPISource(void *obj, obs_data_t *settings)
{
	static_cast<WASAPISource *>(obj)->Update(settings);
}

static void ActivateWASAPISource(void *obj)
{
	static_cast<WASAPISource *>(obj)->Activate();
}

static void DeactivateWASAPISource(void *obj)
{
	static_cast<WASAPISource *>(obj)->Deactivate();
}

static bool UpdateWASAPIMethod(obs_properties_t *props, obs_property_t *, obs_data_t *settings)
{
	WASAPISource *source = (WASAPISource *)obs_properties_get_param(props);
	if (!source)
		return false;

	source->Update(settings);

	return true;
}

static bool device_timing_changed(obs_properties_t *props, obs_property_t *property, obs_data_t *settings)
{
	bool useDeviceTiming = obs_data_get_bool(settings, OPT_USE_DEVICE_TIMING);

	obs_property_t *prop = obs_properties_get(props, OPT_ASYNC_COMPENSATION);
	obs_property_set_enabled(prop, !useDeviceTiming);

	return true;
}

static obs_properties_t *GetWASAPIPropertiesInput(void *)
{
	obs_properties_t *props = obs_properties_create();
	vector<AudioDeviceInfo> devices;

	obs_property_t *device_prop = obs_properties_add_list(props, OPT_DEVICE_ID, obs_module_text("Device"),
							      OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	GetWASAPIAudioDevices(devices, true);

	if (devices.size())
		obs_property_list_add_string(device_prop, obs_module_text("Default"), "default");

	for (size_t i = 0; i < devices.size(); i++) {
		AudioDeviceInfo &device = devices[i];
		obs_property_list_add_string(device_prop, device.name.c_str(), device.id.c_str());
	}

	obs_property_t *device_timing_prop =
		obs_properties_add_bool(props, OPT_USE_DEVICE_TIMING, obs_module_text("UseDeviceTiming"));

	obs_property_set_modified_callback(device_timing_prop, device_timing_changed);

	obs_properties_add_bool(props, OPT_ASYNC_COMPENSATION, obs_module_text("AsyncCompensation"));

	return props;
}

static obs_properties_t *GetWASAPIPropertiesDeviceOutput(void *)
{
	obs_properties_t *props = obs_properties_create();
	vector<AudioDeviceInfo> devices;

	obs_property_t *device_prop = obs_properties_add_list(props, OPT_DEVICE_ID, obs_module_text("Device"),
							      OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	GetWASAPIAudioDevices(devices, false);

	if (devices.size())
		obs_property_list_add_string(device_prop, obs_module_text("Default"), "default");

	for (size_t i = 0; i < devices.size(); i++) {
		AudioDeviceInfo &device = devices[i];
		obs_property_list_add_string(device_prop, device.name.c_str(), device.id.c_str());
	}

	obs_property_t *device_timing_prop =
		obs_properties_add_bool(props, OPT_USE_DEVICE_TIMING, obs_module_text("UseDeviceTiming"));

	obs_property_set_modified_callback(device_timing_prop, device_timing_changed);

	obs_properties_add_bool(props, OPT_ASYNC_COMPENSATION, obs_module_text("AsyncCompensation"));

	return props;
}

static bool wasapi_window_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	WASAPISource *source = (WASAPISource *)obs_properties_get_param(props);
	if (!source)
		return false;

	source->OnWindowChanged(settings);

	ms_check_window_property_setting(props, p, settings, "window", 0);
	return true;
}

static obs_properties_t *GetWASAPIPropertiesProcessOutput(void *data)
{
	obs_properties_t *props = obs_properties_create();
	obs_properties_set_param(props, data, NULL);

	obs_property_t *const window_prop = obs_properties_add_list(props, OPT_WINDOW, obs_module_text("Window"),
								    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	ms_fill_window_list(window_prop, INCLUDE_MINIMIZED, nullptr);
	obs_property_set_modified_callback(window_prop, wasapi_window_changed);

	obs_property_t *const priority_prop = obs_properties_add_list(props, OPT_PRIORITY, obs_module_text("Priority"),
								      OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(priority_prop, obs_module_text("Priority.Title"), WINDOW_PRIORITY_TITLE);
	obs_property_list_add_int(priority_prop, obs_module_text("Priority.Class"), WINDOW_PRIORITY_CLASS);
	obs_property_list_add_int(priority_prop, obs_module_text("Priority.Exe"), WINDOW_PRIORITY_EXE);

	obs_properties_add_bool(props, OPT_ASYNC_COMPENSATION, obs_module_text("AsyncCompensation"));

	return props;
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
	info.activate = ActivateWASAPISource;
	info.deactivate = DeactivateWASAPISource;
	info.get_defaults = GetWASAPIDefaultsInput;
	info.get_properties = GetWASAPIPropertiesInput;
	info.icon_type = OBS_ICON_TYPE_AUDIO_INPUT;
	obs_register_source(&info);
}

void RegisterWASAPIDeviceOutput()
{
	obs_source_info info = {};
	info.id = "wasapi_output_capture";
	info.type = OBS_SOURCE_TYPE_INPUT;
	info.output_flags = OBS_SOURCE_AUDIO | OBS_SOURCE_DO_NOT_DUPLICATE | OBS_SOURCE_DO_NOT_SELF_MONITOR;
	info.get_name = GetWASAPIDeviceOutputName;
	info.create = CreateWASAPIDeviceOutput;
	info.destroy = DestroyWASAPISource;
	info.update = UpdateWASAPISource;
	info.activate = ActivateWASAPISource;
	info.deactivate = DeactivateWASAPISource;
	info.get_defaults = GetWASAPIDefaultsDeviceOutput;
	info.get_properties = GetWASAPIPropertiesDeviceOutput;
	info.icon_type = OBS_ICON_TYPE_AUDIO_OUTPUT;
	obs_register_source(&info);
}

void RegisterWASAPIProcessOutput()
{
	obs_source_info info = {};
	info.id = "wasapi_process_output_capture";
	info.type = OBS_SOURCE_TYPE_INPUT;
	info.output_flags = OBS_SOURCE_AUDIO | OBS_SOURCE_DO_NOT_DUPLICATE | OBS_SOURCE_DO_NOT_SELF_MONITOR;
	info.get_name = GetWASAPIProcessOutputName;
	info.create = CreateWASAPIProcessOutput;
	info.destroy = DestroyWASAPISource;
	info.update = UpdateWASAPISource;
	info.activate = ActivateWASAPISource;
	info.deactivate = DeactivateWASAPISource;
	info.get_defaults = GetWASAPIDefaultsProcessOutput;
	info.get_properties = GetWASAPIPropertiesProcessOutput;
	info.icon_type = OBS_ICON_TYPE_PROCESS_AUDIO_OUTPUT;
	obs_register_source(&info);
}
