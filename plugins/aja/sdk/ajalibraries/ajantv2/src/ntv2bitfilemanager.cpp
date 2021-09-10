/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2bitfilemanager.cpp
	@brief		Implementation of CNTV2BitfileManager class.
	@copyright	(C) 2019-2021 AJA Video Systems, Inc.    All rights reserved.
**/
#include "ntv2bitfilemanager.h"
#include "ntv2bitfile.h"
#include "ntv2utils.h"
#include "ajabase/system/file_io.h"
#include <iostream>
#include <sys/stat.h>
#include <assert.h>
#if defined (AJALinux) || defined (AJAMac)
	#include <arpa/inet.h>
#endif
#include <map>

using namespace std;


CNTV2BitfileManager::CNTV2BitfileManager()
{
}

CNTV2BitfileManager::~CNTV2BitfileManager()
{
	Clear();
}

bool CNTV2BitfileManager::AddFile (const string & inBitfilePath)
{
	AJAFileIO Fio;
	CNTV2Bitfile Bitfile;
	NTV2BitfileInfo Info;

	//	Open bitfile...
    if (!Fio.FileExists(inBitfilePath)) {
        return false;
	}
    if (!Bitfile.Open(inBitfilePath)) {
        return false;
	}

	// get bitfile information
	Info.bitfilePath	= inBitfilePath;
	Info.designName		= Bitfile.GetDesignName();
	Info.designID		= Bitfile.GetDesignID();
	Info.designVersion	= Bitfile.GetDesignVersion();
	Info.bitfileID		= Bitfile.GetBitfileID();
	Info.bitfileVersion	= Bitfile.GetBitfileVersion();
    if (Bitfile.IsTandem()) {
        Info.bitfileFlags = NTV2_BITFILE_FLAG_TANDEM;
	}
    else if (Bitfile.IsClear()) {
        Info.bitfileFlags = NTV2_BITFILE_FLAG_CLEAR;
	}
    else if (Bitfile.IsPartial()) {
        Info.bitfileFlags = NTV2_BITFILE_FLAG_PARTIAL;
	}
    else {
        Info.bitfileFlags = 0;
	}
	Info.deviceID		= Bitfile.GetDeviceID();

	//	Check for reconfigurable bitfile...
    if ((Info.designID == 0) || (Info.designID > 0xfe)) {
        return false;
	}
    if (Info.designVersion > 0xfe) {
        return false;
	}
	if ((Info.bitfileID > 0xfe)) {
        return false;
	}
    if (Info.bitfileVersion > 0xfe) {
        return false;
	}
    if (Info.bitfileFlags == 0) {
        return false;
	}
    if (Info.deviceID == 0) {
        return false;
	}

	//	Add to list...
	_bitfileList.push_back(Info);
	return true;
}

bool CNTV2BitfileManager::AddDirectory (const string & inDirectory)
{
	AJAFileIO Fio;

	//	Check if good directory...
    if (AJA_FAILURE(Fio.DoesDirectoryExist(inDirectory))) {
        return false;
	}

	//	Get bitfiles...
	NTV2StringList fileContainer;
	Fio.ReadDirectory(inDirectory, "*.bit", fileContainer);

	// add bitfiles
	for (NTV2StringListConstIter fcIter(fileContainer.begin());  fcIter != fileContainer.end();  ++fcIter)
        AddFile(*fcIter);
	
	return true;
}

void CNTV2BitfileManager::Clear (void)
{
	_bitfileList.clear();
	_bitstreamList.clear();
}

size_t CNTV2BitfileManager::GetNumBitfiles (void)
{
	return _bitfileList.size();
}

NTV2BitfileInfoList & CNTV2BitfileManager::GetBitfileInfoList (void)
{
	return _bitfileList;
}

bool CNTV2BitfileManager::GetBitStream (NTV2_POINTER & outBitstream,
										const ULWord inDesignID,
										const ULWord inDesignVersion,
										const ULWord inBitfileID,
										const ULWord inBitfileVersion,
										const ULWord inBitfileFlags)
{
	size_t numBitfiles (GetNumBitfiles());
	size_t maxNdx (numBitfiles);
	size_t ndx(0);

	for (ndx = 0;  ndx < numBitfiles;  ndx++)
	{	//	Search for bitstream...
        const NTV2BitfileInfo & info (_bitfileList.at(ndx));
        if (inDesignID == info.designID)
			if (inDesignVersion == info.designVersion)
				if (inBitfileID == info.bitfileID)
					if (inBitfileFlags & info.bitfileFlags)
					{
						if (inBitfileVersion == info.bitfileVersion)
							break;
						if ((maxNdx >= numBitfiles) || (info.bitfileVersion > _bitfileList.at(maxNdx).bitfileVersion))
							maxNdx = ndx;
					}
	}

	//	Looking for latest version?
	if ((inBitfileVersion == 0xff)  &&  (maxNdx < numBitfiles))
		ndx = maxNdx;

	//	Find something?
	if (ndx == numBitfiles)
		return false;	//	Nope

	//	Read in bitstream...
	if (!ReadBitstream(ndx))
		return false;

	outBitstream = _bitstreamList[ndx];
	return true;
}

bool CNTV2BitfileManager::ReadBitstream (const size_t inIndex)
{
	//	Already in cache?
    if ((inIndex < _bitstreamList.size())  &&  !_bitstreamList.at(inIndex).IsNULL()) {
        return true;	//	Yes
	}

	//	Open bitfile to get bitstream...
	CNTV2Bitfile Bitfile;
    if (!Bitfile.Open(_bitfileList.at(inIndex).bitfilePath))
        return false;

	//	Read bitstream from bitfile (will automatically Allocate it)...
    NTV2_POINTER Bitstream;
	if (!Bitfile.GetProgramByteStream(Bitstream))
		return false;

    if (inIndex >= _bitstreamList.size())
        _bitstreamList.resize(inIndex + 1);

    _bitstreamList[inIndex] = Bitstream;
	return true;
}
