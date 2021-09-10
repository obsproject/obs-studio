/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2dynamicdevice.cpp
	@brief		Implementations of DMA-related CNTV2Card methods.
	@copyright	(C) 2004-2021 AJA Video Systems, Inc.
**/

#include "ntv2card.h"
#include "ntv2devicefeatures.h"
#include "ntv2utils.h"
#include "ntv2bitfile.h"
#include "ntv2bitfilemanager.h"

using namespace std;


static CNTV2BitfileManager s_BitfileManager;


bool CNTV2Card::IsDynamicDevice (void)
{
    NTV2ULWordVector reg;

	if (!IsOpen())
		return false;

	// see if we can get bitstream status
    if (!BitstreamStatus(reg))
		return false;

	// the bitstream version cannot be 0
	if (reg[BITSTREAM_VERSION] == 0)
		return false;

	return true;
}

bool CNTV2Card::IsDynamicFirmwareLoaded()
{
	if(!IsDynamicDevice())
		return false;
	
	ULWord value(0);
	ReadRegister(kVRegBaseFirmwareDeviceID, value);
	if(GetDeviceID() == static_cast<NTV2DeviceID>(value))
		return false;
	else
		return true;
}

NTV2DeviceID CNTV2Card::GetBaseDeviceID()
{
	if(!IsDynamicDevice())
		return DEVICE_ID_INVALID;
	
	ULWord value(0);
	ReadRegister(kVRegBaseFirmwareDeviceID, value);
	return static_cast<NTV2DeviceID>(value);
}

NTV2DeviceIDList CNTV2Card::GetDynamicDeviceList (void)
{
	NTV2DeviceIDList	result;
	const NTV2DeviceIDSet devs(GetDynamicDeviceIDs());
	for (NTV2DeviceIDSetConstIter it(devs.begin());  it != devs.end();  ++it)
		result.push_back(*it);
	return result;
}

NTV2DeviceIDSet CNTV2Card::GetDynamicDeviceIDs (void)
{
	NTV2DeviceIDSet result;
	if (!IsOpen())
		return result;

    const NTV2DeviceID currentDeviceID (GetDeviceID());
    if (currentDeviceID == 0)
        return result;

    //	Get current design ID and version...
    NTV2ULWordVector reg;
    if (!BitstreamStatus(reg))
		return result;

	if (reg[BITSTREAM_VERSION] == 0)
		return result;

    ULWord currentUserID = 0;
    ULWord currentDesignID = 0;
    ULWord currentDesignVersion = 0;
    ULWord currentBitfileID = 0;
    ULWord currentBitfileVersion = 0;

    if (GetRunningFirmwareUserID(currentUserID) && (currentUserID != 0))
    {
        // the new way
        currentDesignID = CNTV2Bitfile::GetDesignID(currentUserID);
        currentDesignVersion = CNTV2Bitfile::GetDesignVersion(currentUserID);
        currentBitfileID = CNTV2Bitfile::GetBitfileID(currentUserID);
        currentBitfileVersion = CNTV2Bitfile::GetBitfileVersion(currentUserID);
    }
    else
    {
        // the old way
        currentDesignID = CNTV2Bitfile::GetDesignID(reg[BITSTREAM_VERSION]);
        currentDesignVersion = CNTV2Bitfile::GetDesignVersion(reg[BITSTREAM_VERSION]);
        currentBitfileID = CNTV2Bitfile::ConvertToBitfileID(currentDeviceID);
        currentBitfileVersion = 0xff; // ignores bitfile version
    }

    if (currentDesignID == 0)
		return result;

	//	Get the clear file matching current bitfile...
	NTV2_POINTER clearStream;
	if (!s_BitfileManager.GetBitStream (clearStream,
                                    currentDesignID,
                                    currentDesignVersion,
									currentBitfileID,
                                    currentBitfileVersion,
									NTV2_BITFILE_FLAG_CLEAR) || !clearStream)
		return result;

	//	Build the deviceID set...
	const NTV2BitfileInfoList infoList(s_BitfileManager.GetBitfileInfoList());
	for (NTV2BitfileInfoListConstIter it(infoList.begin());  it != infoList.end();  ++it)
        if (it->designID == currentDesignID)
            if (it->designVersion == currentDesignVersion)
				if (it->bitfileFlags & NTV2_BITFILE_FLAG_PARTIAL)
				{
					const NTV2DeviceID devID (CNTV2Bitfile::ConvertToDeviceID(it->designID, it->bitfileID));
					if (result.find(devID) == result.end())
						result.insert(devID);
				}
	return result;
}

bool CNTV2Card::CanLoadDynamicDevice (const NTV2DeviceID inDeviceID)
{
	const NTV2DeviceIDSet devices(GetDynamicDeviceIDs());
	return devices.find(inDeviceID) != devices.end();
}

bool CNTV2Card::LoadDynamicDevice (const NTV2DeviceID inDeviceID)
{
	if (!IsOpen())
		return false;

    const NTV2DeviceID currentDeviceID (GetDeviceID());
    if (currentDeviceID == 0)
        return false;

	//	Get current design ID and version...
    NTV2ULWordVector reg;
    if (!BitstreamStatus(reg))
		return false;

	if (reg[BITSTREAM_VERSION] == 0)
		return false;

    ULWord currentUserID = 0;
    ULWord currentDesignID = 0;
    ULWord currentDesignVersion = 0;
    ULWord currentBitfileID = 0;
    ULWord currentBitfileVersion = 0;

    if (GetRunningFirmwareUserID(currentUserID) && (currentUserID != 0))
    {
        // the new way
        currentDesignID = CNTV2Bitfile::GetDesignID(currentUserID);
        currentDesignVersion = CNTV2Bitfile::GetDesignVersion(currentUserID);
        currentBitfileID = CNTV2Bitfile::GetBitfileID(currentUserID);
        currentBitfileVersion = CNTV2Bitfile::GetBitfileVersion(currentUserID);
    }
    else
    {
        // the old way
        currentDesignID = CNTV2Bitfile::GetDesignID(reg[BITSTREAM_VERSION]);
        currentDesignVersion = CNTV2Bitfile::GetDesignVersion(reg[BITSTREAM_VERSION]);
        currentBitfileID = CNTV2Bitfile::ConvertToBitfileID(currentDeviceID);
        currentBitfileVersion = 0xff; // ignores bitfile version
    }

    if (currentDesignID == 0)
        return false;

	//	Get the clear file matching current bitfile...
	NTV2_POINTER clearStream;
	if (!s_BitfileManager.GetBitStream (clearStream,
                                    currentDesignID,
                                    currentDesignVersion,
									currentBitfileID,
                                    currentBitfileVersion,
									NTV2_BITFILE_FLAG_CLEAR) || !clearStream)
		return false;

	//	Get the partial file matching the inDeviceID...
	NTV2_POINTER partialStream;
	if (!s_BitfileManager.GetBitStream (partialStream,
                                    currentDesignID,
                                    currentDesignVersion,
									CNTV2Bitfile::ConvertToBitfileID(inDeviceID),
									0xff,
									NTV2_BITFILE_FLAG_PARTIAL) || !partialStream)
		return false;

	//	Load the clear bitstream...
	if (!BitstreamWrite (clearStream, true, true))
		return false;
	//	Load the partial bitstream...
	if (!BitstreamWrite (partialStream, false, true))
		return false;

	return true;
}	//	LoadDynamicDevice

bool CNTV2Card::AddDynamicBitfile (const string & inBitfilePath)
{
	return s_BitfileManager.AddFile(inBitfilePath);
}

bool CNTV2Card::AddDynamicDirectory (const string & inDirectory)
{
	return s_BitfileManager.AddDirectory(inDirectory);
}
