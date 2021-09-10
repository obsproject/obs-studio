/* SPDX-License-Identifier: MIT */
/**
	@file		dpxfileio.cpp
	@brief		Implementation of the AJADPXFileIO class, for low level file I/O.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#include <stdio.h>

#include "dpxfileio.h"
#include "ajabase/system/file_io.h"

using std::string;
using std::vector;


AJADPXFileIO::AJADPXFileIO ()
	:	mPathSet		(false),
		mLoopMode		(true),
		mPauseMode		(false),
		mFileCount		(0),
		mCurrentIndex	(0)
{
}	//	constructor


AJADPXFileIO::~AJADPXFileIO ()
{
	mFileList.clear();
}	//	destructor


uint32_t AJADPXFileIO::GetFileCount () const
{
	return mFileCount;
}	//	GetFileCount


vector<string> &  AJADPXFileIO::GetFileList ()
{
	return mFileList;
}	//	GetFileList


uint32_t AJADPXFileIO::GetIndex () const
{
	return mCurrentIndex;
}	//	GetIndex


bool AJADPXFileIO::GetLoopMode () const
{
	return mLoopMode;
}	//	GetLoopMode


bool AJADPXFileIO::GetPauseMode () const
{
	return mPauseMode;
}	//	GetPauseMode


string AJADPXFileIO::GetPath () const
{
	return mPath;
}	//	GetPath


AJAStatus AJADPXFileIO::Read (const uint32_t index)
{
    AJA_UNUSED(index);

	AJAFileIO	file;
	AJAStatus 	status = AJA_STATUS_SUCCESS;

	//	Check that we've been initialzed
	if (!mPathSet)
		return AJA_STATUS_INITIALIZE;

	//	Check that the index is in range of the vector
	if (mCurrentIndex >= mFileCount)
		return AJA_STATUS_RANGE;

	//	Get the name of the next file to open, then do so
	string fileName = mFileList [mCurrentIndex];
	status = file.Open (fileName, eAJAReadOnly, eAJAUnbuffered);

	//	Read the header from the file
	if (AJA_STATUS_SUCCESS == status)
	{
		uint32_t bytesRead = file.Read ((uint8_t*)&GetHdr(), uint32_t(GetHdrSize()));
		if (bytesRead != GetHdrSize())
			status = AJA_STATUS_IO;
	}

	//	Done with the file
	file.Close ();

	return status;
}


AJAStatus AJADPXFileIO::Read (uint8_t  &		buffer,
							  const uint32_t	bufferSize,
							  uint32_t &		index)
{
    AJA_UNUSED(bufferSize);

	AJAFileIO	file;
	AJAStatus 	status = AJA_STATUS_SUCCESS;

	//	Check that we've been initialzed
	if (!mPathSet)
		return AJA_STATUS_INITIALIZE;

	//	Check that the index is in range of the vector
	if (mCurrentIndex >= mFileCount)
		return AJA_STATUS_RANGE;

	//	Get the name of the next file to open, then do so
	string fileName = mFileList [mCurrentIndex];
	status = file.Open (fileName, eAJAReadOnly, eAJAUnbuffered);

	//	Read the header from the file
	if (AJA_STATUS_SUCCESS == status)
	{
		uint32_t bytesRead = file.Read ((uint8_t*)&GetHdr(), uint32_t(GetHdrSize()));
		if (bytesRead != GetHdrSize())
			status = AJA_STATUS_IO;
	}

	//	Sanity check the "magic number"
	if (AJA_STATUS_SUCCESS == status)
	{
		if (!DPX_VALID( &GetHdr() ))
			status = AJA_STATUS_UNSUPPORTED;
	}

	//	Seek to the image payload
	if (AJA_STATUS_SUCCESS == status)
	{
		status = file.Seek (get_fi_image_offset(), eAJASeekSet);
	}

	//	Read the payload
	if (AJA_STATUS_SUCCESS == status)
	{
		uint32_t bytesRead = file.Read ((uint8_t*)&buffer, uint32_t(get_ii_image_size()));
		if (bytesRead != get_ii_image_size())
			status = AJA_STATUS_IO;
	}

	//	Done with the file
	file.Close ();

	//	Report the current index in the sequence
	index = mCurrentIndex;

	//	Don't update index if paused
	if (mPauseMode)
		return status;

	//	Loop playback if active
	if (mLoopMode && (mCurrentIndex + 1 >= mFileCount))
	{
		mCurrentIndex = 0;
	}
	else
	{
		//	Bump the index even if there was an error, so we skip the bad file
		mCurrentIndex++;
	}

	return status;
}	//	Read


void AJADPXFileIO::SetFileList (vector<string> & list)
{
	mFileList.clear();

	mFileList = list;
}	//	SetFileList


AJAStatus AJADPXFileIO::SetIndex (const uint32_t &	index)
{
	if (index && (index > mFileCount - 1))
		return AJA_STATUS_RANGE;

	mCurrentIndex = index;

	return AJA_STATUS_SUCCESS;
}	//	SetIndex


void AJADPXFileIO::SetLoopMode (bool mode)
{
	mLoopMode = mode;
}	//	SetLoopMode


void AJADPXFileIO::SetPauseMode (bool mode)
{
	mPauseMode = mode;
}	//	SetPauseMode


AJAStatus AJADPXFileIO::SetPath (const string &	path)
{
	AJAFileIO	file;

	mPath = path;
	mFileList.clear();

	//	Construct a list of all the DPX files in the current location
	AJAStatus status = file.ReadDirectory (mPath, "*.dpx", mFileList);
	if (AJA_STATUS_SUCCESS != status)
	{
		mFileList.clear();
		return status;
	}

	mFileCount = uint32_t(mFileList.size());
	mCurrentIndex = 0;

	mPathSet = true;

	return AJA_STATUS_SUCCESS;
}	//	SetPath


AJAStatus AJADPXFileIO::Write (const uint8_t  &	buffer,
							   const uint32_t	bufferSize,
							   const uint32_t &	index) const
{
	string	fileName;
	AJAFileIO	file;
	AJAStatus 	status = AJA_STATUS_SUCCESS;

	//	Check that we've been initialzed
	if (!mPathSet)
		return AJA_STATUS_INITIALIZE;

	//	Construct the file name
	char asciiSequence[9];
	ajasnprintf (asciiSequence, 9, "%.8d", index);

	fileName = mPath + "/" + asciiSequence + ".DPX";

	//	Open the file or return an error code
	status = file.Open (fileName, eAJAWriteOnly, eAJAUnbuffered);
	if (AJA_STATUS_SUCCESS != status)
		return status;

	//	Write the DPX header to the file
	size_t headerSize = GetHdrSize();
	uint32_t bytesWritten = file.Write ((const uint8_t*)&GetHdr(), uint32_t(headerSize));
	if (bytesWritten != headerSize)
	{
		status = AJA_STATUS_IO;
	}

	//	Write the DPX payload to the file
	bytesWritten = file.Write ((uint8_t*)&buffer, bufferSize);
	if (bytesWritten != bufferSize)
	{
		status = AJA_STATUS_IO;
	}

	//	Finished with this file
	file.Close ();

	return status;
}	//	Write

