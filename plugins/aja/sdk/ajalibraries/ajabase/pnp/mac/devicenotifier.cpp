/* SPDX-License-Identifier: MIT */
/**
	@file		devicenotifier.cpp
	@brief		Implements the MacOS-specific KonaNotifier and DeviceNotifier classes, which invoke
				a client-registered callback function when devices are attached and/or detached.
	@copyright	(C) 2011-2021 AJA Video Systems, Inc.
**/

#include <syslog.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <IOKit/IOCFPlugIn.h>
#include "ajatypes.h"
#include "devicenotifier.h"
#include "ajabase/common/common.h"


using namespace std;

static const char * GetKernErrStr (const kern_return_t inError);


//	DeviceNotifier-specific Logging Macros

#define	HEX2(__x__)				"0x" << hex << setw(2)  << setfill('0') << (0x00FF     & uint16_t(__x__)) << dec
#define	HEX4(__x__)				"0x" << hex << setw(4)  << setfill('0') << (0xFFFF     & uint16_t(__x__)) << dec
#define	HEX8(__x__)				"0x" << hex << setw(8)  << setfill('0') << (0xFFFFFFFF & uint32_t(__x__)) << dec
#define	HEX16(__x__)			"0x" << hex << setw(16) << setfill('0') <<               uint64_t(__x__)  << dec
#define KR(_kr_)				"kernErr=" << HEX8(_kr_) << "(" << ::GetKernErrStr(_kr_) << ")"
#define INST(__p__)				"Ins-" << hex << setw(16) << setfill('0') << uint64_t(__p__) << dec
#define THRD(__t__)				"Thr-" << hex << setw(16) << setfill('0') << uint64_t(__t__) << dec

#define	DNDB(__lvl__, __x__)	AJA_sREPORT(AJA_DebugUnit_PnP, (__lvl__),	INST(this) << ": " << AJAFUNC << ": " << __x__)
#define	DNFAIL(__x__)			DNDB(AJA_DebugSeverity_Error,	__x__)
#define	DNWARN(__x__)			DNDB(AJA_DebugSeverity_Warning,	__x__)
#define	DNNOTE(__x__)			DNDB(AJA_DebugSeverity_Notice,	__x__)
#define	DNINFO(__x__)			DNDB(AJA_DebugSeverity_Info,	__x__)
#define	DNDBG(__x__)			DNDB(AJA_DebugSeverity_Debug,	__x__)


// MARK: DeviceNotifier

//-------------------------------------------------------------------------------------------------------------
//	DeviceNotifier
//-------------------------------------------------------------------------------------------------------------
DeviceNotifier::DeviceNotifier (DeviceClientCallback callback, void *refcon)
	:	m_refcon				(refcon),
		m_clientCallback		(callback),
		m_masterPort			(0),
		m_notificationPort		(NULL),
		m_matchingDictionary	(NULL)
{
}


//-------------------------------------------------------------------------------------------------------------
//	~DeviceNotifier
//-------------------------------------------------------------------------------------------------------------
DeviceNotifier::~DeviceNotifier ()
{
	// disable callbacks
	Uninstall ();
	
	m_masterPort = 0;
	m_clientCallback = NULL;
	m_refcon = NULL;
}


//-------------------------------------------------------------------------------------------------------------
//	SetCallback
//-------------------------------------------------------------------------------------------------------------
void DeviceNotifier::SetCallback (DeviceClientCallback callback, void *refcon)
{
	m_clientCallback = callback;
	m_refcon = refcon;
}


//-------------------------------------------------------------------------------------------------------------
//	Install
//-------------------------------------------------------------------------------------------------------------

bool DeviceNotifier::Install (CFMutableDictionaryRef matchingDictionary)
{
	DNDBG("On entry: deviceInterestList.size=" << m_deviceInterestList.size() << ", deviceMatchList.size=" << m_deviceMatchList.size());

	m_matchingDictionary = matchingDictionary;
	
	// if no dictionary given use default
	if (m_matchingDictionary == NULL)
		m_matchingDictionary = CreateMatchingDictionary ();
	else
		::CFRetain (m_matchingDictionary);		// if dictionary was passed in retain it

	// check for NULL dictionary
	if (m_matchingDictionary == NULL)
	{
		DNFAIL("NULL matchingDictionary");
		return false;
	}
	
	// Retrieve the IOKit's master port so a notification port can be created
	mach_port_t masterPort;
	IOReturn ioReturn = ::IOMasterPort (MACH_PORT_NULL, &masterPort);
	if (kIOReturnSuccess != ioReturn)
	{
		DNFAIL(KR(ioReturn) << " -- IOMasterPort failed");
		return false;
	}

	m_masterPort = masterPort;
	m_notificationPort = ::IONotificationPortCreate (masterPort);
	if (0 == m_notificationPort)
	{
		DNFAIL("IONotificationPortCreate failed");
		return false;
	}
	
	// Get the CFRunLoopSource for the notification port and add it to the default run loop.
	// It is not necessary to call CFRunLoopRemoveSource() duringing tear down because that is implicitly done when
	// the the notification port is destroyed.
	CFRunLoopSourceRef runLoopSource = ::IONotificationPortGetRunLoopSource (m_notificationPort);
	CFRunLoopAddSource (CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);

	// IOServiceAddMatchingNotification 'eats' a matching dictionary, so up the retention count
	CFRetain(m_matchingDictionary);

	// lets create a callback
	io_iterator_t iterator;
	ioReturn = IOServiceAddMatchingNotification(m_notificationPort, 
												kIOMatchedNotification, 
												m_matchingDictionary,
												reinterpret_cast<IOServiceMatchingCallback>(DeviceAddedCallback), 
												this, 
												&iterator);
	if (kIOReturnSuccess != ioReturn)
	{
		DNFAIL(KR(ioReturn) << " -- IOServiceAddMatchingNotification failed");
		return false;
	}
	
	DeviceAddedCallback (this, iterator);
	m_deviceMatchList.push_back(iterator);

	DNINFO("On exit: callback installed, deviceInterestList.size=" << m_deviceInterestList.size() << ", deviceMatchList.size=" << m_deviceMatchList.size());
	return (m_deviceInterestList.size() > 0);
}


//--------------------------------------------------------------------------------------------------------------------
//	Uninstall
//--------------------------------------------------------------------------------------------------------------------
void DeviceNotifier::Uninstall ()
{
	DNDBG("On entry: m_deviceInterestList.size()=" << m_deviceInterestList.size()
			<< ", m_deviceMatchList.size()=" << m_deviceMatchList.size());
	//	Release device-matching list...
	list<io_object_t>::iterator p;
    for (p = m_deviceMatchList.begin(); p != m_deviceMatchList.end(); ++p)
		IOObjectRelease (*p);
	m_deviceMatchList.clear();

	//	Release device-interest list...
    for (p = m_deviceInterestList.begin(); p != m_deviceInterestList.end(); ++p)
		IOObjectRelease (*p);
	m_deviceInterestList.clear();

	if (m_notificationPort)
	{
		CFRunLoopSourceRef runLoopSource = IONotificationPortGetRunLoopSource (m_notificationPort);
		CFRunLoopRemoveSource (CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);
		IONotificationPortDestroy (m_notificationPort);
		m_notificationPort = 0;
	}

	if (m_matchingDictionary)
	{
		CFRelease (m_matchingDictionary);
		m_matchingDictionary = NULL;
	}
	DNINFO("On exit: callback removed, m_deviceInterestList.size()=" << m_deviceInterestList.size() << ", m_deviceMatchList.size()=" << m_deviceMatchList.size());
}


//--------------------------------------------------------------------------------------------------------------------
//	CreateMatchingDictionary
//	This high level callbacks only when a specific driver is loaded, goes offline
//--------------------------------------------------------------------------------------------------------------------
CFMutableDictionaryRef DeviceNotifier::CreateMatchingDictionary (CFStringRef deviceDriverName)
{
	// This high level callbacks only when driver is loaded, goes offline
	CFMutableDictionaryRef matchingDictionary = CFDictionaryCreateMutable (	kCFAllocatorDefault,
																			0,
																			&kCFTypeDictionaryKeyCallBacks,
																			&kCFTypeDictionaryValueCallBacks);
	// Specify class type
	CFDictionaryAddValue (matchingDictionary, CFSTR("IOProviderClass"), deviceDriverName);
	return matchingDictionary;
}


// MARK: Callbacks


//--------------------------------------------------------------------------------------------------------------------
//	DeviceAddedCallback
//	matching AJA IOService found
//--------------------------------------------------------------------------------------------------------------------
void DeviceNotifier::DeviceAddedCallback (DeviceNotifier* thisObject, io_iterator_t iterator)
{
	thisObject->DeviceAdded (iterator);
}


//--------------------------------------------------------------------------------------------------------------------
//	DeviceAdded
//--------------------------------------------------------------------------------------------------------------------
void DeviceNotifier::DeviceAdded (io_iterator_t iterator)
{
	io_object_t	service;
	bool deviceFound = false;
	
	//	This iteration is essential to keep this callback working...
	IOIteratorReset(iterator);
	for ( ;(service = IOIteratorNext(iterator)); IOObjectRelease(service))
	{
		AddGeneralInterest(service);		// optional
		deviceFound = true;
	}
	
	// now notify our callback
	DNINFO("Device added, calling DeviceClientCallback " << INST(m_clientCallback));
	if (deviceFound && m_clientCallback)
		(*(m_clientCallback))(kAJADeviceInitialOpen, m_refcon);
}


//--------------------------------------------------------------------------------------------------------------------
//	DeviceRemovedCallback
//	matching AJA IOService has been removed
//--------------------------------------------------------------------------------------------------------------------
void DeviceNotifier::DeviceRemovedCallback (DeviceNotifier* thisObject, io_iterator_t iterator)
{
	thisObject->DeviceRemoved (iterator);
}


//--------------------------------------------------------------------------------------------------------------------
//	DeviceRemoved
//--------------------------------------------------------------------------------------------------------------------
void DeviceNotifier::DeviceRemoved (io_iterator_t iterator)
{
	io_object_t	service;
	bool deviceFound = false;
	
	//	This iteration is essential to keep this callback working...
	IOIteratorReset(iterator);
	for ( ;(service = IOIteratorNext(iterator)); IOObjectRelease(service))
		deviceFound = true;

	// now notify our callback
	DNINFO("Device removed, calling DeviceClientCallback " << INST(m_clientCallback));
	if (deviceFound && m_clientCallback)
		(*(m_clientCallback))(kAJADeviceTerminate, m_refcon);
}


//--------------------------------------------------------------------------------------------------------------------
//	AddGeneralInterest
//	add general interest callback, return true if iterator has one or more items
//--------------------------------------------------------------------------------------------------------------------
void DeviceNotifier::AddGeneralInterest (io_object_t service)
{
	io_object_t notifier;

	// Create a notifier object so 'general interest' notifications can be received for the service.
	// In Kona this is used for debugging only
	IOReturn ioReturn = ::IOServiceAddInterestNotification (m_notificationPort,
															service,
															kIOGeneralInterest,
															reinterpret_cast <IOServiceInterestCallback> (DeviceChangedCallback),
															this,
															&notifier);
	if (kIOReturnSuccess != ioReturn)
	{
		DNFAIL(KR(ioReturn) << " -- IOServiceAddInterestNotification failed");
		return;
	}
	m_deviceInterestList.push_back (notifier);
}


//--------------------------------------------------------------------------------------------------------------------
//  DeviceChangedCallback()
//	notifier messages sent by the driver
//--------------------------------------------------------------------------------------------------------------------
void DeviceNotifier::DeviceChangedCallback (DeviceNotifier* thisObject, io_service_t unitService, natural_t messageType, void* message)
{
	thisObject->DeviceChanged (unitService, messageType, message);
}


//--------------------------------------------------------------------------------------------------------------------
//  DeviceChanged()
//--------------------------------------------------------------------------------------------------------------------
void DeviceNotifier::DeviceChanged (io_service_t unitService, natural_t messageType, void* message)
{
    (void) unitService;
    (void) message;
	DNINFO(MessageTypeToStr(messageType) << ", calling DeviceClientCallback " << INST(m_clientCallback));
	if (m_clientCallback)
		(*(m_clientCallback))(messageType, m_refcon);	// notify client
}



//--------------------------------------------------------------------------------------------------------------------
//  MessageTypeToStr()
//	decode message type
//--------------------------------------------------------------------------------------------------------------------
string DeviceNotifier::MessageTypeToStr (const natural_t messageType)
{
	ostringstream	oss;
	switch (messageType)
	{
		case kIOMessageServiceIsTerminated:			oss << "kIOMessageServiceIsTerminated";			break;
		case kIOMessageServiceIsSuspended:			oss << "kIOMessageServiceIsSuspended";			break;
		case kIOMessageServiceIsResumed:			oss << "kIOMessageServiceIsResumed";			break;
		case kIOMessageServiceIsRequestingClose:	oss << "kIOMessageServiceIsRequestingClose";	break;
		// the more esoteric messages:
		case kIOMessageServiceIsAttemptingOpen:		oss << "kIOMessageServiceIsAttemptingOpen";		break;	//	When another process connects to our device
		case kIOMessageServiceWasClosed:			oss << "kIOMessageServiceWasClosed";			break;	//	When another process disconnects from our device
		case kIOMessageServiceBusyStateChange:		oss << "kIOMessageServiceBusyStateChange";		break;
		case kIOMessageCanDevicePowerOff:			oss << "kIOMessageCanDevicePowerOff";			break;
		case kIOMessageDeviceWillPowerOff:			oss << "kIOMessageDeviceWillPowerOff";			break;
		case kIOMessageDeviceWillNotPowerOff:		oss << "kIOMessageDeviceWillPowerOff";			break;
		case kIOMessageDeviceHasPoweredOn:			oss << "kIOMessageDeviceHasPoweredOn";			break;
		case kIOMessageCanSystemPowerOff:			oss << "kIOMessageCanSystemPowerOff";			break;
		case kIOMessageSystemWillPowerOff:			oss << "kIOMessageSystemWillPowerOff";			break;
		case kIOMessageSystemWillNotPowerOff:		oss << "kIOMessageSystemWillNotPowerOff";		break;
		case kIOMessageCanSystemSleep:				oss << "kIOMessageCanSystemSleep";				break;
		case kIOMessageSystemWillSleep:				oss << "kIOMessageSystemWillSleep";				break;
		case kIOMessageSystemWillNotSleep:			oss << "kIOMessageSystemWillNotSleep";			break;
		case kIOMessageSystemHasPoweredOn:			oss << "kIOMessageSystemHasPoweredOn";			break;
		default:									oss << "msgType=0x" << hex << setw(4) << setfill('0') << messageType;	break;
	}
	return oss.str();
}

// MARK: KonaNotifier


bool KonaNotifier::Install (CFMutableDictionaryRef matchingDictionary)
{
	(void) matchingDictionary;
	DNDBG("On entry: deviceInterestList.size=" << m_deviceInterestList.size() << ", deviceMatchList.size=" << m_deviceMatchList.size());

	// Retrieve the IOKit's master port so a notification port can be created
	mach_port_t masterPort;
	IOReturn ioReturn = ::IOMasterPort (MACH_PORT_NULL, &masterPort);
	if (kIOReturnSuccess != ioReturn)
	{
		DNFAIL(KR(ioReturn) << " -- IOMasterPort failed");
		return false;
	}

	m_masterPort = masterPort;
	m_notificationPort = ::IONotificationPortCreate (masterPort);
	if (0 == m_notificationPort)
	{
		DNFAIL("IONotificationPortCreate failed");
		return false;
	}
	
	//	Get the CFRunLoopSource for the notification port and add it to the default run loop.
	//	It is not necessary to call CFRunLoopRemoveSource() during tear down because that is
	//	implicitly done when the the notification port is destroyed.
	CFRunLoopSourceRef runLoopSource = ::IONotificationPortGetRunLoopSource (m_notificationPort);
	::CFRunLoopAddSource (::CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);
	

	// walk through each of our devices
	static const std::string driverName ("com_aja_iokit_ntv2");
	io_iterator_t notifyIterator_matched, notifyIterator_terminated;
		
	// Device Added
	ioReturn = ::IOServiceAddMatchingNotification (	m_notificationPort,
													kIOMatchedNotification,
													IOServiceMatching(driverName.c_str ()),
													reinterpret_cast<IOServiceMatchingCallback>(DeviceAddedCallback), 
													this, 
													&notifyIterator_matched);

	if (ioReturn != kIOReturnSuccess)
	{
		DNFAIL(KR(ioReturn) << " -- IOServiceAddMatchingNotification for 'kIOMatchedNotification' failed");
		return false;
	}

	// Device Terminated
	ioReturn = ::IOServiceAddMatchingNotification (	m_notificationPort,
													kIOTerminatedNotification,
													IOServiceMatching(driverName.c_str ()),
													reinterpret_cast<IOServiceMatchingCallback>(DeviceRemovedCallback), 
													this, 
													&notifyIterator_terminated);
	if (ioReturn != kIOReturnSuccess)
	{
		DNFAIL(KR(ioReturn) << " -- IOServiceAddMatchingNotification for 'kIOTerminatedNotification' failed");
		return false;
	}

	DeviceAddedCallback (this, notifyIterator_matched);
	m_deviceMatchList.push_back (notifyIterator_matched);

	DeviceRemovedCallback (this, notifyIterator_terminated);
	m_deviceMatchList.push_back (notifyIterator_terminated);

	DNINFO("On exit: callback installed, deviceInterestList.size=" << m_deviceInterestList.size() << ", deviceMatchList.size=" << m_deviceMatchList.size());
	return m_deviceInterestList.size() > 0;	//	**MrBill**	SHOULDN'T THIS RETURN m_deviceMatchList.size() > 0	????

}	//	KonaNotifier::Install


static const char * GetKernErrStr (const kern_return_t inError)
{
	switch (inError)
	{
		case kIOReturnError:			return "kIOReturnError";
		case kIOReturnNoMemory:			return "kIOReturnNoMemory";
		case kIOReturnNoResources:		return "kIOReturnNoResources";
		case kIOReturnIPCError:			return "kIOReturnIPCError";
		case kIOReturnNoDevice:			return "kIOReturnNoDevice";
		case kIOReturnNotPrivileged:	return "kIOReturnNotPrivileged";
		case kIOReturnBadArgument:		return "kIOReturnBadArgument";
		case kIOReturnLockedRead:		return "kIOReturnLockedRead";
		case kIOReturnLockedWrite:		return "kIOReturnLockedWrite";
		case kIOReturnExclusiveAccess:	return "kIOReturnExclusiveAccess";
		case kIOReturnBadMessageID:		return "kIOReturnBadMessageID";
		case kIOReturnUnsupported:		return "kIOReturnUnsupported";
		case kIOReturnVMError:			return "kIOReturnVMError";
		case kIOReturnInternalError:	return "kIOReturnInternalError";
		case kIOReturnIOError:			return "kIOReturnIOError";
		case kIOReturnCannotLock:		return "kIOReturnCannotLock";
		case kIOReturnNotOpen:			return "kIOReturnNotOpen";
		case kIOReturnNotReadable:		return "kIOReturnNotReadable";
		case kIOReturnNotWritable:		return "kIOReturnNotWritable";
		case kIOReturnNotAligned:		return "kIOReturnNotAligned";
		case kIOReturnBadMedia:			return "kIOReturnBadMedia";
		case kIOReturnStillOpen:		return "kIOReturnStillOpen";
		case kIOReturnRLDError:			return "kIOReturnRLDError";
		case kIOReturnDMAError:			return "kIOReturnDMAError";
		case kIOReturnBusy:				return "kIOReturnBusy";
		case kIOReturnTimeout:			return "kIOReturnTimeout";
		case kIOReturnOffline:			return "kIOReturnOffline";
		case kIOReturnNotReady:			return "kIOReturnNotReady";
		case kIOReturnNotAttached:		return "kIOReturnNotAttached";
		case kIOReturnNoChannels:		return "kIOReturnNoChannels";
		case kIOReturnNoSpace:			return "kIOReturnNoSpace";
		case kIOReturnPortExists:		return "kIOReturnPortExists";
		case kIOReturnCannotWire:		return "kIOReturnCannotWire";
		case kIOReturnNoInterrupt:		return "kIOReturnNoInterrupt";
		case kIOReturnNoFrames:			return "kIOReturnNoFrames";
		case kIOReturnMessageTooLarge:	return "kIOReturnMessageTooLarge";
		case kIOReturnNotPermitted:		return "kIOReturnNotPermitted";
		case kIOReturnNoPower:			return "kIOReturnNoPower";
		case kIOReturnNoMedia:			return "kIOReturnNoMedia";
		case kIOReturnUnformattedMedia:	return "kIOReturnUnformattedMedia";
		case kIOReturnUnsupportedMode:	return "kIOReturnUnsupportedMode";
		case kIOReturnUnderrun:			return "kIOReturnUnderrun";
		case kIOReturnOverrun:			return "kIOReturnOverrun";
		case kIOReturnDeviceError:		return "kIOReturnDeviceError";
		case kIOReturnNoCompletion:		return "kIOReturnNoCompletion";
		case kIOReturnAborted:			return "kIOReturnAborted";
		case kIOReturnNoBandwidth:		return "kIOReturnNoBandwidth";
		case kIOReturnNotResponding:	return "kIOReturnNotResponding";
		case kIOReturnIsoTooOld:		return "kIOReturnIsoTooOld";
		case kIOReturnIsoTooNew:		return "kIOReturnIsoTooNew";
		case kIOReturnNotFound:			return "kIOReturnNotFound";
		default:						break;
	}
	return "";
}	//	GetKernErrStr
