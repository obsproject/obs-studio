/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2driverinterface.cpp
	@brief		Implements the CNTV2DriverInterface class.
	@copyright	(C) 2003-2021 AJA Video Systems, Inc.
**/

#include "ajatypes.h"
#include "ajaexport.h"
#include "ntv2enums.h"
#include "ntv2debug.h"
#include "ntv2driverinterface.h"
#include "ntv2devicefeatures.h"
#include "ntv2nubaccess.h"
#include "ntv2bitfile.h"
#include "ntv2registers2022.h"
#include "ntv2spiinterface.h"
#include "ntv2utils.h"
#include "ntv2devicescanner.h"	//	for IsHexDigit, IsAlphaNumeric, etc.
#include "ajabase/system/debug.h"
#include "ajabase/common/common.h"
#include "ajabase/system/atomic.h"
#include "ajabase/system/systemtime.h"
#include "ajabase/system/process.h"
#include <string.h>
#include <assert.h>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <map>

using namespace std;

#define INSTP(_p_)			HEX0N(uint64_t(_p_),16)
#define	DIFAIL(__x__)		AJA_sERROR  (AJA_DebugUnit_DriverInterface, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	DIWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_DriverInterface, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	DINOTE(__x__)		AJA_sNOTICE (AJA_DebugUnit_DriverInterface, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	DIINFO(__x__)		AJA_sINFO   (AJA_DebugUnit_DriverInterface, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	DIDBG(__x__)		AJA_sDEBUG  (AJA_DebugUnit_DriverInterface, INSTP(this) << "::" << AJAFUNC << ": " << __x__)

//	Stats
static uint32_t	gConstructCount(0);	//	Number of constructor calls made
static uint32_t	gDestructCount(0);	//	Number of destructor calls made
static uint32_t	gOpenCount(0);		//	Number of successful Open calls made
static uint32_t	gCloseCount(0);		//	Number of Close calls made


///////////////	CLASS METHODS

NTV2StringList CNTV2DriverInterface::GetLegalSchemeNames (void)
{
	NTV2StringList result;
	result.push_back("ntv2nub"); result.push_back("ntv2"); result.push_back("ntv2local");
	return result;
}

static bool		gSharedMode(false);
void CNTV2DriverInterface::SetShareMode (const bool inSharedMode)		{gSharedMode = inSharedMode;}
bool CNTV2DriverInterface::GetShareMode (void)	{return gSharedMode;}
static bool		gOverlappedMode(false);
void CNTV2DriverInterface::SetOverlappedMode (const bool inOverlapMode)	{gOverlappedMode = inOverlapMode;}
bool CNTV2DriverInterface::GetOverlappedMode (void)	{return gOverlappedMode;}


///////////////	INSTANCE METHODS

CNTV2DriverInterface::CNTV2DriverInterface ()
	:	_boardNumber					(0),
		_boardID						(DEVICE_ID_NOTFOUND),
		_boardOpened					(false),
#if defined(NTV2_WRITEREG_PROFILING)
		mRecordRegWrites				(false),
		mSkipRegWrites					(false),
#endif
		_programStatus					(0),
#if defined (NTV2_NUB_CLIENT_SUPPORT)
		_pRPCAPI						(AJA_NULL),
#endif	//	defined (NTV2_NUB_CLIENT_SUPPORT)
		mInterruptEventHandles			(),
		mEventCounts					(),
#if defined(NTV2_WRITEREG_PROFILING)
		mRegWrites						(),
		mRegWritesLock					(),
#endif	//	NTV2_WRITEREG_PROFILING
#if !defined(NTV2_DEPRECATE_16_0)
		_pFrameBaseAddress				(AJA_NULL),
		_pRegisterBaseAddress			(AJA_NULL),
		_pRegisterBaseAddressLength		(0),
		_pXena2FlashBaseAddress			(AJA_NULL),
		_pCh1FrameBaseAddress			(AJA_NULL),
		_pCh2FrameBaseAddress			(AJA_NULL),
#endif	//	!defined(NTV2_DEPRECATE_16_0)
		_ulNumFrameBuffers				(0),
		_ulFrameBufferSize				(0),
		_pciSlot						(0)			//	DEPRECATE!
{
	mInterruptEventHandles.reserve(eNumInterruptTypes);
	while (mInterruptEventHandles.size() < eNumInterruptTypes)
		mInterruptEventHandles.push_back(AJA_NULL);

	mEventCounts.reserve(eNumInterruptTypes);
	while (mEventCounts.size() < eNumInterruptTypes)
		mEventCounts.push_back(0);
	AJAAtomic::Increment(&gConstructCount);
	DIDBG(DEC(gConstructCount) << " constructed, " << DEC(gDestructCount) << " destroyed");
}	//	constructor


CNTV2DriverInterface::~CNTV2DriverInterface ()
{
	AJAAtomic::Increment(&gDestructCount);
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	if (_pRPCAPI)
		delete _pRPCAPI;
	_pRPCAPI = AJA_NULL;
#endif	//	defined (NTV2_NUB_CLIENT_SUPPORT)
	DIDBG(DEC(gConstructCount) << " constructed, " << DEC(gDestructCount) << " destroyed");
}	//	destructor

CNTV2DriverInterface & CNTV2DriverInterface::operator = (const CNTV2DriverInterface & inRHS)
{	(void) inRHS;	NTV2_ASSERT(false && "Not assignable");	return *this;}	//	operator =

CNTV2DriverInterface::CNTV2DriverInterface (const CNTV2DriverInterface & inObjToCopy)
{	(void) inObjToCopy;	NTV2_ASSERT(false && "Not copyable");}	//	copy constructor


//	Open local physical device (via ajantv2 driver)
bool CNTV2DriverInterface::Open (const UWord inDeviceIndex)
{
	if (IsOpen()  &&  inDeviceIndex == _boardNumber)
		return true;	//	Same local device requested, already open
	Close();
	if (inDeviceIndex >= MaxNumDevices())
		{DIFAIL("Requested device index '" << DEC(inDeviceIndex) << "' at/past limit of '" << DEC(MaxNumDevices()) << "'"); return false;}
	if (!OpenLocalPhysical(inDeviceIndex))
		return false;

	// Read driver version...
	uint16_t	drvrVersComps[4]	=	{0, 0, 0, 0};
	ULWord		driverVersionRaw	(0);
	if (!IsRemote()  &&  !ReadRegister (kVRegDriverVersion, driverVersionRaw))
		{DIFAIL("ReadRegister(kVRegDriverVersion) failed");  Close();  return false;}
	drvrVersComps[0] = uint16_t(NTV2DriverVersionDecode_Major(driverVersionRaw));	//	major
	drvrVersComps[1] = uint16_t(NTV2DriverVersionDecode_Minor(driverVersionRaw));	//	minor
	drvrVersComps[2] = uint16_t(NTV2DriverVersionDecode_Point(driverVersionRaw));	//	point
	drvrVersComps[3] = uint16_t(NTV2DriverVersionDecode_Build(driverVersionRaw));	//	build

	//	Check driver version (local devices only)
	NTV2_ASSERT(!IsRemote());
	if (!(AJA_NTV2_SDK_VERSION_MAJOR))
		DIWARN ("Driver version v" << DEC(drvrVersComps[0]) << "." << DEC(drvrVersComps[1]) << "." << DEC(drvrVersComps[2]) << "."
				<< DEC(drvrVersComps[3]) << " ignored for client SDK v0.0.0.0 (dev mode), driverVersionRaw=" << xHEX0N(driverVersionRaw,8));
	else if (drvrVersComps[0] == uint16_t(AJA_NTV2_SDK_VERSION_MAJOR))
		DIDBG ("Driver v" << DEC(drvrVersComps[0]) << "." << DEC(drvrVersComps[1])
				<< "." << DEC(drvrVersComps[2]) << "." << DEC(drvrVersComps[3]) << " == client SDK v"
				<< DEC(uint16_t(AJA_NTV2_SDK_VERSION_MAJOR)) << "." << DEC(uint16_t(AJA_NTV2_SDK_VERSION_MINOR))
				<< "." << DEC(uint16_t(AJA_NTV2_SDK_VERSION_POINT)) << "." << DEC(uint16_t(AJA_NTV2_SDK_BUILD_NUMBER)));
	else
		DIWARN ("Driver v" << DEC(drvrVersComps[0]) << "." << DEC(drvrVersComps[1])
				<< "." << DEC(drvrVersComps[2]) << "." << DEC(drvrVersComps[3]) << " != client SDK v"
				<< DEC(uint16_t(AJA_NTV2_SDK_VERSION_MAJOR)) << "." << DEC(uint16_t(AJA_NTV2_SDK_VERSION_MINOR)) << "."
				<< DEC(uint16_t(AJA_NTV2_SDK_VERSION_POINT)) << "." << DEC(uint16_t(AJA_NTV2_SDK_BUILD_NUMBER))
				<< ", driverVersionRaw=" << xHEX0N(driverVersionRaw,8));

	FinishOpen();
	AJAAtomic::Increment(&gOpenCount);
	DIDBG(DEC(gOpenCount) << " opened, " << DEC(gCloseCount) << " closed");
	return true;
}

//	Open remote or virtual device
bool CNTV2DriverInterface::Open (const std::string & inURLSpec)
{
	Close();
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	if (OpenRemote(inURLSpec))
	{
		FinishOpen();
		AJAAtomic::Increment(&gOpenCount);
		DIDBG(DEC(gOpenCount) << " opens, " << DEC(gCloseCount) << " closes");
		return true;
	}
#endif	//	defined (NTV2_NUB_CLIENT_SUPPORT)
	return false;
}

#if !defined(NTV2_DEPRECATE_14_3)
	bool CNTV2DriverInterface::Open (UWord boardNumber, bool displayError, NTV2DeviceType eBoardType, const char* hostname)
	{	(void) eBoardType;  (void) displayError;	//	Ignored
		const string host(hostname ? hostname : "");
		if (host.empty())
			return Open(boardNumber);
		return Open(host);
	}
#endif	//	!defined(NTV2_DEPRECATE_14_3)


bool CNTV2DriverInterface::Close (void)
{
	if (IsOpen())
	{
		// Unsubscribe all...
		for (INTERRUPT_ENUMS eInt(eNumInterruptTypes);  eInt < eNumInterruptTypes;  eInt = INTERRUPT_ENUMS(eInt+1))
			ConfigureSubscription (false, eInt, mInterruptEventHandles[eInt]);

		bool closeOK(true);
#if defined (NTV2_NUB_CLIENT_SUPPORT)
		if (IsRemote())
			closeOK = CloseRemote();
		else
#endif	//	defined (NTV2_NUB_CLIENT_SUPPORT)
		{	//	Local/physical device:
			closeOK = CloseLocalPhysical();
			//	Common to all platforms:
#if !defined(NTV2_DEPRECATE_16_0)
			DmaUnlock();
			UnmapFrameBuffers();
			UnmapRegisters();
#endif	//	!defined(NTV2_DEPRECATE_16_0)
		}
		if (closeOK)
			AJAAtomic::Increment(&gCloseCount);
		_boardID = DEVICE_ID_NOTFOUND;
		DIDBG(DEC(gOpenCount) << " opens, " << DEC(gCloseCount) << " closes");
		return closeOK;
	}
	return true;

}	//	Close


bool CNTV2DriverInterface::OpenLocalPhysical (const UWord inDeviceIndex)
{	(void) inDeviceIndex;
	NTV2_ASSERT(false && "Requires platform-specific implementation");
	return false;
}

bool CNTV2DriverInterface::CloseLocalPhysical (void)
{
	NTV2_ASSERT(false && "Requires platform-specific implementation");
	return false;
}


#if defined (NTV2_NUB_CLIENT_SUPPORT)

	#ifdef AJA_WINDOWS
		static bool winsock_inited = false;
		static WSADATA wsaData;

		static void initWinsock(void)
		{
			int wret;
			if (!winsock_inited)
				wret = WSAStartup(MAKEWORD(2,2), &wsaData);
			winsock_inited = true;
		}
	#endif	//	AJA_WINDOWS

	bool CNTV2DriverInterface::OpenRemote (const string & inURLSpec)
	{
#if defined(AJA_WINDOWS)
		initWinsock();
#endif	//	defined(AJA_WINDOWS)
		NTV2_ASSERT(!IsOpen());	//	Must be closed!

		string cleanURL(inURLSpec);
		aja::strip(cleanURL);
		if (cleanURL.empty())
			{DIFAIL("Empty URLSpec");  return false;}
		string urlspec(cleanURL);
		aja::lower(urlspec);

		/*
			Parse the URLSpec and figure out what to open...

			{ipv4}
				For backward compatibility. Uses the nub interface, default port, device index 0.
				...where...		{ipv4}		A dotted quad or 'localhost' that designates the host IP address.
			'ntv2nub://'{ipv4}[':'{port}]['/'[query]]
				This uses the nub interface.
				...where...		{ipv4}		A dotted quad or 'localhost' that designates the host IP address.
								port		Optional unsigned decimal integer that designates the port number to use.
								query		Optionally specifies which device to open on the remote host.
			'ntv2://'{name}['/'[query]
				...where...		{name}		Alphanumeric name that designates a software device to open (e.g. dylib/DLL name).
											This is typically case-insensitve, and should not contain characters that require URL-encoding.
								query		Optionally specifies any number of parameters passed to dylib/DLL's Connect method.
											[? {param}[={value}] [&...]]
												...where...		{param}		The query parameter to be specified.
																			Query parameter names are normally not case-sensitive,
																			and should not require URL-encoding.
																{value}		Optionally specifies a URL-encoded value for the parameter.
			'ntv2local://'{model}|{hexnum}|{serial}|{indexnum}
				...where...		{model}		NTV2 device model name (e.g. 'corvid44').
								{hexnum}	NTV2DeviceID: '0x'|'0X' followed by at least 2 and up to 8 hex digits.
								{serial}	Device serial number
								{indexnum}	Device index number:  at least 1 and up to 2 decimal digits.
		*/
		NTV2StringList tokens, delims;
		{
			string	run; bool isAlphaNumeric(false);
			for (size_t pos(0);  pos < urlspec.length();  pos++)
			{
				const char c(urlspec.at(pos));
				if (c == '\t' || c == ' ')
					continue;
				if (run.empty())
				{	//	Start a new run...
					isAlphaNumeric = CNTV2DeviceScanner::IsAlphaNumeric(c);
					run += c;
					continue;
				}
				if (isAlphaNumeric == CNTV2DeviceScanner::IsAlphaNumeric(c))
				{	//	Extend existing run...
					run += c;
					continue;
				}
				//	Terminate this run...
				NTV2_ASSERT(!run.empty());
				if (isAlphaNumeric)
					tokens.push_back(run);
				else
					delims.push_back(run);
				run.clear();
				pos--;	//	Do this character again
			}
			//	Terminate this run...
			if (!run.empty())
			{
				if (isAlphaNumeric)
					tokens.push_back(run);
				else
					delims.push_back(run);
			}
			while (tokens.size() < 5)
				tokens.push_back("");
			while (delims.size() < 5)
				delims.push_back("");
			DIDBG("'" << urlspec << "':\tTOK[" << aja::join(tokens, "|") << "]\tDEL[" << aja::join(delims, "|") << "]");
		}
		string scheme("ntv2local"), ipv4("localhost"), hostname, port;
		if (delims[0] == "://")
		{	//	Pop scheme from front...
			scheme = tokens[0];
			tokens.erase(tokens.begin());	tokens.push_back("");
			delims.erase(delims.begin());	delims.push_back("");
		}
		//if (!scheme.empty())
		{
			if (CNTV2DeviceScanner::IsLegalDecimalNumber(tokens[0],3) && delims[0] == "."
				&& CNTV2DeviceScanner::IsLegalDecimalNumber(tokens[1],3) && delims[1] == "."
				&& CNTV2DeviceScanner::IsLegalDecimalNumber(tokens[2],3) && delims[2] == "."
				&& CNTV2DeviceScanner::IsLegalDecimalNumber(tokens[3],3))
			{
				ipv4 = tokens[0] + "." + tokens[1] + "." + tokens[2] + "." + tokens[3];
				tokens.erase(tokens.begin());	tokens.erase(tokens.begin());	tokens.erase(tokens.begin());	tokens.erase(tokens.begin());
				delims.erase(delims.begin());	delims.erase(delims.begin());	delims.erase(delims.begin());
			}
			else if (CNTV2DeviceScanner::IsAlphaNumeric(tokens[0]))// && delims[0] == ".")
			{
				while (CNTV2DeviceScanner::IsAlphaNumeric(tokens[0]) && delims[0] == ".")
				{
					hostname += tokens[0];	tokens.erase(tokens.begin());	tokens.push_back("");
					hostname += delims[0];	delims.erase(delims.begin());	delims.push_back("");
				}
				if (CNTV2DeviceScanner::IsAlphaNumeric(tokens[0]))// && delims[0] != ".")
					{hostname += tokens[0];	tokens.erase(tokens.begin());	tokens.push_back("");}
			}
			else
				{DIFAIL("Expected IPv4 address, 'localhost', host name -- instead got '" << urlspec << "'");  return false;}
			if (hostname == "localhost")
				{ipv4 = hostname;  hostname = "";}
			if (CNTV2DeviceScanner::IsLegalDecimalNumber(tokens[0],5) && delims[0] == ":")
			{
				port = tokens[0];	tokens.erase(tokens.begin());	tokens.push_back("");
									delims.erase(delims.begin());	delims.push_back("");
			}
			if (scheme == "ntv2local")
			{	//	Local device (yes, via urlSpec!)
				if (!hostname.empty())
				{	CNTV2Card card;
					if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument(hostname, card))
						{DIFAIL("Failed to open '" << urlspec << "'"); return false;}
					return Open(card.GetIndexNumber());
				}
				else if (!ipv4.empty())
					scheme = "ntv2nub";
			}
			if (scheme == "ntv2nub")
			{
				//cerr << "TOK: " << aja::join(tokens, "|") << endl << "DEL: " << aja::join(delims, "|") << endl;
				_pRPCAPI = NTV2RPCAPI::MakeNTV2NubRPCAPI(ipv4, port);
			}
			else if (scheme == "ntv2")
			{	//	Software device:  dylib/DLL name is in "hostname"
				NTV2StringList halves(aja::split(cleanURL, "://"));
				if (halves.size() != 2)
					{DIFAIL("'ntv2://' scheme has another '://' in the URL (should be URLencoded)"); return false;}
				const string hostQuery (halves.at(1));	//	e.g. "ntv2swdevice/etc"
				halves = aja::split(hostQuery, "/");
				string checkHost(halves.empty() ? hostQuery : halves.at(0));
				aja::lower(checkHost);
				if (checkHost != hostname)
					{DIFAIL("'ntv2://' scheme has mismatched name: '" << checkHost << "' != '" << hostname << "'"); return false;}
				string query(halves.size() < 2 ? "" : halves.at(1));
				if (!query.empty()  &&  query.at(0) == '?')
					query.erase(0,1);	//	Remove leading '?'
				DIDBG("scheme='ntv2' host='" << hostname << "' query='" << query << "'");
				_pRPCAPI = NTV2RPCAPI::FindNTV2SoftwareDevice(hostname, query);
			}
			else
				{DIFAIL("Invalid URL scheme '" << scheme << "' in '" << urlspec << "'");  return false;}
		}
/*		if (delims[0] == ":")
		{
		}
		else if (delims[0] == "/")
		{
		}
		else if (delims[0] == "")
		{
		}
		else
		{
			DIFAIL("Expected URL, IPv4 address or 'localhost', instead got '" << urlspec << "'");
			return false;
		}*/
		if (IsRemote())
			_boardOpened = ReadRegister(kRegBoardID, _boardID);
		if (!IsRemote() || !IsOpen())
			DIFAIL("Failed to open '" << urlspec << "'");
		return IsRemote() && IsOpen();
	}	//	OpenRemote


	bool CNTV2DriverInterface::CloseRemote()
	{
		if (_pRPCAPI)
		{
			DIINFO("Remote closed: " << *_pRPCAPI);
			delete _pRPCAPI;
			_pRPCAPI = AJA_NULL;
			_boardOpened = false;
			return true;
		}
		//	Wasn't open
		_boardOpened = false;
		return false;
	}
#endif	//	defined (NTV2_NUB_CLIENT_SUPPORT)


bool CNTV2DriverInterface::GetInterruptEventCount (const INTERRUPT_ENUMS inInterrupt, ULWord & outCount)
{
	outCount = 0;
	if (!NTV2_IS_VALID_INTERRUPT_ENUM(inInterrupt))
		return false;
	outCount = mEventCounts.at(inInterrupt);
	return true;
}

bool CNTV2DriverInterface::SetInterruptEventCount (const INTERRUPT_ENUMS inInterrupt, const ULWord inCount)
{
	if (!NTV2_IS_VALID_INTERRUPT_ENUM(inInterrupt))
		return false;
	mEventCounts.at(inInterrupt) = inCount;
	return true;
}

bool CNTV2DriverInterface::GetInterruptCount (const INTERRUPT_ENUMS eInterrupt,  ULWord & outCount)
{	(void) eInterrupt;
	outCount = 0;
	NTV2_ASSERT(false && "Needs subclass implementation");
	return false;
}

HANDLE CNTV2DriverInterface::GetInterruptEvent (const INTERRUPT_ENUMS eInterruptType)
{
	if (!NTV2_IS_VALID_INTERRUPT_ENUM(eInterruptType))
		return HANDLE(0);
	return HANDLE(uint64_t(mInterruptEventHandles.at(eInterruptType)));
}

bool CNTV2DriverInterface::ConfigureInterrupt (const bool bEnable, const INTERRUPT_ENUMS eInterruptType)
{	(void) bEnable;  (void) eInterruptType;
	NTV2_ASSERT(false && "Needs subclass implementation");
	return false;
}

bool CNTV2DriverInterface::ConfigureSubscription (const bool bSubscribe, const INTERRUPT_ENUMS eInterruptType, PULWord & outSubscriptionHdl)
{
	if (!NTV2_IS_VALID_INTERRUPT_ENUM(eInterruptType))
		return false;
	outSubscriptionHdl = mInterruptEventHandles.at(eInterruptType);
	if (bSubscribe)
	{										//	If subscribing,
		mEventCounts [eInterruptType] = 0;	//		clear this interrupt's event counter
		DIDBG("Subscribing '" << ::NTV2InterruptEnumString(eInterruptType) << "' (" << UWord(eInterruptType)
				<< "), event counter reset");
	}
 	else
 	{
		DIDBG("Unsubscribing '" << ::NTV2InterruptEnumString(eInterruptType) << "' (" << UWord(eInterruptType) << "), "
				<< mEventCounts[eInterruptType] << " event(s) received");
	}
	return true;

}	//	ConfigureSubscription


NTV2DeviceID CNTV2DriverInterface::GetDeviceID (void)
{
	ULWord value(0);
	if (IsOpen()  &&  ReadRegister(kRegBoardID, value))
	{
#if 0	//	Fake out:
	if (value == ULWord(DEVICE_ID_CORVID88))	//	Pretend a Corvid88 is a TTapPro
		value = ULWord(DEVICE_ID_TTAP_PRO);
#endif
		const NTV2DeviceID currentValue(NTV2DeviceID(value+0));
		if (currentValue != _boardID)
			DIWARN(xHEX0N(this,16) << ":  NTV2DeviceID " << xHEX0N(value,8) << " (" << ::NTV2DeviceIDToString(currentValue)
					<< ") read from register " << kRegBoardID << " doesn't match _boardID " << xHEX0N(_boardID,8) << " ("
					<< ::NTV2DeviceIDToString(_boardID) << ")");
		return currentValue;
	}
	return DEVICE_ID_NOTFOUND;
}


// Common remote card read register.  Subclasses have overloaded function
// that does platform-specific read of register on local card.
bool CNTV2DriverInterface::ReadRegister (const ULWord inRegNum, ULWord & outValue, const ULWord inMask, const ULWord inShift)
{
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	if (IsRemote())
		return !_pRPCAPI->NTV2ReadRegisterRemote (inRegNum, outValue, inMask, inShift);
#else
	(void) inRegNum;	(void) outValue;	(void) inMask;	(void) inShift;
#endif
	return false;
}

bool CNTV2DriverInterface::ReadRegisters (NTV2RegisterReads & inOutValues)
{
	if (!IsOpen())
		return false;		//	Device not open!
	if (inOutValues.empty())
		return true;		//	Nothing to do!

	NTV2GetRegisters getRegsParams (inOutValues);
	if (NTV2Message(reinterpret_cast<NTV2_HEADER*>(&getRegsParams)))
	{
		if (!getRegsParams.GetRegisterValues(inOutValues))
			return false;
	}
	else	//	Non-atomic user-space workaround until GETREGS implemented in driver...
		for (NTV2RegisterReadsIter iter(inOutValues.begin());  iter != inOutValues.end();  ++iter)
			if (iter->registerNumber != kRegXenaxFlashDOUT)	//	Prevent firmware erase/program/verify failures
				if (!ReadRegister (iter->registerNumber, iter->registerValue))
					return false;
	return true;
}

#if !defined(NTV2_DEPRECATE_16_0)
	// Common remote card read multiple registers.  Subclasses have overloaded function
	bool CNTV2DriverInterface::ReadRegisterMulti (const ULWord inNumRegs, ULWord * pOutWhichRegFailed, NTV2RegInfo pOutRegInfos[])
	{
		if (!pOutWhichRegFailed)
			return false;	//	NULL pointer
		*pOutWhichRegFailed = 0xFFFFFFFF;
		if (!inNumRegs)
			return false;	//	numRegs is zero
		#if defined (NTV2_NUB_CLIENT_SUPPORT)
		if (IsRemote())
			return !_pRPCAPI->NTV2ReadRegisterMultiRemote(inNumRegs, *pOutWhichRegFailed, pOutRegInfos);
		#endif	//	defined (NTV2_NUB_CLIENT_SUPPORT)

	#if 0	//	Original implementation
		for (ULWord ndx(0);  ndx < inNumRegs;  ndx++)
		{
			NTV2RegInfo & regInfo(pOutRegInfos[ndx]);
			if (!ReadRegister (regInfo.registerNumber, regInfo.registerValue, regInfo.registerMask, regInfo.registerShift))
			{
				*pOutWhichRegFailed = regInfo.registerNumber;
				return false;
			}
		}
		return true;
	#endif
		//	New in SDK 16.0:  Use ReadRegs NTV2Message
		NTV2RegReads regReads, result;
		regReads.reserve(inNumRegs);  result.reserve(inNumRegs);
		for (size_t ndx(0);  ndx < size_t(inNumRegs);  ndx++)
			regReads.push_back(pOutRegInfos[ndx]);
		result = regReads;
		bool retVal (ReadRegisters(result));
		NTV2_ASSERT(result.size() <= regReads.size());
		if (result.size() < regReads.size())
			*pOutWhichRegFailed = result.empty() ? regReads.front().registerNumber : result.back().registerNumber;
		return retVal;
	}

	Word CNTV2DriverInterface::SleepMs (const LWord milliseconds)
	{
		AJATime::Sleep(milliseconds);
		return 0; // Beware, this function always returns zero, even if sleep was interrupted
	}
#endif	//	!defined(NTV2_DEPRECATE_16_0)


// Common remote card write register.  Subclasses overloaded this to do platform-specific register write.
bool CNTV2DriverInterface::WriteRegister (const ULWord inRegNum, const ULWord inValue, const ULWord inMask, const ULWord inShift)
{
#if defined(NTV2_WRITEREG_PROFILING)
	//	Recording is done in platform-specific WriteRegister
#endif	//	NTV2_WRITEREG_PROFILING
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	//	If we get here, must be a non-physical device connection...
	return IsRemote() ? !_pRPCAPI->NTV2WriteRegisterRemote(inRegNum, inValue, inMask, inShift) : false;
#else
	(void) inRegNum;	(void) inValue;	(void) inMask;	(void) inShift;
	return false;
#endif
}


bool CNTV2DriverInterface::DmaTransfer (const NTV2DMAEngine	inDMAEngine,
										const bool			inIsRead,
										const ULWord		inFrameNumber,
										ULWord *			pFrameBuffer,
										const ULWord		inCardOffsetBytes,
										const ULWord		inTotalByteCount,
										const bool			inSynchronous)
{
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	NTV2_ASSERT(IsRemote());
	return !_pRPCAPI->NTV2DMATransferRemote(inDMAEngine, inIsRead, inFrameNumber, pFrameBuffer, inCardOffsetBytes,
											inTotalByteCount, 0/*numSegs*/,  0/*hostPitch*/,  0/*cardPitch*/, inSynchronous);
#else
	(void) inDMAEngine;	(void) inIsRead;	(void) inFrameNumber;	(void) pFrameBuffer;	(void) inCardOffsetBytes;
	(void) inTotalByteCount;	(void) inSynchronous;
	return false;
#endif
}

bool CNTV2DriverInterface::DmaTransfer (const NTV2DMAEngine	inDMAEngine,
										const bool			inIsRead,
										const ULWord		inFrameNumber,
										ULWord *			pFrameBuffer,
										const ULWord		inCardOffsetBytes,
										const ULWord		inTotalByteCount,
										const ULWord		inNumSegments,
										const ULWord		inHostPitchPerSeg,
										const ULWord		inCardPitchPerSeg,
										const bool			inSynchronous)
{
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	NTV2_ASSERT(IsRemote());
	return !_pRPCAPI->NTV2DMATransferRemote(inDMAEngine, inIsRead, inFrameNumber, pFrameBuffer, inCardOffsetBytes,
											inTotalByteCount, inNumSegments, inHostPitchPerSeg, inCardPitchPerSeg,
											inSynchronous);
#else
	(void) inDMAEngine;	(void) inIsRead;	(void) inFrameNumber;	(void) pFrameBuffer;	(void) inCardOffsetBytes;
	(void) inTotalByteCount;	(void) inNumSegments;	(void) inHostPitchPerSeg;	(void) inCardPitchPerSeg;	(void) inSynchronous;
	return false;
#endif
}

bool CNTV2DriverInterface::DmaTransfer (const NTV2DMAEngine inDMAEngine,
									const NTV2Channel inDMAChannel,
									const bool inIsTarget,
									const ULWord inFrameNumber,
									const ULWord inCardOffsetBytes,
									const ULWord inByteCount,
									const ULWord inNumSegments,
									const ULWord inSegmentHostPitch,
									const ULWord inSegmentCardPitch,
									const PCHANNEL_P2P_STRUCT & inP2PData)
{	(void) inDMAEngine;	(void) inDMAChannel; (void) inIsTarget; (void) inFrameNumber; (void) inCardOffsetBytes;
	(void) inByteCount; (void) inNumSegments; (void) inSegmentHostPitch; (void) inSegmentCardPitch; (void) inP2PData;
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	NTV2_ASSERT(IsRemote());
	//	No NTV2DMATransferP2PRemote implementation yet
#endif
	return false;
}

// Common remote card waitforinterrupt.  Subclasses have overloaded function
// that does platform-specific waitforinterrupt on local cards.
bool CNTV2DriverInterface::WaitForInterrupt (INTERRUPT_ENUMS eInterrupt, ULWord timeOutMs)
{
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	NTV2_ASSERT(IsRemote());
	return !_pRPCAPI->NTV2WaitForInterruptRemote(eInterrupt, timeOutMs);
#else
	(void) eInterrupt;
	(void) timeOutMs;
	return false;
#endif
}

// Common remote card autocirculate.  Subclasses have overloaded function
// that does platform-specific autocirculate on local cards.
bool CNTV2DriverInterface::AutoCirculate (AUTOCIRCULATE_DATA & autoCircData)
{
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	NTV2_ASSERT(IsRemote());

	switch(autoCircData.eCommand)
	{
		case eStartAutoCirc:
		case eAbortAutoCirc:
		case ePauseAutoCirc:
		case eFlushAutoCirculate:
		case eGetAutoCirc:
		case eStopAutoCirc:
			return !_pRPCAPI->NTV2AutoCirculateRemote(autoCircData);
		default:	// Others not handled
			return false;
	}
#else
	(void) autoCircData;
	return false;
#endif
}

bool CNTV2DriverInterface::NTV2Message (NTV2_HEADER * pInMessage)
{
	(void) pInMessage;
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	NTV2_ASSERT(IsRemote());
	return !_pRPCAPI->NTV2MessageRemote(pInMessage);
#else
	return false;
#endif
}


// Common remote card DriverGetBitFileInformation.  Subclasses have overloaded function
// that does platform-specific function on local cards.
bool CNTV2DriverInterface::DriverGetBitFileInformation (BITFILE_INFO_STRUCT & bitFileInfo, const NTV2BitFileType bitFileType)
{
	if (IsRemote())
#if defined (NTV2_NUB_CLIENT_SUPPORT)
		return !_pRPCAPI->NTV2DriverGetBitFileInformationRemote(bitFileInfo, bitFileType);
#else
		return false;
#endif
	if (!::NTV2DeviceHasSPIFlash(_boardID))
		return false;

	ParseFlashHeader(bitFileInfo);
	bitFileInfo.bitFileType = 0;
	switch (_boardID)
	{
		case DEVICE_ID_CORVID1:						bitFileInfo.bitFileType = NTV2_BITFILE_CORVID1_MAIN;				break;
		case DEVICE_ID_CORVID22:					bitFileInfo.bitFileType = NTV2_BITFILE_CORVID22_MAIN;				break;
		case DEVICE_ID_CORVID24:					bitFileInfo.bitFileType = NTV2_BITFILE_CORVID24_MAIN;				break;
		case DEVICE_ID_CORVID3G:					bitFileInfo.bitFileType = NTV2_BITFILE_CORVID3G_MAIN;				break;
		case DEVICE_ID_CORVID44:					bitFileInfo.bitFileType = NTV2_BITFILE_CORVID44;					break;
		case DEVICE_ID_CORVID44_2X4K:               bitFileInfo.bitFileType = NTV2_BITFILE_CORVID44_2X4K_MAIN;			break;
		case DEVICE_ID_CORVID44_8K:                 bitFileInfo.bitFileType = NTV2_BITFILE_CORVID44_8K_MAIN;			break;
		case DEVICE_ID_CORVID44_8KMK:               bitFileInfo.bitFileType = NTV2_BITFILE_CORVID44_8KMK_MAIN;			break;
		case DEVICE_ID_CORVID44_PLNR:               bitFileInfo.bitFileType = NTV2_BITFILE_CORVID44_PLNR_MAIN;			break;
		case DEVICE_ID_CORVID88:					bitFileInfo.bitFileType = NTV2_BITFILE_CORVID88;					break;
		case DEVICE_ID_CORVIDHBR:					bitFileInfo.bitFileType = NTV2_BITFILE_NUMBITFILETYPES;				break;
		case DEVICE_ID_CORVIDHEVC:					bitFileInfo.bitFileType = NTV2_BITFILE_CORVIDHEVC;					break;
		case DEVICE_ID_IO4K:						bitFileInfo.bitFileType = NTV2_BITFILE_IO4K_MAIN;					break;
		case DEVICE_ID_IO4KPLUS:					bitFileInfo.bitFileType = NTV2_BITFILE_IO4KPLUS_MAIN;				break;
		case DEVICE_ID_IO4KUFC:						bitFileInfo.bitFileType = NTV2_BITFILE_IO4KUFC_MAIN;				break;
		case DEVICE_ID_IOEXPRESS:					bitFileInfo.bitFileType = NTV2_BITFILE_IOEXPRESS_MAIN;				break;
		case DEVICE_ID_IOIP_2022:					bitFileInfo.bitFileType = NTV2_BITFILE_IOIP_2022;					break;
		case DEVICE_ID_IOIP_2110:					bitFileInfo.bitFileType = NTV2_BITFILE_IOIP_2110;					break;
		case DEVICE_ID_IOIP_2110_RGB12:				bitFileInfo.bitFileType = NTV2_BITFILE_IOIP_2110_RGB12;				break;
		case DEVICE_ID_IOXT:						bitFileInfo.bitFileType = NTV2_BITFILE_IOXT_MAIN;					break;
		case DEVICE_ID_KONA1:						bitFileInfo.bitFileType = NTV2_BITFILE_KONA1;						break;
		case DEVICE_ID_KONA3G:						bitFileInfo.bitFileType = NTV2_BITFILE_KONA3G_MAIN;					break;
		case DEVICE_ID_KONA3GQUAD:					bitFileInfo.bitFileType = NTV2_BITFILE_KONA3G_QUAD;					break;
		case DEVICE_ID_KONA4:						bitFileInfo.bitFileType = NTV2_BITFILE_KONA4_MAIN;					break;
		case DEVICE_ID_KONA4UFC:					bitFileInfo.bitFileType = NTV2_BITFILE_KONA4UFC_MAIN;				break;
		case DEVICE_ID_KONA5:						bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_MAIN;					break;
		case DEVICE_ID_KONA5_2X4K:					bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_2X4K_MAIN;				break;
		case DEVICE_ID_KONA5_3DLUT:					bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_3DLUT_MAIN;			break;
		case DEVICE_ID_KONA5_8K:                    bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_8K_MAIN;				break;
		case DEVICE_ID_KONA5_8KMK:                  bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_8KMK_MAIN;				break;
		case DEVICE_ID_KONA5_OE1:		            bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_OE1_MAIN;				break;
		case DEVICE_ID_KONA5_OE2:		            bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_OE2_MAIN;				break;
		case DEVICE_ID_KONA5_OE3:		            bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_OE3_MAIN;				break;
		case DEVICE_ID_KONA5_OE4:		            bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_OE4_MAIN;				break;
		case DEVICE_ID_KONA5_OE5:		            bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_OE5_MAIN;				break;
		case DEVICE_ID_KONA5_OE6:		            bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_OE6_MAIN;				break;
		case DEVICE_ID_KONA5_OE7:		            bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_OE7_MAIN;				break;
		case DEVICE_ID_KONA5_OE8:		            bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_OE8_MAIN;				break;
		case DEVICE_ID_KONA5_OE9:		            bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_OE9_MAIN;				break;
		case DEVICE_ID_KONA5_OE10:		            bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_OE10_MAIN;				break;
		case DEVICE_ID_KONA5_OE11:		            bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_OE11_MAIN;				break;
		case DEVICE_ID_KONA5_OE12:		            bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_OE12_MAIN;				break;
		case DEVICE_ID_KONAHDMI:					bitFileInfo.bitFileType = NTV2_BITFILE_KONAHDMI;					break;
		case DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K:		bitFileInfo.bitFileType = NTV2_BITFILE_KONAIP_1RX_1TX_1SFP_J2K;		break;
		case DEVICE_ID_KONAIP_1RX_1TX_2110:			bitFileInfo.bitFileType = NTV2_BITFILE_KONAIP_1RX_1TX_2110;			break;
		case DEVICE_ID_KONAIP_2022:                 bitFileInfo.bitFileType = NTV2_BITFILE_KONAIP_2022;                 break;
		case DEVICE_ID_KONAIP_2110:                 bitFileInfo.bitFileType = NTV2_BITFILE_KONAIP_2110;                 break;
		case DEVICE_ID_KONAIP_2110_RGB12:			bitFileInfo.bitFileType = NTV2_BITFILE_KONAIP_2110_RGB12;			break;
		case DEVICE_ID_KONAIP_2TX_1SFP_J2K:			bitFileInfo.bitFileType = NTV2_BITFILE_KONAIP_2TX_1SFP_J2K;			break;
		case DEVICE_ID_KONAIP_4CH_2SFP:				bitFileInfo.bitFileType = NTV2_BITFILE_KONAIP_4CH_2SFP;				break;
		case DEVICE_ID_KONALHEPLUS:					bitFileInfo.bitFileType = NTV2_BITFILE_KONALHE_PLUS;				break;
		case DEVICE_ID_KONALHI:						bitFileInfo.bitFileType = NTV2_BITFILE_LHI_MAIN;					break;
		case DEVICE_ID_KONALHIDVI:					bitFileInfo.bitFileType = NTV2_BITFILE_NUMBITFILETYPES;				break;
		case DEVICE_ID_TTAP:						bitFileInfo.bitFileType = NTV2_BITFILE_TTAP_MAIN;					break;
		case DEVICE_ID_TTAP_PRO:					bitFileInfo.bitFileType = NTV2_BITFILE_TTAP_PRO_MAIN;				break;
		case DEVICE_ID_IOX3:						bitFileInfo.bitFileType = NTV2_BITFILE_IOX3_MAIN;					break;
		case DEVICE_ID_NOTFOUND:					bitFileInfo.bitFileType = NTV2_BITFILE_TYPE_INVALID;				break;
	#if !defined (_DEBUG)
		default:					break;
	#endif
	}
	bitFileInfo.checksum = 0;
	bitFileInfo.structVersion = 0;
	bitFileInfo.structSize = sizeof(BITFILE_INFO_STRUCT);
	bitFileInfo.whichFPGA = eFPGAVideoProc;

	const string bitFileDesignNameString = string(bitFileInfo.designNameStr) + ".bit";
	::strncpy(bitFileInfo.designNameStr, bitFileDesignNameString.c_str(), sizeof(bitFileInfo.designNameStr)-1);
	return true;
}

bool CNTV2DriverInterface::GetPackageInformation (PACKAGE_INFO_STRUCT & packageInfo)
{
	if (!IsDeviceReady(false) || !IsIPDevice())
		return false;	// cannot read flash

	string packInfo;
	ULWord deviceID = ULWord(_boardID);
	ReadRegister (kRegBoardID, deviceID);

	if (CNTV2AxiSpiFlash::DeviceSupported(NTV2DeviceID(deviceID)))
	{
		CNTV2AxiSpiFlash spiFlash(_boardNumber, false);

		uint32_t offset = spiFlash.Offset(SPI_FLASH_SECTION_MCSINFO);
		vector<uint8_t> mcsInfoData;
		if (spiFlash.Read(offset, mcsInfoData, 256))
		{
			packInfo.assign(mcsInfoData.begin(), mcsInfoData.end());

			// remove any trailing nulls
			size_t found = packInfo.find('\0');
			if (found != string::npos)
			{
				packInfo.resize(found);
			}
		}
		else
			return false;
	}
	else
	{
		ULWord baseAddress = (16 * 1024 * 1024) - (3 * 256 * 1024);
		const ULWord dwordSizeCount = 256/4;

		WriteRegister(kRegXenaxFlashAddress, ULWord(1));   // bank 1
		WriteRegister(kRegXenaxFlashControlStatus, 0x17);
		bool busy = true;
		ULWord timeoutCount = 1000;
		do
		{
			ULWord regValue;
			ReadRegister(kRegXenaxFlashControlStatus, regValue);
			if (regValue & BIT(8))
			{
				busy = true;
				timeoutCount--;
			}
			else
				busy = false;
		} while (busy == true && timeoutCount > 0);
		if (timeoutCount == 0)
			return false;

		ULWord* bitFilePtr =  new ULWord[dwordSizeCount];
		for ( ULWord count = 0; count < dwordSizeCount; count++, baseAddress += 4 )
		{
			WriteRegister(kRegXenaxFlashAddress, baseAddress);
			WriteRegister(kRegXenaxFlashControlStatus, 0x0B);
			busy = true;
			timeoutCount = 1000;
			do
			{
				ULWord regValue;
				ReadRegister(kRegXenaxFlashControlStatus, regValue);
				if ( regValue & BIT(8))
				{
					busy = true;
					timeoutCount--;
				}
				else
					busy = false;
			} while(busy == true && timeoutCount > 0);
			if (timeoutCount == 0)
			{
				delete [] bitFilePtr;
				return false;
			}
			ReadRegister(kRegXenaxFlashDOUT, bitFilePtr[count]);
		}

		packInfo = reinterpret_cast<char*>(bitFilePtr);
		delete [] bitFilePtr;
	}

	istringstream iss(packInfo);
	vector<string> results;
	string token;
	while (getline(iss,token, ' '))
		results.push_back(token);

	if (results.size() < 8)
		return false;

	packageInfo.date = results[1];
	token = results[2];
	token.erase(remove(token.begin(), token.end(), '\n'), token.end());
	packageInfo.time = token;
	packageInfo.buildNumber   = results[4];
	packageInfo.packageNumber = results[7];
	return true;
}

// Common remote card DriverGetBuildInformation.  Subclasses have overloaded function
// that does platform-specific function on local cards.
bool CNTV2DriverInterface::DriverGetBuildInformation (BUILD_INFO_STRUCT & buildInfo)
{
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	NTV2_ASSERT (IsRemote());
	return !_pRPCAPI->NTV2DriverGetBuildInformationRemote(buildInfo);
#else
	(void) buildInfo;
	return false;
#endif
}

bool CNTV2DriverInterface::BitstreamWrite (const NTV2_POINTER & inBuffer, const bool inFragment, const bool inSwap)
{
	NTV2Bitstream bsMsg (inBuffer,
						 BITSTREAM_WRITE |
						 (inFragment? BITSTREAM_FRAGMENT : 0) |
						 (inSwap? BITSTREAM_SWAP : 0));
	return NTV2Message (reinterpret_cast<NTV2_HEADER*>(&bsMsg));
}

bool CNTV2DriverInterface::BitstreamReset (const bool inConfiguration, const bool inInterface)
{
	NTV2_POINTER inBuffer;
	NTV2Bitstream bsMsg (inBuffer,
						 (inConfiguration? BITSTREAM_RESET_CONFIG : 0) |
						 (inInterface? BITSTREAM_RESET_MODULE : 0));
	return NTV2Message (reinterpret_cast<NTV2_HEADER*>(&bsMsg));
}

bool CNTV2DriverInterface::BitstreamStatus (NTV2ULWordVector & outRegValues)
{
	outRegValues.reserve(BITSTREAM_MCAP_DATA);
	outRegValues.clear();

	NTV2_POINTER inBuffer;
	NTV2Bitstream bsMsg (inBuffer, BITSTREAM_READ_REGISTERS);
	if (!NTV2Message (reinterpret_cast<NTV2_HEADER*>(&bsMsg)))
		return false;

	for (UWord ndx(0);  ndx < BITSTREAM_MCAP_DATA;  ndx++)
		outRegValues.push_back(bsMsg.mRegisters[ndx]);

	return true;
}


// FinishOpen
// NOTE _boardID must be set before calling this routine.
void CNTV2DriverInterface::FinishOpen (void)
{
	// HACK! FinishOpen needs frame geometry to determine frame buffer size and number.
	NTV2FrameGeometry fg;
	ULWord val1(0), val2(0);
	ReadRegister (kRegGlobalControl, fg, kRegMaskGeometry, kRegShiftGeometry);	//	Read FrameGeometry
	ReadRegister (kRegCh1Control, val1, kRegMaskFrameFormat, kRegShiftFrameFormat);	//	Read PixelFormat
	ReadRegister (kRegCh1Control, val2, kRegMaskFrameFormatHiBit, kRegShiftFrameFormatHiBit);
	NTV2PixelFormat pf(NTV2PixelFormat((val1 & 0x0F) | ((val2 & 0x1) << 4)));
	_ulFrameBufferSize = ::NTV2DeviceGetFrameBufferSize(_boardID, fg, pf);
	_ulNumFrameBuffers = ::NTV2DeviceGetNumberFrameBuffers(_boardID, fg, pf);

	ULWord returnVal1 = false;
	ULWord returnVal2 = false;
	if (::NTV2DeviceCanDo4KVideo(_boardID))
		ReadRegister(kRegGlobalControl2, returnVal1, kRegMaskQuadMode, kRegShiftQuadMode);
	if (::NTV2DeviceCanDo425Mux(_boardID))
		ReadRegister(kRegGlobalControl2, returnVal2, kRegMask425FB12, kRegShift425FB12);

#if !defined(NTV2_DEPRECATE_16_0)
    _pFrameBaseAddress = AJA_NULL;
    _pRegisterBaseAddress = AJA_NULL;
	_pRegisterBaseAddressLength = 0;
	_pXena2FlashBaseAddress  = AJA_NULL;
	_pCh1FrameBaseAddress = AJA_NULL;
	_pCh2FrameBaseAddress = AJA_NULL;
#endif	//	!defined(NTV2_DEPRECATE_16_0)

}	//	FinishOpen


bool CNTV2DriverInterface::ParseFlashHeader (BITFILE_INFO_STRUCT & bitFileInfo)
{
	if (!IsDeviceReady(false))
		return false;	// cannot read flash

	if (::NTV2DeviceHasSPIv4(_boardID))
	{
		uint32_t val;
		ReadRegister((0x100000 + 0x08) / 4, val);
		if (val != 0x01)
			return false;	// cannot read flash
	}

    if (::NTV2DeviceHasSPIv3(_boardID) || ::NTV2DeviceHasSPIv4(_boardID) || ::NTV2DeviceHasSPIv5(_boardID))
	{
		WriteRegister(kRegXenaxFlashAddress, 0ULL);
		WriteRegister(kRegXenaxFlashControlStatus, 0x17);
		bool busy = true;
		ULWord timeoutCount = 1000;
		do
		{
			ULWord regValue;
			ReadRegister(kRegXenaxFlashControlStatus, regValue);
			if (regValue & BIT(8))
			{
				busy = true;
				timeoutCount--;
			}
			else
				busy = false;
		} while (busy  &&  timeoutCount);
		if (!timeoutCount)
			return false;
	}

	//	Allocate header buffer, read/fill from SPI-flash...
	static const ULWord dwordCount(256/4);
	NTV2_POINTER bitFileHdrBuffer(dwordCount * sizeof(ULWord));
	if (!bitFileHdrBuffer)
		return false;

	ULWord* pULWord(bitFileHdrBuffer),  baseAddress(0);
	for (ULWord count(0);  count < dwordCount;  count++, baseAddress += 4)
		if (!ReadFlashULWord(baseAddress, pULWord[count]))
			return false;

	CNTV2Bitfile fileInfo;
	std::string headerError;
#if 0	//	Fake out:
	if (_boardID == DEVICE_ID_TTAP_PRO)	//	Fake TTapPro -- load "flash" from on-disk bitfile:
	{	fileInfo.Open("/Users/demo/dev-svn/firmware/T3_Tap/t_tap_pro.bit");
		headerError = fileInfo.GetLastError();
	} else
#endif
	headerError = fileInfo.ParseHeaderFromBuffer(bitFileHdrBuffer);
	if (headerError.empty())
	{
		::strncpy(bitFileInfo.dateStr, fileInfo.GetDate().c_str(), NTV2_BITFILE_DATETIME_STRINGLENGTH);
		::strncpy(bitFileInfo.timeStr, fileInfo.GetTime().c_str(), NTV2_BITFILE_DATETIME_STRINGLENGTH);
		::strncpy(bitFileInfo.designNameStr, fileInfo.GetDesignName().c_str(), NTV2_BITFILE_DESIGNNAME_STRINGLENGTH);
		::strncpy(bitFileInfo.partNameStr, fileInfo.GetPartName().c_str(), NTV2_BITFILE_PARTNAME_STRINGLENGTH);
		bitFileInfo.numBytes = ULWord(fileInfo.GetProgramStreamLength());
	}
	return headerError.empty();
}	//	ParseFlashHeader

bool CNTV2DriverInterface::ReadFlashULWord (const ULWord inAddress, ULWord & outValue, const ULWord inRetryCount)
{
	if (!WriteRegister(kRegXenaxFlashAddress, inAddress))
		return false;
	if (!WriteRegister(kRegXenaxFlashControlStatus, 0x0B))
		return false;
	bool busy(true);
	ULWord timeoutCount(inRetryCount);
	do
	{
		ULWord regValue(0);
		ReadRegister(kRegXenaxFlashControlStatus, regValue);
		if (regValue & BIT(8))
		{
			busy = true;
			timeoutCount--;
		}
		else
			busy = false;
	} while (busy  &&  timeoutCount);
	if (!timeoutCount)
		return false;
	return ReadRegister(kRegXenaxFlashDOUT, outValue);
}


//--------------------------------------------------------------------------------------------------------------------
//	Application acquire and release stuff
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2DriverInterface::AcquireStreamForApplicationWithReference (const ULWord inAppCode, const int32_t inProcessID)
{
	ULWord currentCode(0), currentPID(0);
	if (!ReadRegister(kVRegApplicationCode, currentCode) || !ReadRegister(kVRegApplicationPID, currentPID))
		return false;

	// Check if owner is deceased
	if (!AJAProcess::IsValid(currentPID))
	{
		// Process doesn't exist, so make the board our own
		ReleaseStreamForApplication (currentCode, int32_t(currentPID));
	}

	if (!ReadRegister(kVRegApplicationCode, currentCode) || !ReadRegister(kVRegApplicationPID, currentPID))
		return false;

	for (int count(0);  count < 20;  count++)
	{
		if (!currentPID)
		{
			// Nothing has the board
			if (!WriteRegister(kVRegApplicationCode, inAppCode))
				return false;
			// Just in case this is not zero
			WriteRegister(kVRegAcquireLinuxReferenceCount, 0);
			WriteRegister(kVRegAcquireLinuxReferenceCount, 1);
			return WriteRegister(kVRegApplicationPID, ULWord(inProcessID));
		}
		else if (currentCode == inAppCode  &&  currentPID == ULWord(inProcessID))
			return WriteRegister(kVRegAcquireLinuxReferenceCount, 1);	// Process already acquired, so bump the count
		// Someone else has the board, so wait and try again
		AJATime::Sleep(50);
	}
	return false;
}

bool CNTV2DriverInterface::ReleaseStreamForApplicationWithReference (const ULWord inAppCode, const int32_t inProcessID)
{
	ULWord currentCode(0), currentPID(0), currentCount(0);
	if (!ReadRegister(kVRegApplicationCode, currentCode)
		|| !ReadRegister(kVRegApplicationPID, currentPID)
		|| !ReadRegister(kVRegAcquireLinuxReferenceCount, currentCount))
			return false;

	if (currentCode == inAppCode  &&  currentPID == ULWord(inProcessID))
	{
		if (currentCount > 1)
			return WriteRegister(kVRegReleaseLinuxReferenceCount, 1);
		if (currentCount == 1)
			return ReleaseStreamForApplication(inAppCode, inProcessID);
		return true;
	}
	return false;
}

bool CNTV2DriverInterface::AcquireStreamForApplication (const ULWord inAppCode, const int32_t inProcessID)
{
	// Loop for a while trying to acquire the board
	for (int count(0);  count < 20;  count++)
	{
		if (WriteRegister(kVRegApplicationCode, inAppCode))
			return WriteRegister(kVRegApplicationPID, ULWord(inProcessID));
		AJATime::Sleep(50);
	}

	// Get data about current owner
	ULWord currentCode(0), currentPID(0);
	if (!ReadRegister(kVRegApplicationCode, currentCode) || !ReadRegister(kVRegApplicationPID, currentPID))
		return false;

	//	Check if owner is deceased
	if (!AJAProcess::IsValid(currentPID))
	{	// Process doesn't exist, so make the board our own
		ReleaseStreamForApplication (currentCode, int32_t(currentPID));
		for (int count(0);  count < 20;  count++)
		{
			if (WriteRegister(kVRegApplicationCode, inAppCode))
				return WriteRegister(kVRegApplicationPID, ULWord(inProcessID));
			AJATime::Sleep(50);
		}
	}
	// Current owner is alive, so don't interfere
	return false;
}

bool CNTV2DriverInterface::ReleaseStreamForApplication (const ULWord inAppCode, const int32_t inProcessID)
{	(void)inAppCode;	//	Don't care which appCode
	if (WriteRegister(kVRegReleaseApplication, ULWord(inProcessID)))
	{
		WriteRegister(kVRegAcquireLinuxReferenceCount, 0);
		return true;	// We don't care if the above call failed
	}
	return false;
}

bool CNTV2DriverInterface::SetStreamingApplication (const ULWord inAppCode, const int32_t inProcessID)
{
	if (!WriteRegister(kVRegForceApplicationCode, inAppCode))
		return false;
	return WriteRegister(kVRegForceApplicationPID, ULWord(inProcessID));
}

bool CNTV2DriverInterface::GetStreamingApplication (ULWord & outAppType, int32_t & outProcessID)
{
	if (!ReadRegister(kVRegApplicationCode, outAppType))
		return false;
	return CNTV2DriverInterface::ReadRegister(kVRegApplicationPID, outProcessID);
}

//	This function is used by the retail ControlPanel.
//	Read the current RP188 registers (which typically give you the timecode corresponding to the LAST frame).
//	NOTE:	This is a hack to avoid making a "real" driver call! Since the RP188 data requires three ReadRegister()
//			calls, there is a chance that it can straddle a VBI, which could give bad results. To avoid this, we
//			read the 3 registers until we get two consecutive passes that give us the same data. (Someday it'd
//			be nice if the driver automatically read these as part of its VBI IRQ handler...
bool CNTV2DriverInterface::ReadRP188Registers (const NTV2Channel inChannel, RP188_STRUCT * pRP188Data)
{	(void) inChannel;
	if (!pRP188Data)
		return false;

	RP188_STRUCT rp188;
	NTV2DeviceID boardID = DEVICE_ID_NOTFOUND;
	RP188SourceFilterSelect source = kRP188SourceEmbeddedLTC;
	ULWord dbbReg(0), msReg(0), lsReg(0);

	CNTV2DriverInterface::ReadRegister(kRegBoardID, boardID);
	CNTV2DriverInterface::ReadRegister(kVRegRP188SourceSelect, source);
	bool bLTCPort = (source == kRP188SourceLTCPort);

	// values come from LTC port registers
	if (bLTCPort)
	{
		ULWord ltcPresent;
		ReadRegister (kRegStatus, ltcPresent, kRegMaskLTCInPresent, kRegShiftLTCInPresent);

		// there is no equivalent DBB for LTC port - we synthesize it here
		rp188.DBB = (ltcPresent) ? 0xFE000000 | NEW_SELECT_RP188_RCVD : 0xFE000000;

		// LTC port registers
		dbbReg = 0; // don't care - does not exist
		msReg = kRegLTCAnalogBits0_31;
		lsReg  = kRegLTCAnalogBits32_63;
	}
	else
	{
		// values come from RP188 registers
		NTV2Channel channel = NTV2_CHANNEL1;
		NTV2InputVideoSelect inputSelect = NTV2_Input1Select;

		if (::NTV2DeviceGetNumVideoInputs(boardID) > 1)
		{
			CNTV2DriverInterface::ReadRegister (kVRegInputSelect, inputSelect);
			channel = (inputSelect == NTV2_Input2Select) ? NTV2_CHANNEL2 : NTV2_CHANNEL1;
		}
		else
			channel = NTV2_CHANNEL1;

		// rp188 registers
		dbbReg = (channel == NTV2_CHANNEL1 ? kRegRP188InOut1DBB : kRegRP188InOut2DBB);
		//Check to see if TC is received
		uint32_t tcReceived = 0;
		ReadRegister(dbbReg, tcReceived, BIT(16), 16);
		if(tcReceived == 0)
			return false;//No TC recevied

		ReadRegister (dbbReg, rp188.DBB, kRegMaskRP188DBB, kRegShiftRP188DBB );
		switch (rp188.DBB)//What do we have?
		{
			default:
			case 0x01:
			case 0x02:
			{
				//We have VITC - what do we want?
				if (pRP188Data->DBB == 0x01 || pRP188Data->DBB == 0x02)
				{	//	We want VITC
					msReg  = (channel == NTV2_CHANNEL1 ? kRegRP188InOut1Bits0_31  : kRegRP188InOut2Bits0_31 );
					lsReg  = (channel == NTV2_CHANNEL1 ? kRegRP188InOut1Bits32_63 : kRegRP188InOut2Bits32_63);
				}
				else
				{	//	We want Embedded LTC, so we should check one other place
					uint32_t ltcPresent = 0;
					ReadRegister(dbbReg, ltcPresent, BIT(18), 18);
					if (ltcPresent != 1)
						return false;
					//Read LTC registers
					msReg  = (channel == NTV2_CHANNEL1 ? kRegLTCEmbeddedBits0_31  : kRegLTC2EmbeddedBits0_31 );
					lsReg  = (channel == NTV2_CHANNEL1 ? kRegLTCEmbeddedBits32_63 : kRegLTC2EmbeddedBits32_63);
				}
				break;
			}
			case 0x00:
				//We have LTC - do we want it?
				if (pRP188Data->DBB != 0x00)
					return false;
				msReg  = (channel == NTV2_CHANNEL1 ? kRegRP188InOut1Bits0_31  : kRegRP188InOut2Bits0_31 );
				lsReg  = (channel == NTV2_CHANNEL1 ? kRegRP188InOut1Bits32_63 : kRegRP188InOut2Bits32_63);
				break;
		}
		//Re-Read the whole register just in case something is expecting other status values
		ReadRegister (dbbReg, rp188.DBB);
	}
	ReadRegister (msReg,  rp188.Low );
	ReadRegister (lsReg,  rp188.High);

	// register stability filter
	do
	{
		*pRP188Data = rp188;	// struct copy to result

		// read again into local struct
		if (!bLTCPort)
			ReadRegister (dbbReg, rp188.DBB);
		ReadRegister (msReg,  rp188.Low );
		ReadRegister (lsReg,  rp188.High);

		// if the new read equals the previous read, consider it done
		if (rp188.DBB  == pRP188Data->DBB  &&
			rp188.Low  == pRP188Data->Low  &&
			rp188.High == pRP188Data->High)
				break;
	} while (true);

	return true;
}

void CNTV2DriverInterface::BumpEventCount (const INTERRUPT_ENUMS eInterruptType)
{
	if (NTV2_IS_VALID_INTERRUPT_ENUM(eInterruptType))
		mEventCounts[eInterruptType] += 1;

}	//	BumpEventCount


string CNTV2DriverInterface::GetHostName (void) const
{
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	if (_pRPCAPI)
		return _pRPCAPI->Name();
#endif	//	defined (NTV2_NUB_CLIENT_SUPPORT)
	return "";
}

bool CNTV2DriverInterface::IsRemote (void) const
{
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	if (_pRPCAPI)
		return _pRPCAPI->IsConnected();
#endif	//	defined (NTV2_NUB_CLIENT_SUPPORT)
	return false;
}

bool CNTV2DriverInterface::IsDeviceReady (const bool checkValid)
{
	if (!IsIPDevice())
		return true;	//	Non-IP devices always ready

	if (!IsMBSystemReady())
		return false;

	if (checkValid && !IsMBSystemValid())
		return false;

	return true;	//	Ready!
}

bool CNTV2DriverInterface::IsMBSystemValid (void)
{
	if (IsIPDevice())
	{
        uint32_t val;
        ReadRegister(SAREK_REGS + kRegSarekIfVersion, val);
        return val == SAREK_IF_VERSION;
	}
	return true;
}

bool CNTV2DriverInterface::IsMBSystemReady (void)
{
	if (!IsIPDevice())
		return false;	//	No microblaze

	uint32_t val;
	ReadRegister(SAREK_REGS + kRegSarekMBState, val);
	if (val != 0x01)
		return false;	//	MB not ready

	// Not enough to read MB State, we need to make sure MB is running
	ReadRegister(SAREK_REGS + kRegSarekMBUptime, val);
	return (val < 2) ? false : true;
}

#if defined(NTV2_WRITEREG_PROFILING)	//	Register Write Profiling
	bool CNTV2DriverInterface::GetRecordedRegisterWrites (NTV2RegisterWrites & outRegWrites) const
	{
		AJAAutoLock	autoLock(&mRegWritesLock);
		outRegWrites = mRegWrites;
		return true;
	}

	bool CNTV2DriverInterface::StartRecordRegisterWrites (const bool inSkipActualWrites)
	{
		AJAAutoLock	autoLock(&mRegWritesLock);
		if (mRecordRegWrites)
			return false;	//	Already recording
		mRegWrites.clear();
		mRecordRegWrites = true;
		mSkipRegWrites = inSkipActualWrites;
		return true;
	}

	bool CNTV2DriverInterface::ResumeRecordRegisterWrites (void)
	{	//	Identical to Start, but don't clear mRegWrites nor change mSkipRegWrites
		AJAAutoLock	autoLock(&mRegWritesLock);
		if (mRecordRegWrites)
			return false;	//	Already recording
		mRecordRegWrites = true;
		return true;
	}

	bool CNTV2DriverInterface::IsRecordingRegisterWrites (void) const
	{	//	NB: This will return false if paused
		AJAAutoLock	autoLock(&mRegWritesLock);
		return mRecordRegWrites;
	}

	bool CNTV2DriverInterface::StopRecordRegisterWrites (void)
	{
		AJAAutoLock	autoLock(&mRegWritesLock);
		mRecordRegWrites = mSkipRegWrites = false;
		return true;
	}

	bool CNTV2DriverInterface::PauseRecordRegisterWrites (void)
	{	//	Identical to Stop, but don't change mSkipRegWrites
		AJAAutoLock	autoLock(&mRegWritesLock);
		if (!mRecordRegWrites)
			return false;	//	Already stopped/paused
		mRecordRegWrites = false;
		return true;
	}

	ULWord CNTV2DriverInterface::GetNumRecordedRegisterWrites (void) const
	{
		AJAAutoLock	autoLock(&mRegWritesLock);
		return ULWord(mRegWrites.size());
	}
#endif	//	NTV2_WRITEREG_PROFILING


#if !defined (NTV2_DEPRECATE)
NTV2BoardType CNTV2DriverInterface::GetCompileFlag ()
{
	NTV2BoardType eBoardType = BOARDTYPE_UNKNOWN;

#ifdef HDNTV
	eBoardType = BOARDTYPE_HDNTV;
#elif defined KSD
	eBoardType = BOARDTYPE_KSD;
#elif defined KHD
	eBoardType = BOARDTYPE_KHD;
#elif defined XENA2
	eBoardType = BOARDTYPE_AJAXENA2;
#elif defined BORG
	eBoardType = BOARDTYPE_BORG;
#endif

	return eBoardType;
}
#endif	//	!NTV2_DEPRECATE


#if defined (AJADLL_BUILD) || defined (AJASTATIC)
	//	This code forces link/load errors if the SDK client was built with NTV2_DEPRECATE defined,
	//	but the SDK lib/dylib/DLL was built without NTV2_DEPRECATE defined, ... or vice-versa...
	#if defined (NTV2_DEPRECATE)
		AJAExport	int	gNTV2_DEPRECATE	(void);
		AJAExport	int	gNTV2_DEPRECATE	(void){return 0;}
	#else
		AJAExport	int	gNTV2_NON_DEPRECATE	(void);
		AJAExport	int	gNTV2_NON_DEPRECATE	(void){return 0;}
	#endif
#endif	//	AJADLL_BUILD or AJASTATIC
