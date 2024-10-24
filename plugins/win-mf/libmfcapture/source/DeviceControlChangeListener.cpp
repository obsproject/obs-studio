#include "DeviceControlChangeListener.hpp"

DeviceControlChangeListener::DeviceControlChangeListener() {}

DeviceControlChangeListener::~DeviceControlChangeListener() {}

// IUnknown methods
HRESULT __stdcall DeviceControlChangeListener::QueryInterface(REFIID iid,
							      void **ppv)
{

	if (iid == IID_IUnknown) {
		*ppv = static_cast<IUnknown *>(this);
	} else if (iid == IID_IMFCameraControlNotify) {
		*ppv = static_cast<IMFCameraControlNotify *>(this);
	} else {
		*ppv = NULL;
		return E_NOINTERFACE;
	}
	reinterpret_cast<IUnknown *>(*ppv)->AddRef();
	return S_OK;
}

ULONG __stdcall DeviceControlChangeListener::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

ULONG __stdcall DeviceControlChangeListener::Release()
{

	if (InterlockedDecrement(&m_cRef) == 0) {
		delete this;
		return 0;
	}
	return m_cRef;
}

HRESULT
DeviceControlChangeListener::HandleControlSet_ExtendedCameraControl(UINT32 id)
{

	if (id != KSPROPERTY_CAMERACONTROL_EXTENDED_EYEGAZECORRECTION &&
	    id != KSPROPERTY_CAMERACONTROL_EXTENDED_BACKGROUNDSEGMENTATION &&
	    id != KSPROPERTY_CAMERACONTROL_EXTENDED_DIGITALWINDOW &&
	    id != KSPROPERTY_CAMERACONTROL_EXTENDED_DIGITALWINDOW_CONFIGCAPS &&
	    id != KSPROPERTY_CAMERACONTROL_EXTENDED_VIDEOHDR) {
		return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
	}

	DWORD dwStreamIndex = KSCAMERA_EXTENDEDPROP_FILTERSCOPE;
	wil::com_ptr<IMFExtendedCameraControl> spExtControl = nullptr;
	RETURN_IF_FAILED(m_spExtCamController->GetExtendedCameraControl(
		dwStreamIndex, id, &spExtControl));

	ULONGLONG Flags = spExtControl->GetFlags();

	WS_CONTROL_TYPE ctrlType = WS_EFFECT_UNKNOWN;
	WS_CONTROL_VALUE ctrlValue = WS_EFFECT_OFF;

	if (id == KSPROPERTY_CAMERACONTROL_EXTENDED_EYEGAZECORRECTION) {

		bool effect = Flags &
			      KSCAMERA_EXTENDEDPROP_EYEGAZECORRECTION_ON;

	} else if (id ==
		   KSPROPERTY_CAMERACONTROL_EXTENDED_BACKGROUNDSEGMENTATION) {

		bool blur = Flags &
			    KSCAMERA_EXTENDEDPROP_BACKGROUNDSEGMENTATION_BLUR;
		bool focus =
			Flags &
			KSCAMERA_EXTENDEDPROP_BACKGROUNDSEGMENTATION_SHALLOWFOCUS;
		bool mask = Flags &
			    KSCAMERA_EXTENDEDPROP_BACKGROUNDSEGMENTATION_MASK;

		if (focus) {
			blur = false;
		}

	} else if (id == KSPROPERTY_CAMERACONTROL_EXTENDED_DIGITALWINDOW) {

		bool effect =
			Flags &
			KSCAMERA_EXTENDEDPROP_DIGITALWINDOW_AUTOFACEFRAMING;
	}

	return S_OK;
}

void PrintGuid(GUID guid)
{
	printf("{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}",
	       guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1],
	       guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5],
	       guid.Data4[6], guid.Data4[7]);
}

void STDMETHODCALLTYPE
DeviceControlChangeListener::OnChange(_In_ REFGUID controlSet, _In_ UINT32 id)
{
	auto lock = m_lock.lock();

	printf("Guid = ");
	PrintGuid(controlSet);
	printf(" Id = %d\n", id);

	if (IsEqualCLSID(controlSet, KSPROPERTYSETID_ANYCAMERACONTROL)) {
		printf("KSPROPERTYSETID_ANYCAMERACONTROL\n");
	} else if (IsEqualCLSID(controlSet, PROPSETID_VIDCAP_VIDEOPROCAMP)) {
		printf("PROPSETID_VIDCAP_VIDEOPROCAMP\n");
	} else if (IsEqualCLSID(controlSet, PROPSETID_VIDCAP_CAMERACONTROL)) {
		printf("PROPSETID_VIDCAP_CAMERACONTROL\n");

	} else if (IsEqualCLSID(controlSet,
				KSPROPERTYSETID_ExtendedCameraControl)) {
		printf("KSPROPERTYSETID_ExtendedCameraControl\n");
		HandleControlSet_ExtendedCameraControl(id);
	} else {
		printf("Unknown control set\n");
	}
}

void STDMETHODCALLTYPE
DeviceControlChangeListener::OnError(_In_ HRESULT hrStatus)
{
	auto lock = m_lock.lock();
	printf("%s, hrStatus = 0x%x\n", __FUNCSIG__, hrStatus);
}

HRESULT DeviceControlChangeListener::Start(
	const wchar_t *devId, IMFExtendedCameraController *pExtCamController)
{
	{
		auto lock = m_lock.lock();
		if (m_bStarted) {
			return S_OK;
		}
	}

	wil::com_ptr_nothrow<IMFCameraControlMonitor> spMonitor;
	RETURN_IF_FAILED(MFCreateCameraControlMonitor(devId, this, &spMonitor));
	RETURN_IF_FAILED(spMonitor->AddControlSubscription(
		KSPROPERTYSETID_ANYCAMERACONTROL, 0));
	RETURN_IF_FAILED(spMonitor->Start());

	{
		auto lock = m_lock.lock();
		m_spMonitor = spMonitor;
	}

	m_spExtCamController = pExtCamController;

	m_bStarted = true;
	return S_OK;
}

HRESULT DeviceControlChangeListener::Stop()
{

	auto lock = m_lock.lock();
	if (!m_bStarted) {
		return S_OK;
	}

	if (m_spMonitor != nullptr) {
		m_spMonitor->RemoveControlSubscription(
			KSPROPERTYSETID_ANYCAMERACONTROL, 0);
		(void)m_spMonitor->Shutdown();
		m_spMonitor = nullptr;
	}

	m_spExtCamController = nullptr;

	m_bStarted = false;
	return S_OK;
}
