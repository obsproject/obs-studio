/* SPDX-License-Identifier: MIT */
/**
	@file		pnp/mac/masterport.cpp
	@brief		Implements the MasterPort class.
	@copyright	(C) 2013-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#include "masterport.h"
#include <iostream>


using namespace std;


static bool				gTerminating	(false);
static MasterPortPtr	gMasterPort;	//	Singleton


static ostream &	operator << (ostream & inOutStr, const MasterPort & inObj)
{
	return inOutStr << "MasterPort instance " << hex << &inObj << dec << " portRef " << hex << inObj.GetPortRef () << dec << " " << (inObj.IsOkay () ? "OK" : "Not OK");

}	//	MasterPort ostream operator <<


IONotificationPortRef MasterPort::Get (void)
{
	MasterPortPtr			masterPortInstance	(GetInstance ());
	IONotificationPortRef	masterPortRef		(NULL);

	assert (masterPortInstance && "Must have valid MasterPort instance");
	if (masterPortInstance)
	{
		masterPortRef = *masterPortInstance;
		assert (masterPortRef && "Must have valid masterPortRef");
	}

	return masterPortRef;

}	//	Get


MasterPortPtr MasterPort::GetInstance (void)
{
	if (!gMasterPort)
		Create (gMasterPort);

	return gMasterPort;

}	//	GetInstance


bool MasterPort::Create (MasterPortPtr & outObj)
{
	outObj = NULL;
	try
	{
		outObj = gTerminating ? NULL : new MasterPort;
		if (!outObj->IsOkay ())
			outObj = NULL;
	}
	catch (std::bad_alloc)
	{
	}
	return outObj;

}	//	Create


MasterPort::MasterPort ()
{
	//
	//	To set up asynchronous notifications, create a notification port, then add its run loop event source to our run loop.
	//	It is not necessary to call CFRunLoopRemoveSource during teardown because that's implicitly done when the notification
	//	port is destroyed.
	//
	mpMasterPort = IONotificationPortCreate (kIOMasterPortDefault);
	if (!mpMasterPort)
	{
		cerr << "IONotificationPortCreate returned NULL" << endl;
		return;
	}
	CFRunLoopSourceRef	pRunLoopSource	(IONotificationPortGetRunLoopSource (mpMasterPort));
	if (!pRunLoopSource)
	{
		cerr << "IONotificationPortGetRunLoopSource returned NULL" << endl;
		return;
	}
	CFRunLoopAddSource (CFRunLoopGetCurrent (), pRunLoopSource, kCFRunLoopDefaultMode);

}	//	constructor


MasterPort::~MasterPort ()
{
	cerr << *this << " destructor" << endl;
	if (mpMasterPort)
		IONotificationPortDestroy (mpMasterPort);
	mpMasterPort = NULL;

}	//	destructor


/**
	@brief	I sit around waiting for main() to exit(), whereupon I destroy the gMasterPort singleton.
**/
class MasterPortDestroyer
{
	public:
		explicit inline MasterPortDestroyer ()		{cerr << "MasterPortDestroyer is armed" << endl;}
		virtual inline ~MasterPortDestroyer ()
		{
			cerr << "MasterPortDestroyer triggered" << endl;
			gMasterPort = NULL;
		}	//	destructor

	private:
		//	Do not copy!
		inline							MasterPortDestroyer (const MasterPortDestroyer & inObj)		{if (&inObj != this) assert (false);}
		inline MasterPortDestroyer &	operator = (const MasterPortDestroyer & inRHS)				{if (&inRHS != this) assert (false); return *this;}

};	//	MasterPortDestroyer

static MasterPortDestroyer	gMasterPortDestroyer;	///	MasterPortDestroyer singleton
