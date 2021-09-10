/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2windriverinterface.cpp
	@brief		Implementation of CNTV2WinDriverInterface.
	@copyright	(C) 2003-2021 AJA Video Systems, Inc.
**/

#include "ntv2windriverinterface.h"
#include "ntv2winpublicinterface.h"
#include "ntv2publicinterface.h"
#include "ntv2nubtypes.h"
#include "ntv2debug.h"
#include "winioctl.h"
#include "ajabase/system/debug.h"
#include <sstream>

using namespace std;

#ifdef _AJA_COMPILE_WIN2K_SOFT_LINK
typedef WINSETUPAPI
HDEVINFO
(WINAPI *
pfcnSetupDiGetClassDevsA)(
    IN CONST GUID *ClassGuid,  OPTIONAL
    IN PCSTR       Enumerator, OPTIONAL
    IN HWND        hwndParent, OPTIONAL
    IN DWORD       Flags
    );

typedef WINSETUPAPI
HDEVINFO
(WINAPI *
pfcnSetupDiGetClassDevsW)(
    IN CONST GUID *ClassGuid,  OPTIONAL
    IN PCWSTR      Enumerator, OPTIONAL
    IN HWND        hwndParent, OPTIONAL
    IN DWORD       Flags
    );
#ifdef UNICODE
#define pfcnSetupDiGetClassDevs pfcnSetupDiGetClassDevsW
#else
#define pfcnSetupDiGetClassDevs pfcnSetupDiGetClassDevsA
#endif

typedef WINSETUPAPI
BOOL
(WINAPI *
pfcnSetupDiEnumDeviceInterfaces)(
    IN  HDEVINFO                   DeviceInfoSet,
    IN  PSP_DEVINFO_DATA           DeviceInfoData,     OPTIONAL
    IN  CONST GUID                *InterfaceClassGuid,
    IN  DWORD                      MemberIndex,
    OUT PSP_DEVICE_INTERFACE_DATA  DeviceInterfaceData
    );


typedef WINSETUPAPI
BOOL
(WINAPI *
pfcnSetupDiGetDeviceInterfaceDetailA)(
    IN  HDEVINFO                           DeviceInfoSet,
    IN  PSP_DEVICE_INTERFACE_DATA          DeviceInterfaceData,
    OUT PSP_DEVICE_INTERFACE_DETAIL_DATA_A DeviceInterfaceDetailData,     OPTIONAL
    IN  DWORD                              DeviceInterfaceDetailDataSize,
    OUT PDWORD                             RequiredSize,                  OPTIONAL
    OUT PSP_DEVINFO_DATA                   DeviceInfoData                 OPTIONAL
    );

typedef WINSETUPAPI
BOOL
(WINAPI *
pfcnSetupDiGetDeviceInterfaceDetailW)(
    IN  HDEVINFO                           DeviceInfoSet,
    IN  PSP_DEVICE_INTERFACE_DATA          DeviceInterfaceData,
    OUT PSP_DEVICE_INTERFACE_DETAIL_DATA_W DeviceInterfaceDetailData,     OPTIONAL
    IN  DWORD                              DeviceInterfaceDetailDataSize,
    OUT PDWORD                             RequiredSize,                  OPTIONAL
    OUT PSP_DEVINFO_DATA                   DeviceInfoData                 OPTIONAL
	);
#ifdef UNICODE
#define pfcnSetupDiGetDeviceInterfaceDetail pfcnSetupDiGetDeviceInterfaceDetailW
#else
#define pfcnSetupDiGetDeviceInterfaceDetail pfcnSetupDiGetDeviceInterfaceDetailA
#endif

typedef WINSETUPAPI
BOOL
(WINAPI *
 pfcnSetupDiDestroyDeviceInfoList)(
 IN HDEVINFO DeviceInfoSet
 );

typedef WINSETUPAPI
BOOL
(WINAPI *
 pfcnSetupDiDestroyDriverInfoList)(
 IN HDEVINFO         DeviceInfoSet,
 IN PSP_DEVINFO_DATA DeviceInfoData, OPTIONAL
 IN DWORD            DriverType
 );

static pfcnSetupDiGetClassDevs pSetupDiGetClassDevs = NULL;
static pfcnSetupDiEnumDeviceInterfaces pSetupDiEnumDeviceInterfaces = NULL;
static pfcnSetupDiGetDeviceInterfaceDetail pSetupDiGetDeviceInterfaceDetail = NULL;
static pfcnSetupDiDestroyDeviceInfoList pSetupDiDestroyDeviceInfoList = NULL;
static pfcnSetupDiDestroyDriverInfoList pSetupDiDestroyDriverInfoList = NULL;

HINSTANCE hSetupAPI = NULL;

#endif

static std::string GetKernErrStr (const DWORD inError)
{
	LPVOID lpMsgBuf(NULL);
	FormatMessage (	FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					inError,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
					(LPTSTR) &lpMsgBuf,
					0,
					NULL);
	string result (lpMsgBuf ? reinterpret_cast<const char*>(lpMsgBuf) : "");
	LocalFree (lpMsgBuf);
//	Truncate at <CR><LF>...
	const size_t	crPos(result.find(".\r"));
	if (crPos != string::npos)
		result.resize(crPos);
	return result;
}


//	WinDriverInterface Logging Macros
#define	HEX2(__x__)			"0x" << hex << setw(2)  << setfill('0') << (0xFF       & uint8_t (__x__)) << dec
#define	HEX4(__x__)			"0x" << hex << setw(4)  << setfill('0') << (0xFFFF     & uint16_t(__x__)) << dec
#define	HEX8(__x__)			"0x" << hex << setw(8)  << setfill('0') << (0xFFFFFFFF & uint32_t(__x__)) << dec
#define	HEX16(__x__)		"0x" << hex << setw(16) << setfill('0') <<               uint64_t(__x__)  << dec
#define KR(_kr_)			"kernResult=" << HEX8(_kr_) << "(" << GetKernErrStr(_kr_) << ")"
#define INSTP(_p_)			HEX16(uint64_t(_p_))

#define	WDIFAIL(__x__)		AJA_sERROR  (AJA_DebugUnit_DriverInterface,  INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	WDIWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_DriverInterface,  INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	WDINOTE(__x__)		AJA_sNOTICE (AJA_DebugUnit_DriverInterface,  INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	WDIINFO(__x__)		AJA_sINFO   (AJA_DebugUnit_DriverInterface,  INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	WDIDBG(__x__)		AJA_sDEBUG  (AJA_DebugUnit_DriverInterface,  INSTP(this) << "::" << AJAFUNC << ": " << __x__)


CNTV2WinDriverInterface::CNTV2WinDriverInterface()
	:	_pspDevIFaceDetailData		(AJA_NULL)
		,_hDevInfoSet				(INVALID_HANDLE_VALUE)
		,_hDevice					(INVALID_HANDLE_VALUE)
		,_previousAudioState		(0)
		,_previousAudioSelection	(0)
#if !defined(NTV2_DEPRECATE_16_0)
		,_vecDmaLocked				()
#endif	//	!defined(NTV2_DEPRECATE_16_0)
{
	::memset(&_spDevInfoData, 0, sizeof(_spDevInfoData));
	::memset(&_GUID_PROPSET, 0, sizeof(_GUID_PROPSET));
}

CNTV2WinDriverInterface::~CNTV2WinDriverInterface()
{
	if (IsOpen())
		Close();
}


/////////////////////////////////////////////////////////////////////////////////////
// Board Open / Close methods
/////////////////////////////////////////////////////////////////////////////////////

bool CNTV2WinDriverInterface::OpenLocalPhysical (const UWord inDeviceIndex)
{
	NTV2_ASSERT(!IsRemote());
	NTV2_ASSERT(!IsOpen());
	NTV2_ASSERT(_hDevice == INVALID_HANDLE_VALUE);
	string boardStr;

	{
		DEFINE_GUIDSTRUCT("844B39E5-C98E-45a1-84DE-3BAF3F4F9F14", AJAVIDEO_NTV2_PROPSET);
#define AJAVIDEO_NTV2_PROPSET DEFINE_GUIDNAMED(AJAVIDEO_NTV2_PROPSET)
		_GUID_PROPSET = AJAVIDEO_NTV2_PROPSET;
	}

	REFGUID refguid = _GUID_PROPSET;
	_boardNumber = inDeviceIndex;

	DWORD dwShareMode (GetShareMode() ? FILE_SHARE_READ | FILE_SHARE_WRITE : 0);
	DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED;
	DWORD dwReqSize=0;
	SP_DEVICE_INTERFACE_DATA spDevIFaceData;
	memset(&spDevIFaceData, 0, sizeof(SP_DEVICE_INTERFACE_DATA));
	GUID myguid = refguid;  // an un-const guid for compiling with new Platform SDK!
	_hDevInfoSet = SetupDiGetClassDevs(&myguid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (_hDevInfoSet == INVALID_HANDLE_VALUE)
		return false;
	spDevIFaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	myguid = refguid;
	if (!SetupDiEnumDeviceInterfaces(_hDevInfoSet, NULL, &myguid, _boardNumber, &spDevIFaceData))
	{
		SetupDiDestroyDeviceInfoList(_hDevInfoSet);
		return false;
	}

	if (SetupDiGetDeviceInterfaceDetail(_hDevInfoSet, &spDevIFaceData, NULL, 0, &dwReqSize, NULL))
	{
		SetupDiDestroyDeviceInfoList(_hDevInfoSet);
		return false; //should have failed!
	}
	if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
	{
		SetupDiDestroyDeviceInfoList(_hDevInfoSet);
		return false;
	}
	_pspDevIFaceDetailData = PSP_DEVICE_INTERFACE_DETAIL_DATA(new BYTE[dwReqSize]);
	if (!_pspDevIFaceDetailData)
	{
		SetupDiDestroyDeviceInfoList(_hDevInfoSet);
		return false; // out of memory
	}

	memset(&_spDevInfoData, 0, sizeof(SP_DEVINFO_DATA));
	_spDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	_pspDevIFaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
	//now we are setup to get the info we want!
	if (!SetupDiGetDeviceInterfaceDetail(_hDevInfoSet, &spDevIFaceData, _pspDevIFaceDetailData, dwReqSize, NULL, &_spDevInfoData))
	{
		delete [] _pspDevIFaceDetailData;
		_pspDevIFaceDetailData = NULL;
		SetupDiDestroyDeviceInfoList(_hDevInfoSet);
		return false; // out of memory
	}

	ULONG deviceInstanceSize = 0;
	CM_Get_Device_ID_Size(&deviceInstanceSize, _spDevInfoData.DevInst, 0);
	char* deviceInstance = (char*)new BYTE[deviceInstanceSize*2];
	CM_Get_Device_IDA(_spDevInfoData.DevInst, deviceInstance, deviceInstanceSize*2, 0);
	boardStr = deviceInstance;
	delete [] deviceInstance;

	_hDevice = CreateFile(_pspDevIFaceDetailData->DevicePath, GENERIC_READ | GENERIC_WRITE, dwShareMode, NULL, OPEN_EXISTING, dwFlagsAndAttributes, NULL);
	if (_hDevice == INVALID_HANDLE_VALUE)
	{
		WDIFAIL("CreateFile failed for '" << boardStr << "'");
		delete [] _pspDevIFaceDetailData;
		_pspDevIFaceDetailData = NULL;
		SetupDiDestroyDeviceInfoList(_hDevInfoSet);
		_hDevInfoSet = NULL;
		return false;
	}

	_boardOpened = true;
	CNTV2DriverInterface::ReadRegister(kRegBoardID, _boardID);

	WDIINFO ("Opened '" << boardStr << "' deviceID=" << HEX8(_boardID) << " deviceIndex=" << DEC(_boardNumber));
	return true;
}	//	OpenLocalPhysical



bool CNTV2WinDriverInterface::CloseLocalPhysical (void)
{
	NTV2_ASSERT(!IsRemote());
	NTV2_ASSERT(IsOpen());
	NTV2_ASSERT(_hDevice != INVALID_HANDLE_VALUE);
	if (_pspDevIFaceDetailData)
	{
		delete [] _pspDevIFaceDetailData;
		_pspDevIFaceDetailData = AJA_NULL;
	}
	if (_hDevInfoSet)
	{
#ifndef _AJA_COMPILE_WIN2K_SOFT_LINK
		SetupDiDestroyDeviceInfoList(_hDevInfoSet);
#else
		if (pSetupDiDestroyDeviceInfoList)
			pSetupDiDestroyDeviceInfoList(_hDevInfoSet);
#endif
		_hDevInfoSet = AJA_NULL;
	}

	if (_hDevice != INVALID_HANDLE_VALUE)
		CloseHandle(_hDevice);
	WDIINFO ("Closed deviceID=" << HEX8(_boardID) << " deviceIndex=" << DEC(_boardNumber));

	_hDevice = INVALID_HANDLE_VALUE;
	_boardOpened = false;
	return true;
}



///////////////////////////////////////////////////////////////////////////////////
// Read and Write Register methods
///////////////////////////////////////////////////////////////////////////////////

bool CNTV2WinDriverInterface::ReadRegister (const ULWord inRegNum,  ULWord & outValue,  const ULWord inMask,  const ULWord inShift)
{
	if (!IsOpen())
		return false;
	if (inShift >= 32)
	{
		WDIFAIL("Shift " << DEC(inShift) << " > 31, reg=" << DEC(inRegNum) << " msk=" << xHEX0N(inMask,8));
		return false;
	}
#if defined(NTV2_NUB_CLIENT_SUPPORT)
	if (IsRemote())
		return CNTV2DriverInterface::ReadRegister (inRegNum, outValue, inMask, inShift);
#endif	//	defined(NTV2_NUB_CLIENT_SUPPORT)
	NTV2_ASSERT( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0));

	KSPROPERTY_AJAPROPS_GETSETREGISTER_S propStruct;
	DWORD dwBytesReturned = 0;
	ZeroMemory(&propStruct, sizeof(KSPROPERTY_AJAPROPS_GETSETREGISTER_S));
	propStruct.Property.Set		= _GUID_PROPSET;
	propStruct.Property.Id		= KSPROPERTY_AJAPROPS_GETSETREGISTER;
	propStruct.Property.Flags	= KSPROPERTY_TYPE_GET;
	propStruct.RegisterID		= inRegNum;
	propStruct.ulRegisterMask	= inMask;
	propStruct.ulRegisterShift	= inShift;
	if (DeviceIoControl(_hDevice, IOCTL_AJAPROPS_GETSETREGISTER, &propStruct, sizeof(KSPROPERTY_AJAPROPS_GETSETREGISTER_S),
						&propStruct, sizeof(KSPROPERTY_AJAPROPS_GETSETREGISTER_S), &dwBytesReturned, NULL))
	{
		outValue = propStruct.ulRegisterValue;
		return true;
	}
	WDIFAIL("reg=" << DEC(inRegNum) << " val=" << xHEX0N(outValue,8) << " msk=" << xHEX0N(inMask,8) << " shf=" << DEC(inShift) << " failed: " << ::GetKernErrStr(GetLastError()));
	return false;
}


bool CNTV2WinDriverInterface::WriteRegister (const ULWord inRegNum,  const ULWord inValue,  const ULWord inMask, const ULWord inShift)
{
	if (!IsOpen())
		return false;
	if (inShift >= 32)
	{
		WDIFAIL("Shift " << DEC(inShift) << " > 31, reg=" << DEC(inRegNum) << " msk=" << xHEX0N(inMask,8));
		return false;
	}
#if defined(NTV2_WRITEREG_PROFILING)	//	Register Write Profiling
	if (mRecordRegWrites)
	{
		AJAAutoLock	autoLock(&mRegWritesLock);
		mRegWrites.push_back(NTV2RegInfo(inRegNum, inValue, inMask, inShift));
		if (mSkipRegWrites)
			return true;
	}
#endif	//	defined(NTV2_WRITEREG_PROFILING)	//	Register Write Profiling
#if defined(NTV2_NUB_CLIENT_SUPPORT)
	if (IsRemote())
		return CNTV2DriverInterface::WriteRegister(inRegNum, inValue, inMask, inShift);
#endif	//	defined(NTV2_NUB_CLIENT_SUPPORT)
	KSPROPERTY_AJAPROPS_GETSETREGISTER_S propStruct;
	DWORD dwBytesReturned = 0;
	NTV2_ASSERT(inShift < 32);

	ZeroMemory(&propStruct,sizeof(KSPROPERTY_AJAPROPS_GETSETREGISTER_S));
	propStruct.Property.Set		= _GUID_PROPSET;
	propStruct.Property.Id		= KSPROPERTY_AJAPROPS_GETSETREGISTER;
	propStruct.Property.Flags	= KSPROPERTY_TYPE_SET;
	propStruct.RegisterID		= inRegNum;
	propStruct.ulRegisterValue	= inValue;
	propStruct.ulRegisterMask	= inMask;
	propStruct.ulRegisterShift	= inShift;
	if (!DeviceIoControl(_hDevice, IOCTL_AJAPROPS_GETSETREGISTER, &propStruct, sizeof(KSPROPERTY_AJAPROPS_GETSETREGISTER_S),
							&propStruct, sizeof(KSPROPERTY_AJAPROPS_GETSETREGISTER_S), &dwBytesReturned, NULL))
	{
		WDIFAIL("reg=" << DEC(inRegNum) << " val=" << xHEX0N(inValue,8) << " msk=" << xHEX0N(inMask,8) << " shf=" << DEC(inShift) << " failed: " << ::GetKernErrStr(GetLastError()));
		return false;
	}
	return true;
}

/////////////////////////////////////////////////////////////////////////////
// Interrupt enabling / disabling method
/////////////////////////////////////////////////////////////////////////////

// Method:	ConfigureInterrupt
// Input:	bool bEnable (turn on/off interrupt), INTERRUPT_ENUMS eInterruptType
// Output:	bool status
// Purpose:	Provides a 1 point connection to driver for interrupt calls
bool CNTV2WinDriverInterface::ConfigureInterrupt (const bool bEnable, const INTERRUPT_ENUMS eInterruptType)
{
	if (!IsOpen())
		return false;
	if (IsRemote())
		return CNTV2DriverInterface::ConfigureInterrupt(bEnable, eInterruptType);
	KSPROPERTY_AJAPROPS_INTERRUPTS_S propStruct;	// boilerplate AVStream Property structure
	DWORD dwBytesReturned = 0;
	ZeroMemory (&propStruct,sizeof(KSPROPERTY_AJAPROPS_INTERRUPTS_S));
	propStruct.Property.Set		= _GUID_PROPSET;
	propStruct.Property.Id		= KSPROPERTY_AJAPROPS_INTERRUPTS;
	propStruct.Property.Flags	= bEnable ? KSPROPERTY_TYPE_GET : KSPROPERTY_TYPE_SET;
	propStruct.eInterrupt		= eInterruptType;

	if (!DeviceIoControl (_hDevice,  IOCTL_AJAPROPS_INTERRUPTS,  &propStruct,  sizeof(KSPROPERTY_AJAPROPS_INTERRUPTS_S),
							&propStruct,  sizeof(KSPROPERTY_AJAPROPS_INTERRUPTS_S),  &dwBytesReturned,  NULL))
	{
		WDIFAIL("interruptType=" << DEC(eInterruptType) << " enable=" << (bEnable?"Y":"N") << " failed: " << ::GetKernErrStr(GetLastError()));
		return false;
	}
	return true;
}

// Method: ConfigureSubscriptions
// Input:  bool bSubscribe (true if subscribing), INTERRUPT_ENUMS eInterruptType,
//		HANDLE & hSubcription
// Output: HANDLE & hSubcription (if subscribing)
// Notes:  collects all driver calls for subscriptions in one place
bool CNTV2WinDriverInterface::ConfigureSubscription (const bool bSubscribe, const INTERRUPT_ENUMS eInterruptType, PULWord & outSubscriptionHdl)
{
	if (!IsOpen())
		return false;
	bool res(CNTV2DriverInterface::ConfigureSubscription (bSubscribe, eInterruptType, outSubscriptionHdl));
	if (IsRemote())
		return res;

	// Check for previouse call to subscribe
	if (bSubscribe  &&  outSubscriptionHdl)
		return true;	//	Already subscribed

	// Check for valid handle to unsubscribe
	if (!bSubscribe  &&  !outSubscriptionHdl)
		return true;	//	Already unsubscribed

	// Assure that the avCard has been properly opened
	HANDLE hSubscription = bSubscribe ? CreateEvent (NULL, FALSE, FALSE, NULL) : HANDLE(outSubscriptionHdl);
	KSPROPERTY_AJAPROPS_NEWSUBSCRIPTIONS_S propStruct;
	DWORD dwBytesReturned = 0;

	ZeroMemory (&propStruct,sizeof(KSPROPERTY_AJAPROPS_NEWSUBSCRIPTIONS_S));
	propStruct.Property.Set		= _GUID_PROPSET;
	propStruct.Property.Id		= KSPROPERTY_AJAPROPS_NEWSUBSCRIPTIONS;
	propStruct.Property.Flags	= bSubscribe ? KSPROPERTY_TYPE_GET : KSPROPERTY_TYPE_SET;
	propStruct.Handle			= hSubscription;
	propStruct.eInterrupt		= eInterruptType;

	BOOL bRet = DeviceIoControl (_hDevice,  IOCTL_AJAPROPS_NEWSUBSCRIPTIONS,  &propStruct,  sizeof(KSPROPERTY_AJAPROPS_NEWSUBSCRIPTIONS_S),
								&propStruct,  sizeof(KSPROPERTY_AJAPROPS_NEWSUBSCRIPTIONS_S),  &dwBytesReturned,  NULL);
	if ((!bSubscribe && bRet)  ||  (bSubscribe && !bRet))
	{
		CloseHandle(hSubscription);
		outSubscriptionHdl = 0;
	}

	if (!bRet)
	{
		WDIFAIL("interruptType=" << DEC(eInterruptType) << " subscribe=" << (bSubscribe?"Y":"N") << " failed: " << ::GetKernErrStr(GetLastError()));
	    return false;
    }
	if (bSubscribe)
		outSubscriptionHdl = PULWord(hSubscription);
	return true;
}

// Method: getInterruptCount
// Input:  NONE
// Output: ULONG or equivalent(i.e. ULWord).
bool CNTV2WinDriverInterface::GetInterruptCount (const INTERRUPT_ENUMS eInterruptType, ULWord & outCount)
{
	if (!IsOpen())
		return false;
#if defined(NTV2_NUB_CLIENT_SUPPORT)
	if (IsRemote())
		return CNTV2DriverInterface::GetInterruptCount(eInterruptType, outCount);
#endif	//	defined(NTV2_NUB_CLIENT_SUPPORT)
	KSPROPERTY_AJAPROPS_NEWSUBSCRIPTIONS_S propStruct;
	DWORD dwBytesReturned = 0;
	ZeroMemory (&propStruct,sizeof(KSPROPERTY_AJAPROPS_NEWSUBSCRIPTIONS_S));
	propStruct.Property.Set		= _GUID_PROPSET;
	propStruct.Property.Id		= KSPROPERTY_AJAPROPS_NEWSUBSCRIPTIONS;
	propStruct.Property.Flags	= KSPROPERTY_TYPE_GET;
	propStruct.eInterrupt		= eGetIntCount;
	propStruct.ulIntCount		= ULONG(eInterruptType);
	if (!DeviceIoControl (_hDevice,  IOCTL_AJAPROPS_NEWSUBSCRIPTIONS,  &propStruct,  sizeof(KSPROPERTY_AJAPROPS_NEWSUBSCRIPTIONS_S),
							&propStruct,  sizeof(KSPROPERTY_AJAPROPS_NEWSUBSCRIPTIONS_S),  &dwBytesReturned,  NULL))
	{
		WDIFAIL("interruptType=" << DEC(eInterruptType) << " failed: " << ::GetKernErrStr(GetLastError()));
		return false;
	}
	outCount = propStruct.ulIntCount;
	return true;
}

bool CNTV2WinDriverInterface::WaitForInterrupt (const INTERRUPT_ENUMS eInterruptType, const ULWord timeOutMs)
{
	if (!IsOpen())
		return false;
#if defined(NTV2_NUB_CLIENT_SUPPORT)
	if (IsRemote())
		return CNTV2DriverInterface::WaitForInterrupt(eInterruptType,timeOutMs);
#endif	//	defined(NTV2_NUB_CLIENT_SUPPORT)
    bool bInterruptHappened = false;    // return value

	HANDLE hEvent (GetInterruptEvent(eInterruptType));
	if (NULL == hEvent)
	{
		// no interrupt hooked up so just use Sleep function
		Sleep (timeOutMs);
	}
	else
    {
        // interrupt hooked up. Wait
        DWORD status = WaitForSingleObject(hEvent, timeOutMs);
        if ( status == WAIT_OBJECT_0 )
        {
            bInterruptHappened = true;
            BumpEventCount (eInterruptType);
        }
		else
		{
			;//MessageBox (0, "WaitForInterrupt timed out", "CNTV2WinDriverInterface", MB_ICONERROR | MB_OK);
		}
    }
    return bInterruptHappened;
}

//////////////////////////////////////////////////////////////////////////////
// OEM Mapping to Userspace Methods
//////////////////////////////////////////////////////////////////////////////

#if !defined(NTV2_DEPRECATE_16_0)
	// Method:	MapFrameBuffers
	bool CNTV2WinDriverInterface::MapFrameBuffers (void)
	{
		if (IsRemote())
			return false;
		if (!IsOpen())
			return false;
		KSPROPERTY_AJAPROPS_MAPMEMORY_S propStruct;
		DWORD dwBytesReturned = 0;
		_pFrameBaseAddress = 0;
		_pCh1FrameBaseAddress = 0;
		ZeroMemory (&propStruct,sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S));
		propStruct.Property.Set		= _GUID_PROPSET;
		propStruct.Property.Id		= KSPROPERTY_AJAPROPS_MAPMEMORY;
		propStruct.Property.Flags	= KSPROPERTY_TYPE_GET;
		propStruct.bMapType			= NTV2_MAPMEMORY_FRAMEBUFFER;

		if (!DeviceIoControl(_hDevice,  IOCTL_AJAPROPS_MAPMEMORY,  &propStruct,  sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S),
								&propStruct,  sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S),  &dwBytesReturned,  NULL))
		{
			WDIFAIL("failed: " << ::GetKernErrStr(GetLastError()));
			return false;
		}
		ULWord boardIDRegister;
		ReadRegister(kRegBoardID, boardIDRegister);	//unfortunately GetBoardID is in ntv2card...ooops.
		_pFrameBaseAddress = (ULWord *) propStruct.mapMemory.Address;
		_pCh1FrameBaseAddress = _pFrameBaseAddress;
		_pCh2FrameBaseAddress = _pFrameBaseAddress;
		return true;
	}

	// Method:	UnmapFrameBuffers
	bool CNTV2WinDriverInterface::UnmapFrameBuffers (void)
	{
		if (IsRemote())
			return false;
		if (!IsOpen())
			return false;
		ULWord boardIDRegister;
		ReadRegister(kRegBoardID, boardIDRegister);	//unfortunately GetBoardID is in ntv2card...ooops.
		ULWord * pFrameBaseAddress;
		pFrameBaseAddress = _pFrameBaseAddress;
		if (!pFrameBaseAddress)
			return true;

		KSPROPERTY_AJAPROPS_MAPMEMORY_S propStruct;
		DWORD dwBytesReturned = 0;
		ZeroMemory (&propStruct,sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S));
		propStruct.Property.Set			= _GUID_PROPSET;
		propStruct.Property.Id			= KSPROPERTY_AJAPROPS_MAPMEMORY;
		propStruct.Property.Flags		= KSPROPERTY_TYPE_SET;
		propStruct.bMapType				= NTV2_MAPMEMORY_FRAMEBUFFER;
		propStruct.mapMemory.Address	= pFrameBaseAddress;
		BOOL fRet = DeviceIoControl (_hDevice,  IOCTL_AJAPROPS_MAPMEMORY,  &propStruct,  sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S),
									&propStruct,  sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S),  &dwBytesReturned,  NULL);
		_pFrameBaseAddress = 0;
		_pCh1FrameBaseAddress = 0;
		_pCh2FrameBaseAddress = 0;
		if (fRet)
			return true;

		WDIFAIL("failed: " << ::GetKernErrStr(GetLastError()));
		return false;
	}

	// Method:	MapRegisters
	bool CNTV2WinDriverInterface::MapRegisters (void)
	{
		if (IsRemote())
			return false;
		if (!IsOpen())
			return false;
		KSPROPERTY_AJAPROPS_MAPMEMORY_S propStruct;
		DWORD dwBytesReturned = 0;
		_pRegisterBaseAddress = 0;
		_pRegisterBaseAddressLength = 0;
		ZeroMemory (&propStruct,sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S));
		propStruct.Property.Set=_GUID_PROPSET;
		propStruct.Property.Id =KSPROPERTY_AJAPROPS_MAPMEMORY;
		propStruct.Property.Flags = KSPROPERTY_TYPE_GET;
		propStruct.bMapType = NTV2_MAPMEMORY_REGISTER;
		if (!DeviceIoControl (_hDevice,  IOCTL_AJAPROPS_MAPMEMORY,  &propStruct,  sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S),
								&propStruct,  sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S),  &dwBytesReturned,  NULL))
		{
			WDIFAIL("failed: " << ::GetKernErrStr(GetLastError()));
			return false;
		}

		_pRegisterBaseAddress = (ULWord *) propStruct.mapMemory.Address;
		_pRegisterBaseAddressLength = propStruct.mapMemory.Length;
		return true;
	}

	// Method:	UnmapRegisters
	bool CNTV2WinDriverInterface::UnmapRegisters (void)
	{
		if (IsRemote())
			return false;
		if (!IsOpen())
			return false;
		if (_pRegisterBaseAddress == 0)
			return true;

		KSPROPERTY_AJAPROPS_MAPMEMORY_S propStruct;
		DWORD dwBytesReturned = 0;
		ZeroMemory (&propStruct,sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S));
		propStruct.Property.Set			= _GUID_PROPSET;
		propStruct.Property.Id			= KSPROPERTY_AJAPROPS_MAPMEMORY;
		propStruct.Property.Flags		= KSPROPERTY_TYPE_SET;
		propStruct.bMapType				= NTV2_MAPMEMORY_REGISTER;
		propStruct.mapMemory.Address	= _pRegisterBaseAddress;
		BOOL fRet = DeviceIoControl(_hDevice,  IOCTL_AJAPROPS_MAPMEMORY,  &propStruct,  sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S),
									&propStruct,  sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S),  &dwBytesReturned,  NULL);
		_pRegisterBaseAddress = 0;
		_pRegisterBaseAddressLength = 0;
		if (fRet)
			return true;
		WDIFAIL("failed: " << ::GetKernErrStr(GetLastError()));
		return false;
	}

	// Method:	MapXena2Flash
	bool CNTV2WinDriverInterface::MapXena2Flash (void)
	{
		if (IsRemote())
			return false;
		if (!IsOpen())
			return false;
		KSPROPERTY_AJAPROPS_MAPMEMORY_S propStruct;
		DWORD dwBytesReturned = 0;
		_pRegisterBaseAddress = 0;
		_pRegisterBaseAddressLength = 0;
		ZeroMemory (&propStruct,sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S));
		propStruct.Property.Set		= _GUID_PROPSET;
		propStruct.Property.Id		= KSPROPERTY_AJAPROPS_MAPMEMORY;
		propStruct.Property.Flags	= KSPROPERTY_TYPE_GET;
		propStruct.bMapType			= NTV2_MAPMEMORY_PCIFLASHPROGRAM;
		if (!DeviceIoControl (_hDevice,  IOCTL_AJAPROPS_MAPMEMORY,  &propStruct,  sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S),
								&propStruct,  sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S),  &dwBytesReturned,  NULL))
		{
			WDIFAIL("failed: " << ::GetKernErrStr(GetLastError()));
			return false;
		}
		_pXena2FlashBaseAddress = (ULWord *) propStruct.mapMemory.Address;
		_pRegisterBaseAddressLength = propStruct.mapMemory.Length;
		return true;
	}

	// Method:	UnmapXena2Flash
	bool CNTV2WinDriverInterface::UnmapXena2Flash (void)
	{
		if (IsRemote())
			return false;
		if (!IsOpen())
			return false;
		if (_pRegisterBaseAddress == 0)
			return true;

		KSPROPERTY_AJAPROPS_MAPMEMORY_S propStruct;
		DWORD dwBytesReturned = 0;
		ZeroMemory (&propStruct,sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S));
		propStruct.Property.Set			= _GUID_PROPSET;
		propStruct.Property.Id			= KSPROPERTY_AJAPROPS_MAPMEMORY;
		propStruct.Property.Flags		= KSPROPERTY_TYPE_SET;
		propStruct.bMapType				= NTV2_MAPMEMORY_PCIFLASHPROGRAM;
		propStruct.mapMemory.Address	= _pRegisterBaseAddress;
		BOOL fRet = DeviceIoControl (_hDevice,  IOCTL_AJAPROPS_MAPMEMORY,  &propStruct,  sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S),
									&propStruct,  sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S),  &dwBytesReturned,  NULL);
		_pRegisterBaseAddress = 0;
		_pRegisterBaseAddressLength = 0;
		if (fRet)
			return true;
		WDIFAIL("failed: " << ::GetKernErrStr(GetLastError()));
		return false;
	}

	bool CNTV2WinDriverInterface::MapMemory (PVOID pvUserVa, ULWord ulNumBytes, bool bMap, ULWord* ulUser)
	{
		if (IsRemote())
			return false;
		if (!IsOpen())
			return false;

		KSPROPERTY_AJAPROPS_DMA_S propStruct;
		DWORD dwBytesReturned = 0;
		ZeroMemory (&propStruct,sizeof(KSPROPERTY_AJAPROPS_DMA_S));
		propStruct.Property.Set		= _GUID_PROPSET;
		propStruct.Property.Id		= KSPROPERTY_AJAPROPS_DMA;
		propStruct.Property.Flags	= bMap ? KSPROPERTY_TYPE_GET : KSPROPERTY_TYPE_SET;
		propStruct.dmaEngine		= NTV2_PIO;
		propStruct.pvVidUserVa		= pvUserVa;
		propStruct.ulVidNumBytes	= ulNumBytes;
		if (!DeviceIoControl (_hDevice,  IOCTL_AJAPROPS_DMA,  &propStruct,  sizeof(KSPROPERTY_AJAPROPS_DMA_S),
								&propStruct,  sizeof(KSPROPERTY_AJAPROPS_DMA_S),  &dwBytesReturned,  NULL))
		{
			WDIFAIL (KR(GetLastError()) << " -- numBytes=" << ulNumBytes << " map=" << (bMap?"Y":"N"));
			return false;
		}
		if (ulUser)
			*ulUser = propStruct.ulFrameOffset;
		return true;
	}

	// DmaUnlock
	// Called from avclasses destructor to insure the process isn't terminating
	//	with memory still locked - a guaranteed cause of a blue screen
	bool CNTV2WinDriverInterface::DmaUnlock (void)
	{
		if (!IsOpen())
			return false;
		// For every locked entry, try to free it
		for (size_t ndx(0);  ndx < _vecDmaLocked.size();  ndx++)
			CompleteMemoryForDMA(_vecDmaLocked.at(ndx));

		NTV2_ASSERT(_vecDmaLocked.empty()  &&  "_vecDmaLocked should be empty");
		return true;
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	// PrepareMemoryForDMA
	// Passes the address of a user space allocated frame buffer and the buffer's size to the
	//	kernel, where the kernel
	//	creates a MDL and probes & locks the memory.  The framebuffer's userspace address is saved
	//	in the kernel, and all subsequent DMA calls to this address will avoid the time penalties
	//	of the MDL creation and the probe & lock.
	// NOTE: When this method is used to lock new()ed memory, this memory *should*
	//	be unlocked with the CompleteMemoryForDMA() method before calling delete(). The avclasses
	//	destructor does call DmaUnlock() which attempts to unlock all locked pages with calls
	//	to CompleteMemoryForDMA().
	// NOTE: Any memory that is new()ed *should* be delete()ed before the process goes out of
	//	scope.
	bool CNTV2WinDriverInterface::PrepareMemoryForDMA (ULWord * pFrameBuffer, ULWord ulNumBytes)
	{
		if (!IsOpen())
			return false;
		// Use NTV2_PIO as an overloaded flag to indicate this is not a DMA transfer
		if (!DmaTransfer (NTV2_PIO, true, 0, pFrameBuffer, 0, ulNumBytes, false))
			return false;
		// Succeeded -- add pFrameBuffer to the avclasses' vector of locked memory
		_vecDmaLocked.push_back(pFrameBuffer);
		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////
	// CompleteMemoryForDMA: unlock and free resources
	// The inverse of PrepareMemoryForDMA.  It passes the framebuffer address to the kernel, where
	//	the kernel looks up this address and unlocks the memory, unmaps the memory, and deletes the
	//	MDL.
	// NOTE: this method does not cleanup the call to new() which created the memory.  It is the new()
	//	caller's responsibility to call delete() after calling this method to finish the cleanup.
	bool CNTV2WinDriverInterface::CompleteMemoryForDMA (ULWord * pFrameBuffer)
	{
		if (!IsOpen())
			return false;
		// Use NTV2_PIO as an overloaded flag to indicate this is not a DMA transfer
		if (!DmaTransfer (NTV2_PIO, false, 0, pFrameBuffer, 0, 0))
			return false;
		// Succeeded -- remove pFrameBuffer from the avclasses' vector of locked memory
		// Find the entry in the avclasses vector that holds this framebuffer's address
		for (DMA_LOCKED_VEC::iterator vecIter(_vecDmaLocked.begin());  vecIter != _vecDmaLocked.end();  ++vecIter)
			// If we've found one, erase (delete) it
			if (*vecIter == pFrameBuffer)
			{
				_vecDmaLocked.erase(vecIter);
				break;
			}
		return true;
	}
#endif	//	!defined(NTV2_DEPRECATE_16_0)


//////////////////////////////////////////////////////////////////////////////////////
// DMA

bool CNTV2WinDriverInterface::DmaTransfer (	const NTV2DMAEngine	inDMAEngine,
											const bool			inIsRead,
											const ULWord		inFrameNumber,
											ULWord *			pFrameBuffer,
											const ULWord		inOffsetBytes,
											const ULWord		inByteCount,
											const bool			inSynchronous)
{
	if (IsRemote())
		return false;
	if (!IsOpen())
		return false;

	KSPROPERTY_AJAPROPS_DMA_S propStruct;
	DWORD dwBytesReturned = 0;
	ZeroMemory (&propStruct,sizeof(KSPROPERTY_AJAPROPS_DMA_S));
	propStruct.Property.Set		= _GUID_PROPSET;
	propStruct.Property.Id		= KSPROPERTY_AJAPROPS_DMA;
	propStruct.Property.Flags	= inIsRead ? KSPROPERTY_TYPE_GET : KSPROPERTY_TYPE_SET;
	propStruct.dmaEngine		= inDMAEngine;
	propStruct.pvVidUserVa		= PVOID(pFrameBuffer);
	propStruct.ulFrameNumber	= inFrameNumber;
	propStruct.ulFrameOffset	= inOffsetBytes;
	propStruct.ulVidNumBytes	= inByteCount;
	propStruct.ulAudNumBytes	= 0;
	propStruct.bSync			= inSynchronous;
	if (!DeviceIoControl(_hDevice, IOCTL_AJAPROPS_DMA, &propStruct, sizeof(KSPROPERTY_AJAPROPS_DMA_S),
						&propStruct, sizeof(KSPROPERTY_AJAPROPS_DMA_S), &dwBytesReturned, NULL))
	{
		WDIFAIL ("failed: " << ::GetKernErrStr(GetLastError()) << ": eng=" << inDMAEngine << " frm=" << inFrameNumber
				<< " off=" << HEX8(inOffsetBytes) << " len=" << HEX8(inByteCount) << " " << (inIsRead ? "Rd" : "Wr"));
		return false;
	}
	return true;
}


bool CNTV2WinDriverInterface::DmaTransfer (	const NTV2DMAEngine	inDMAEngine,
											const bool			inIsRead,
											const ULWord		inFrameNumber,
											ULWord *			pFrameBuffer,
											const ULWord		inOffsetBytes,
											const ULWord		inByteCount,
											const ULWord		inNumSegments,
											const ULWord		inHostPitch,
											const ULWord		inCardPitch,
											const bool			inSynchronous)
{
	if (IsRemote())
		return false;
	if (!IsOpen())
		return false;

	KSPROPERTY_AJAPROPS_DMA_EX_S propStruct;
	DWORD dwBytesReturned = 0;
	ZeroMemory (&propStruct,sizeof(KSPROPERTY_AJAPROPS_DMA_EX_S));
	propStruct.Property.Set				= _GUID_PROPSET;
	propStruct.Property.Id				= KSPROPERTY_AJAPROPS_DMA_EX;
	propStruct.Property.Flags			= inIsRead ? KSPROPERTY_TYPE_GET : KSPROPERTY_TYPE_SET;
	propStruct.dmaEngine				= inDMAEngine;
	propStruct.pvVidUserVa				= PVOID(pFrameBuffer);
	propStruct.ulFrameNumber			= inFrameNumber;
	propStruct.ulFrameOffset			= inOffsetBytes;
	propStruct.ulVidNumBytes			= inByteCount;
	propStruct.ulAudNumBytes			= 0;
	propStruct.ulVidNumSegments			= inNumSegments;
	propStruct.ulVidSegmentHostPitch	= inHostPitch;
	propStruct.ulVidSegmentCardPitch	= inCardPitch;
	propStruct.bSync					= inSynchronous;
  	if (!DeviceIoControl(_hDevice, IOCTL_AJAPROPS_DMA_EX, &propStruct, sizeof(KSPROPERTY_AJAPROPS_DMA_EX_S),
								&propStruct, sizeof(KSPROPERTY_AJAPROPS_DMA_EX_S), &dwBytesReturned, NULL))
	{
		WDIFAIL ("failed: " << ::GetKernErrStr(GetLastError()) << ": eng=" << inDMAEngine << " frm=" << inFrameNumber
				<< " off=" << HEX8(inOffsetBytes) << " len=" << HEX8(inByteCount) << " " << (inIsRead ? "Rd" : "Wr"));
		return false;
	}
	return true;
}

bool CNTV2WinDriverInterface::DmaTransfer (	const NTV2DMAEngine			inDMAEngine,
											const NTV2Channel			inDMAChannel,
											const bool					inIsTarget,
											const ULWord				inFrameNumber,
											const ULWord				inCardOffsetBytes,
											const ULWord				inByteCount,
											const ULWord				inNumSegments,
											const ULWord				inHostPitch,
											const ULWord				inCardPitch,
											const PCHANNEL_P2P_STRUCT &	inP2PData)
{
	if (IsRemote())
		return false;
	if (!IsOpen())
		return false;
	if (!inP2PData)
	{
		WDIFAIL ("pP2PData == NULL");
		return false;
	}

	DWORD dwBytesReturned(0);
	KSPROPERTY_AJAPROPS_DMA_P2P_S propStruct;
	ZeroMemory (&propStruct,sizeof(KSPROPERTY_AJAPROPS_DMA_P2P_S));

	propStruct.Property.Set		= _GUID_PROPSET;
	propStruct.Property.Id		= KSPROPERTY_AJAPROPS_DMA_P2P;
	if (inIsTarget)
	{	// reset p2p struct
		memset(inP2PData, 0, sizeof(CHANNEL_P2P_STRUCT));
		inP2PData->p2pSize = sizeof(CHANNEL_P2P_STRUCT);
		propStruct.Property.Flags = KSPROPERTY_TYPE_GET;	// get does target
	}
	else
	{	// check for valid p2p struct
		if (inP2PData->p2pSize != sizeof(CHANNEL_P2P_STRUCT))
		{
			WDIFAIL ("p2pSize=" << inP2PData->p2pSize << " != sizeof(CHANNEL_P2P_STRUCT) " <<  sizeof(CHANNEL_P2P_STRUCT));
			return false;
		}
		propStruct.Property.Flags = KSPROPERTY_TYPE_SET;	// set does transfer
	}

	propStruct.dmaEngine				= inDMAEngine;
	propStruct.dmaChannel				= inDMAChannel;
	propStruct.ulFrameNumber			= inFrameNumber;
	propStruct.ulFrameOffset			= inCardOffsetBytes;
	propStruct.ulVidNumBytes			= inByteCount;
	propStruct.ulVidNumSegments			= inNumSegments;
	propStruct.ulVidSegmentHostPitch	= inHostPitch;
	propStruct.ulVidSegmentCardPitch	= inCardPitch;
	propStruct.ullVideoBusAddress		= inP2PData->videoBusAddress;
	propStruct.ullMessageBusAddress		= inP2PData->messageBusAddress;
	propStruct.ulVideoBusSize			= inP2PData->videoBusSize;
	propStruct.ulMessageData			= inP2PData->messageData;
	if (!DeviceIoControl(_hDevice, IOCTL_AJAPROPS_DMA_P2P, &propStruct, sizeof(KSPROPERTY_AJAPROPS_DMA_P2P_S),
						&propStruct, sizeof(KSPROPERTY_AJAPROPS_DMA_P2P_S), &dwBytesReturned, NULL))
	{
		WDIFAIL ("Failed: " << ::GetKernErrStr(GetLastError()) << ": eng=" << inDMAEngine << " ch=" << inDMAChannel
				<< " frm=" << inFrameNumber << " off=" << HEX8(inCardOffsetBytes) << " siz=" << HEX8(inByteCount)
				<< " #segs=" << inNumSegments << " hostPitch=" << inHostPitch << " cardPitch=" << inCardPitch
				<< " target=" << (inIsTarget?"Y":"N"));
		return false;
	}
	if (inIsTarget)
	{
		// check for data returned
		if (dwBytesReturned != sizeof(KSPROPERTY_AJAPROPS_DMA_P2P_S))
		{
			WDIFAIL ("Target failed: " << ::GetKernErrStr(GetLastError()) << " eng=" << inDMAEngine << " ch=" << inDMAChannel
					<< " frm=" << inFrameNumber << " off=" << HEX8(inCardOffsetBytes) << " vSiz=" << HEX8(inByteCount)
					<< " segs=" << inNumSegments << " hostPitch=" << inHostPitch << " cardPitch=" << inCardPitch
					<< " target=" << (inIsTarget?"Y":"N") << " p2pBytesRet=" << HEX8(dwBytesReturned)
					<< " p2pSize=" << HEX8(sizeof(KSPROPERTY_AJAPROPS_DMA_P2P_S)));
			return false;
		}

		// fill in p2p data
		inP2PData->videoBusAddress		= propStruct.ullVideoBusAddress;
		inP2PData->messageBusAddress	= propStruct.ullMessageBusAddress;
		inP2PData->videoBusSize			= propStruct.ulVideoBusSize;
		inP2PData->messageData			= propStruct.ulMessageData;
	}
	return true;
}


///////////////////////////////////////////////////////////////////////////
// AutoCirculate
bool CNTV2WinDriverInterface::AutoCirculate (AUTOCIRCULATE_DATA &autoCircData)
{
	if (IsRemote())
		return CNTV2DriverInterface::AutoCirculate(autoCircData);
	bool bRes(true);
	DWORD dwBytesReturned(0);

	switch (autoCircData.eCommand)
	{
		case eInitAutoCirc:
		{
			if (autoCircData.lVal4 <= 1  &&  autoCircData.lVal5 == 0  &&  autoCircData.lVal6 == 0)
			{
				KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL_S autoCircControl;
				memset(&autoCircControl, 0, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL_S));
				autoCircControl.Property.Set	= _GUID_PROPSET;
				autoCircControl.Property.Id		= KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL;
				autoCircControl.Property.Flags	= KSPROPERTY_TYPE_SET;
				autoCircControl.channelSpec		= autoCircData.channelSpec;
				autoCircControl.eCommand		= autoCircData.eCommand;

				autoCircControl.lVal1 = autoCircData.lVal1;
				autoCircControl.lVal2 = autoCircData.lVal2;
				autoCircControl.lVal3 = autoCircData.lVal3;
				autoCircControl.bVal1 = autoCircData.bVal1;
				autoCircControl.bVal2 = autoCircData.bVal2;
				autoCircControl.bVal3 = autoCircData.bVal3;
				autoCircControl.bVal4 = autoCircData.bVal4;
				autoCircControl.bVal5 = autoCircData.bVal5;
				autoCircControl.bVal6 = autoCircData.bVal6;
				autoCircControl.bVal7 = autoCircData.bVal7;
				autoCircControl.bVal8 = autoCircData.bVal8;
				bRes = DeviceIoControl(_hDevice, IOCTL_AJAPROPS_AUTOCIRC_CONTROL, &autoCircControl, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL_S),
										&autoCircControl, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL_S), &dwBytesReturned, NULL);
				if (!bRes)
					WDIFAIL("ACInit failed: " << ::GetKernErrStr(GetLastError()));
			}
			else
			{
				KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL_EX_S autoCircControl;
				memset(&autoCircControl, 0, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL_EX_S));
				autoCircControl.Property.Set	= _GUID_PROPSET;
				autoCircControl.Property.Id		= KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL_EX;
				autoCircControl.Property.Flags	= KSPROPERTY_TYPE_SET;
				autoCircControl.channelSpec		= autoCircData.channelSpec;
				autoCircControl.eCommand		= autoCircData.eCommand;

				autoCircControl.lVal1 = autoCircData.lVal1;
				autoCircControl.lVal2 = autoCircData.lVal2;
				autoCircControl.lVal3 = autoCircData.lVal3;
				autoCircControl.lVal4 = autoCircData.lVal4;
				autoCircControl.lVal5 = autoCircData.lVal5;
				autoCircControl.lVal6 = autoCircData.lVal6;
				autoCircControl.bVal1 = autoCircData.bVal1;
				autoCircControl.bVal2 = autoCircData.bVal2;
				autoCircControl.bVal3 = autoCircData.bVal3;
				autoCircControl.bVal4 = autoCircData.bVal4;
				autoCircControl.bVal5 = autoCircData.bVal5;
				autoCircControl.bVal6 = autoCircData.bVal6;
				autoCircControl.bVal7 = autoCircData.bVal7;
				autoCircControl.bVal8 = autoCircData.bVal8;
				bRes = DeviceIoControl(_hDevice, IOCTL_AJAPROPS_AUTOCIRC_CONTROL_EX, &autoCircControl, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL_EX_S),
										&autoCircControl, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL_EX_S), &dwBytesReturned, NULL);
				if (!bRes)
					WDIFAIL("ACInitEx failed: " << ::GetKernErrStr(GetLastError()));
			}
			break;
		}	//	eInitAutoCirc

		case eStartAutoCirc:
		case eStopAutoCirc:
		case eAbortAutoCirc:
		case ePauseAutoCirc:
		case eFlushAutoCirculate:
		case ePrerollAutoCirculate:
		case eStartAutoCircAtTime:
		{
			KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL_S autoCircControl;
			memset(&autoCircControl, 0, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL_S));
			autoCircControl.Property.Set	= _GUID_PROPSET;
			autoCircControl.Property.Id		= KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL;
			autoCircControl.Property.Flags	= KSPROPERTY_TYPE_SET;
			autoCircControl.channelSpec		= autoCircData.channelSpec;
			autoCircControl.eCommand		= autoCircData.eCommand;

			switch (autoCircData.eCommand)
			{
				case ePauseAutoCirc:
					autoCircControl.bVal1 = autoCircData.bVal1;
					break;

				case ePrerollAutoCirculate:
					autoCircControl.lVal1 = autoCircData.lVal1;
					break;

				case eStartAutoCircAtTime:
					autoCircControl.lVal1 = autoCircData.lVal1;
					autoCircControl.lVal2 = autoCircData.lVal2;
					break;

				default:	break; //NTV2_ASSERT(false && "Bad eCommand");
			}

			bRes = DeviceIoControl(_hDevice, IOCTL_AJAPROPS_AUTOCIRC_CONTROL, &autoCircControl, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL_S),
									&autoCircControl, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL_S), &dwBytesReturned, NULL);
			if (!bRes)
				WDIFAIL("ACInitEx failed: " << ::GetKernErrStr(GetLastError()));
			break;
		}	//	eStartAutoCirc, eStopAutoCirc, etc.

		case eGetAutoCirc:
		{
			KSPROPERTY_AJAPROPS_AUTOCIRC_STATUS_S autoCircStatus;
			memset(&autoCircStatus, 0, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_STATUS_S));
			autoCircStatus.Property.Set		= _GUID_PROPSET;
			autoCircStatus.Property.Id		= KSPROPERTY_AJAPROPS_AUTOCIRC_STATUS;
			autoCircStatus.Property.Flags	= KSPROPERTY_TYPE_GET;
			autoCircStatus.channelSpec		= autoCircData.channelSpec;
			autoCircStatus.eCommand			= autoCircData.eCommand;
			if (autoCircData.pvVal1)
			{
				bRes = DeviceIoControl(_hDevice, IOCTL_AJAPROPS_AUTOCIRC_STATUS, &autoCircStatus, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_STATUS_S),
										&autoCircStatus, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_STATUS_S), &dwBytesReturned, NULL);
				if (bRes)
					*(AUTOCIRCULATE_STATUS_STRUCT *)autoCircData.pvVal1 = autoCircStatus.autoCircStatus;
				else
					WDIFAIL("GetAC failed: " << ::GetKernErrStr(GetLastError()));
			}
			else
				{bRes = false;  WDIFAIL("GetAC failed: NULL pvVal1");}
			break;
		}	//	eGetAutoCirc

		case eGetFrameStamp:
		{
			KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_S autoCircFrame;
			memset(&autoCircFrame, 0, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_S));
			autoCircFrame.Property.Set	  = _GUID_PROPSET;
			autoCircFrame.Property.Id	  = KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME;
			autoCircFrame.Property.Flags  = KSPROPERTY_TYPE_GET;
			autoCircFrame.channelSpec	= autoCircData.channelSpec;
			autoCircFrame.eCommand		= autoCircData.eCommand;
			autoCircFrame.lFrameNum		= autoCircData.lVal1;

			if (autoCircData.pvVal1)
			{
				autoCircFrame.frameStamp	= *(FRAME_STAMP_STRUCT *) autoCircData.pvVal1;
				bRes = DeviceIoControl(_hDevice, IOCTL_AJAPROPS_AUTOCIRC_FRAME, &autoCircFrame, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_S),
										&autoCircFrame, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_S), &dwBytesReturned, NULL);
				if (bRes)
					*(FRAME_STAMP_STRUCT *)autoCircData.pvVal1 = autoCircFrame.frameStamp;
				else
					WDIFAIL("GetFrameStamp failed: " << ::GetKernErrStr(GetLastError()));
			}
			else
				{bRes = false;  WDIFAIL("GetFrameStamp failed: NULL pvVal1");}
			break;
		}	//	eGetFrameStamp

		case eGetFrameStampEx2:
		{
			KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_EX2_S autoCircFrame;
			memset(&autoCircFrame, 0, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_EX2_S));
			autoCircFrame.Property.Set		= _GUID_PROPSET;
			autoCircFrame.Property.Id		= KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_EX2;
			autoCircFrame.Property.Flags	= KSPROPERTY_TYPE_GET;
			autoCircFrame.channelSpec		= autoCircData.channelSpec;
			autoCircFrame.eCommand			= autoCircData.eCommand;
			autoCircFrame.lFrameNum			= autoCircData.lVal1;

			if (autoCircData.pvVal1)
			{
				autoCircFrame.frameStamp	= *(FRAME_STAMP_STRUCT *) autoCircData.pvVal1;
				if (autoCircData.pvVal2)
					autoCircFrame.acTask	= *(AUTOCIRCULATE_TASK_STRUCT *) autoCircData.pvVal2;

				bRes = DeviceIoControl(_hDevice, IOCTL_AJAPROPS_AUTOCIRC_FRAME_EX2, &autoCircFrame, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_EX2_S),
										&autoCircFrame, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_EX2_S), &dwBytesReturned, NULL);
				if (bRes)
				{
					*(FRAME_STAMP_STRUCT *)autoCircData.pvVal1 = autoCircFrame.frameStamp;
					if (autoCircData.pvVal2)
						*(AUTOCIRCULATE_TASK_STRUCT *) autoCircData.pvVal2 = autoCircFrame.acTask;
				}
				else
					WDIFAIL("GetFrameStampEx2 failed: " << ::GetKernErrStr(GetLastError()));
			}
			else
				{bRes = false;  WDIFAIL("GetFrameStampEx2 failed: NULL pvVal1");}
			break;
		}

		case eTransferAutoCirculate:
		{
			KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_S acXfer;
			memset(&acXfer, 0, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_S));
			acXfer.Property.Set		= _GUID_PROPSET;
			acXfer.Property.Id		= KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER;
			acXfer.Property.Flags	= KSPROPERTY_TYPE_GET;
			acXfer.eCommand			= autoCircData.eCommand;

			if (!autoCircData.pvVal1  ||  !autoCircData.pvVal2)
				{bRes = false; WDIFAIL("ACXfer failed: pvVal1 or pvVal2 NULL"); break;}
			acXfer.acTransfer = *(PAUTOCIRCULATE_TRANSFER_STRUCT) autoCircData.pvVal1;										//	Reqd XferStruct
			AUTOCIRCULATE_TRANSFER_STATUS_STRUCT acStatus = *(PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT) autoCircData.pvVal2;	//	Reqd XferStatusStruct

			//	Ensure audio buffer alignment OK
			if (acXfer.acTransfer.audioBufferSize  &&  (acXfer.acTransfer.audioBufferSize % 4))
				{bRes = false; WDIFAIL("ACXfer failed: audio buffer size " << DEC(acXfer.acTransfer.audioBufferSize) << " not mod 4"); break;}
			if (acXfer.acTransfer.audioBuffer  &&  (ULWord64(acXfer.acTransfer.audioBuffer) % 4))
				{bRes = false; WDIFAIL("ACXfer failed: audio buffer addr " << xHEX0N(acXfer.acTransfer.audioBuffer,16) << " not DWORD-aligned"); break;}

			bRes = DeviceIoControl(_hDevice, IOCTL_AJAPROPS_AUTOCIRC_TRANSFER, &acXfer, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_S),
									&acStatus, sizeof (AUTOCIRCULATE_TRANSFER_STATUS_STRUCT), &dwBytesReturned, NULL);
			if (bRes)
				*(PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT)autoCircData.pvVal2 = acStatus;
			else
				WDIFAIL("ACXfer failed: " << ::GetKernErrStr(GetLastError()));
			break;
		}	//	eTransferAutoCirculate

		case eTransferAutoCirculateEx:
		{
			KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_EX_S acXfer;
			memset(&acXfer, 0, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_EX_S));
			acXfer.Property.Set		= _GUID_PROPSET;
			acXfer.Property.Id		= KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_EX;
			acXfer.Property.Flags	= KSPROPERTY_TYPE_GET;
			acXfer.eCommand			= autoCircData.eCommand;

			if (!autoCircData.pvVal1  ||  !autoCircData.pvVal2)
				{bRes = false; WDIFAIL("ACXferEx failed: NULL XferStruct or XferStatusStruct"); break;}
			acXfer.acTransfer = *(PAUTOCIRCULATE_TRANSFER_STRUCT) autoCircData.pvVal1;										//	Reqd XferStruct
			AUTOCIRCULATE_TRANSFER_STATUS_STRUCT acStatus = *(PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT) autoCircData.pvVal2;	//	Reqd XferStatusStruct
			if (autoCircData.pvVal3)
				acXfer.acTransferRoute = *(NTV2RoutingTable*) autoCircData.pvVal3;											//	Optional RoutingTable

			//	Ensure audio buffer alignment OK
			if (acXfer.acTransfer.audioBufferSize  &&  (acXfer.acTransfer.audioBufferSize % 4))
				{bRes = false; WDIFAIL("ACXferEx failed: audio buffer size " << DEC(acXfer.acTransfer.audioBufferSize) << " not mod 4"); break;}
			if (acXfer.acTransfer.audioBuffer  &&  (ULWord64(acXfer.acTransfer.audioBuffer) % 4))
				{bRes = false; WDIFAIL("ACXferEx failed: audio buffer addr " << xHEX0N(acXfer.acTransfer.audioBuffer,16) << " not DWORD-aligned"); break;}

			bRes = DeviceIoControl(_hDevice, IOCTL_AJAPROPS_AUTOCIRC_TRANSFER_EX, &acXfer, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_EX_S),
									&acStatus, sizeof (AUTOCIRCULATE_TRANSFER_STATUS_STRUCT), &dwBytesReturned, NULL);
			if (bRes)
				*(PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT)autoCircData.pvVal2 = acStatus;
			else
				WDIFAIL("ACXferEx failed: " << ::GetKernErrStr(GetLastError()));
			break;
		}	//	eTransferAutoCirculateEx

		case eTransferAutoCirculateEx2:
		{
			KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_EX2_S acXfer;
			memset(&acXfer, 0, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_EX2_S));
			acXfer.Property.Set		= _GUID_PROPSET;
			acXfer.Property.Id		= KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_EX2;
			acXfer.Property.Flags	= KSPROPERTY_TYPE_GET;
			acXfer.eCommand			= autoCircData.eCommand;

			if (!autoCircData.pvVal1  ||  !autoCircData.pvVal2)
				{bRes = false; WDIFAIL("ACXferEx2 failed: NULL XferStruct or RoutingTable"); break;}

			acXfer.acTransfer = *(PAUTOCIRCULATE_TRANSFER_STRUCT) autoCircData.pvVal1;										//	Reqd XferStruct
			AUTOCIRCULATE_TRANSFER_STATUS_STRUCT acStatus = *(PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT) autoCircData.pvVal2;	//	Reqd XferStatusStruct
			if (autoCircData.pvVal3)
				acXfer.acTransferRoute = *(NTV2RoutingTable*) autoCircData.pvVal3;											//	Optional RoutingTable
			if (autoCircData.pvVal4)
				acXfer.acTask = *(PAUTOCIRCULATE_TASK_STRUCT) autoCircData.pvVal4;											//	Optional TaskStruct

			//	Ensure audio buffer alignment OK
			if (acXfer.acTransfer.audioBufferSize  &&  (acXfer.acTransfer.audioBufferSize % 4))
				{bRes = false; WDIFAIL("ACXferEx2 failed: audio buffer size " << DEC(acXfer.acTransfer.audioBufferSize) << " not mod 4"); break;}
			if (acXfer.acTransfer.audioBuffer  &&  (ULWord64(acXfer.acTransfer.audioBuffer) % 4))
				{bRes = false; WDIFAIL("ACXferEx2 failed: audio buffer addr " << xHEX0N(acXfer.acTransfer.audioBuffer,16) << " not DWORD-aligned"); break;}

			bRes = DeviceIoControl(_hDevice, IOCTL_AJAPROPS_AUTOCIRC_TRANSFER_EX2, &acXfer, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_EX2_S),
									&acStatus, sizeof (AUTOCIRCULATE_TRANSFER_STATUS_STRUCT), &dwBytesReturned, NULL);
			if (bRes)
				*(PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT)autoCircData.pvVal2 = acStatus;
			else
				WDIFAIL("ACXferEx2 failed: " << ::GetKernErrStr(GetLastError()));
			break;
		}	//	eTransferAutoCirculateEx2

		case eSetCaptureTask:
		{
			KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_EX2_S autoCircFrame;
			memset(&autoCircFrame, 0, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_EX2_S));
			autoCircFrame.Property.Set	  = _GUID_PROPSET;
			autoCircFrame.Property.Id	  = KSPROPERTY_AJAPROPS_AUTOCIRC_CAPTURE_TASK;
			autoCircFrame.Property.Flags  = KSPROPERTY_TYPE_SET;
			autoCircFrame.channelSpec	= autoCircData.channelSpec;
			autoCircFrame.eCommand		= autoCircData.eCommand;
			autoCircFrame.lFrameNum		= 0;

			if (!autoCircData.pvVal1)
				{bRes = false; WDIFAIL("ACSetCaptureTask failed: NULL TaskStruct"); break;}
			autoCircFrame.acTask = *(AUTOCIRCULATE_TASK_STRUCT *) autoCircData.pvVal1;			//	Reqd TaskStruct

			bRes = DeviceIoControl(_hDevice, IOCTL_AJAPROPS_AUTOCIRC_CAPTURE_TASK, &autoCircFrame, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_EX2_S),
									&autoCircFrame, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_EX2_S), &dwBytesReturned, NULL);
			if (!bRes)
				WDIFAIL("ACSetCaptureTask failed: " << ::GetKernErrStr(GetLastError()));
			break;
		}	//	eSetCaptureTask

		case eSetActiveFrame:
		case AUTO_CIRC_NUM_COMMANDS:
		{	bRes = false;
			WDIFAIL("Bad AC command %d" << autoCircData.eCommand);
			break;
		}
	}	//	switch on autoCircData.eCommand
	return bRes;
}	//	AutoCirculate


bool CNTV2WinDriverInterface::NTV2Message (NTV2_HEADER * pInMessage)
{
	if (!pInMessage)
		{WDIFAIL("Failed: NULL pointer"); return false;}
	DWORD dwBytesReturned(0);
	if (!DeviceIoControl(_hDevice, IOCTL_AJANTV2_MESSAGE, pInMessage, pInMessage->GetSizeInBytes (), pInMessage, pInMessage->GetSizeInBytes(), &dwBytesReturned, NULL))
		{WDIFAIL("Failed: " << ::GetKernErrStr(GetLastError()));  return false;}
	return true;
}


bool CNTV2WinDriverInterface::HevcSendMessage (HevcMessageHeader* pInMessage)
{
	if (!pInMessage)
		{WDIFAIL("Failed: NULL pointer"); return false;}
	DWORD dwBytesReturned(0);
	if (!DeviceIoControl(_hDevice, IOCTL_AJAHEVC_MESSAGE, pInMessage, pInMessage->size, pInMessage, pInMessage->size, &dwBytesReturned, NULL))
		{WDIFAIL("Failed: " << ::GetKernErrStr(GetLastError()));  return false;}
	return true;
}


bool CNTV2WinDriverInterface::SetAudioOutputMode (NTV2_GlobalAudioPlaybackMode mode)
{
	return WriteRegister(kVRegGlobalAudioPlaybackMode,mode);
}

bool CNTV2WinDriverInterface::GetAudioOutputMode (NTV2_GlobalAudioPlaybackMode* mode)
{
	return mode ? CNTV2DriverInterface::ReadRegister(kVRegGlobalAudioPlaybackMode, *mode) : false;
}


//
// Management of downloaded Xilinx bitfile
//
//
bool CNTV2WinDriverInterface::DriverGetBitFileInformation (BITFILE_INFO_STRUCT & outBitfileInfo, const NTV2BitFileType bitFileType)
{
	if (IsRemote())
		return CNTV2DriverInterface::DriverGetBitFileInformation (outBitfileInfo, bitFileType);
	if (::NTV2DeviceHasSPIFlash(_boardID))	//	No need to query the driver for boards with SPIFlash
		return CNTV2DriverInterface::DriverGetBitFileInformation (outBitfileInfo, bitFileType);

	//	Ask the driver...
	DWORD dwBytesReturned(0);
	KSPROPERTY_AJAPROPS_GETSETBITFILEINFO_S propStruct;
	ZeroMemory(&propStruct,sizeof(KSPROPERTY_AJAPROPS_GETSETBITFILEINFO_S));
	propStruct.Property.Set		= _GUID_PROPSET;
	propStruct.Property.Id		= KSPROPERTY_AJAPROPS_GETSETBITFILEINFO;
	propStruct.Property.Flags	= KSPROPERTY_TYPE_GET;
	if (!DeviceIoControl(_hDevice, IOCTL_AJAPROPS_GETSETBITFILEINFO, &propStruct, sizeof(KSPROPERTY_AJAPROPS_GETSETBITFILEINFO_S),
						&propStruct, sizeof(KSPROPERTY_AJAPROPS_GETSETBITFILEINFO_S), &dwBytesReturned, NULL))
	{
		WDIFAIL("Failed");
		return false;
	}
	outBitfileInfo = propStruct.bitFileInfoStruct;
	return true;
}

bool CNTV2WinDriverInterface::DriverSetBitFileInformation (const BITFILE_INFO_STRUCT & inBitfileInfo)
{
	DWORD dwBytesReturned(0);
 	KSPROPERTY_AJAPROPS_GETSETBITFILEINFO_S propStruct;
	ZeroMemory(&propStruct, sizeof(KSPROPERTY_AJAPROPS_GETSETBITFILEINFO_S));
	propStruct.Property.Set			= _GUID_PROPSET;
	propStruct.Property.Id			= KSPROPERTY_AJAPROPS_GETSETBITFILEINFO;
	propStruct.Property.Flags		= KSPROPERTY_TYPE_SET;
	propStruct.bitFileInfoStruct	= inBitfileInfo;
	if (DeviceIoControl(_hDevice, IOCTL_AJAPROPS_GETSETBITFILEINFO, &propStruct, sizeof(KSPROPERTY_AJAPROPS_GETSETBITFILEINFO_S),
						&propStruct, sizeof(KSPROPERTY_AJAPROPS_GETSETBITFILEINFO_S), &dwBytesReturned, NULL))
		return true;

	WDIFAIL("Failed");
	return false;
}


#include <ntv2devicefeatures.h>

bool CNTV2WinDriverInterface::RestoreHardwareProcampRegisters (void)
{
	return WriteRegister(kVRegRestoreHardwareProcampRegisters, 0);
}
