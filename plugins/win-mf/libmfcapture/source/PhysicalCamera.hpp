#pragma once

#define ARGB_SEG_MASK_WORKAROUND

struct CameraInformation {
	std::wstring FriendlyName;
	std::wstring SymbolicLink;
};

struct StreamInformation {
	UINT32 uiWidth;
	UINT32 uiHeight;
	UINT32 uiFpsN;
	UINT32 uiFpsD;
	GUID guidSubtype;
};

struct MepSetting {
	std::wstring SymbolicLink;
	bool Blur = false;
	bool ShallowFocus = false;
	bool Mask = false;
	bool AutoFraming = false;
	bool EyeGazeCorrection = false;
};

class PhysicalCamera : public IMFSourceReaderCallback {
	static std::vector<MepSetting> s_MepSettings;

private:
	long m_cRef = 1;
	winrt::slim_mutex m_Lock;
	wil::com_ptr_nothrow<IMFMediaSource> m_spDevSource = nullptr;
	wil::com_ptr_nothrow<IMFSourceReader> m_spSourceReader = nullptr;
	wil::com_ptr_nothrow<IMFDXGIDeviceManager> m_spDxgiDevManager = nullptr;
	wil::com_ptr_nothrow<IMFExtendedCameraController> m_spExtController =
		nullptr;

	MF_VideoDataCallback m_cbVideoData = nullptr;
	void *m_pUserData = nullptr;

	MF_COLOR_FORMAT m_fmt = MF_COLOR_FORMAT_UNKNOWN;
	UINT32 m_uiWidth = 0;
	UINT32 m_uiHeight = 0;
	LONG m_lDefaultStride = 0;

	UINT32 m_uiSegMaskBufSize = 0;
	BYTE *m_pSegMaskBuf = nullptr;

	bool m_transparent = false;

#ifdef ARGB_SEG_MASK_WORKAROUND
	BYTE *m_pBufferArgbOut = nullptr;
#endif

	std::wstring m_wsSymbolicName;

	DeviceControlChangeListener *m_pDevCtrlNotify = nullptr;

private:
	HRESULT CreateSourceReader(IMFMediaSource *pSource,
				   IMFDXGIDeviceManager *pDxgiDevIManager,
				   IMFSourceReader **ppSourceReader);

	HRESULT FindMatchingNativeMediaType(DWORD dwPhyStrmIndex,
					    UINT32 uiWidth, UINT32 uiHeight,
					    UINT32 uiFpsN, UINT32 uiFpsD,
					    IMFMediaType **ppMatchingType);

	HRESULT FindMatchingNativeMediaType(DWORD dwPhyStrmIndex,
					    UINT32 uiWidth, UINT32 uiHeight,
					    LONGLONG llInterval,
					    IMFMediaType **ppMatchingType);

	HRESULT GetMepSetting(MepSetting &setting);
	HRESULT SetMepSetting(MepSetting &setting);

	HRESULT SaveSettingsToDefault();

	HRESULT FillSegMask(IMFSample *pSample);
	HRESULT FillAlphaWithSegMask(DWORD *pData);

public:
	PhysicalCamera();
	~PhysicalCamera();

	PhysicalCamera(const PhysicalCamera &) = delete;
	PhysicalCamera &operator=(PhysicalCamera const &) = delete;

	// IUnknown methods
	STDMETHODIMP QueryInterface(REFIID iid, void **ppv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// IMFSourceReaderCallback methods
	STDMETHODIMP OnReadSample(HRESULT hrStatus, DWORD dwPhyStrmIndex,
				  DWORD dwStreamFlags, LONGLONG llTimestamp,
				  IMFSample *pSample);

	STDMETHODIMP OnEvent(DWORD, IMFMediaEvent *) { return S_OK; }

	STDMETHODIMP OnFlush(DWORD) { return S_OK; }

	HRESULT Initialize(LPCWSTR pwszSymLink);

	HRESULT Uninitialize();

	HRESULT SetD3dManager(IMFDXGIDeviceManager *pD3dManager);

	HRESULT Prepare();

	HRESULT Start(DWORD dwPhyStrmIndex, MF_VideoDataCallback cb,
		      void *pUserData);

	HRESULT Stop();

	HRESULT GetStreamCapabilities(DWORD dwPhyStrmIndex,
				      std::vector<StreamInformation> &strmCaps);

	void SetTransparent(bool flag) { m_transparent = flag; }

	bool GetTransparent() { return m_transparent; }

	// if llInterval is 0, set the camera to us the max fps
	// otherwise, set camera with matching fps.
	HRESULT SetOutputResolution(DWORD dwPhyStrmIndex, UINT32 uiWidth,
				    UINT32 uiHeight, LONGLONG llInterval,
				    MF_COLOR_FORMAT fmt);

	HRESULT GetCurrentStreamInformation(DWORD dwPhyStrmIndex,
					    StreamInformation &strmInfo);

	HRESULT SetBlur(bool blur, bool shallowFocus, bool mask);
	HRESULT GetBlur(bool &blur, bool &shallowFocus, bool &mask);

	HRESULT SetAutoFraming(bool enable);
	HRESULT GetAutoFraming(bool &enable);

	HRESULT SetEyeGazeCorrection(bool enable);
	HRESULT GetEyeGazeCorrection(bool &enable);

	HRESULT RestoreDefaultSettings();

	static HRESULT CreateInstance(PhysicalCamera **ppPhysicalCamera);

	static HRESULT
	GetPhysicalCameras(std::vector<CameraInformation> &cameras);
};
