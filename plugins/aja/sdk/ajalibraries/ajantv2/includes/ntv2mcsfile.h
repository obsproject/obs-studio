/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2mcsfile.h
	@brief		Declares the CNTV2MCSfile class.
	@copyright	(C) 2015-2021 AJA Video Systems, Inc.    All rights reserved.
**/

#ifndef NTV2MCSFILE_H
#define NTV2MCSFILE_H

#include <string>
#include <vector>
#include <fstream>
#include "ntv2utils.h"
#ifdef AJALinux
	#include <stdint.h>
	#include <stdlib.h>
#endif

typedef enum
{
	IRT_UNKNOWN,
	IRT_DR,		//	Data Record
	IRT_EOFR,	//	End Of File Record
	IRT_ESAR,	//	Extended Segment Address Record
	IRT_ELAR	//	Extended Linear Address Record
} IntelRecordType;


struct IntelRecordInfo
{
		IntelRecordInfo()
			:	byteCount		(0)
			,	linearAddress	(0)
			,	segmentAddress	(0)
			,	recordType		(IRT_UNKNOWN)
			,	checkSum		(0)				{}

	uint8_t			byteCount;
	uint16_t		linearAddress;
	uint32_t		segmentAddress;
	IntelRecordType	recordType;
	uint8_t *		dataBuffer[16];
	uint8_t			checkSum;
};


/**
	@brief	Instances of me can parse an MCS file.
**/
class CNTV2MCSfile
{
public:
	/**
		@brief		My constructor.
	**/
	CNTV2MCSfile();

	/**
		@brief		My destructor.
	**/
	virtual								~CNTV2MCSfile();

	/**
		@brief		Opens the bitfile at the given path, then parses its header.
		@param[in]	inMCSFilePath	Specifies the path to the MCS bitfile to be parsed.
		@return		True if open & parse succeeds; otherwise false.
	**/
	virtual bool						Open (const std::string & inMCSFilePath);

	/**
		@brief	Closes mcs file (if open).
	**/
	virtual void						Close (void);
	virtual bool						InsertBitFile (const std::string & inBitFileName, const std::string & inMCSFileName, const std::string & inUserMessage);
	virtual void						IRecordOutput (const char *pIRecord);

	virtual uint32_t					GetFileByteStream (uint32_t numberOfLines = 0);
	virtual bool						isReady (void) const;
	virtual bool						FindExtendedLinearAddressRecord (uint16_t address = 0x0000);
	virtual bool						GetCurrentParsedRecord (IntelRecordInfo &recordInfo);
	virtual uint32_t					GetPartition (UByteSequence & patitionBuffer, uint16_t baseELARaddress, uint16_t & partitionOffset, bool nextPartition = false);
	virtual const std::string &			GetBitfileDateString (void) const			{return mBitfileDate;}
	virtual const std::string &			GetBitfileDesignString (void) const			{return mBitfileDesignName;}
	virtual const std::string &			GetBitfilePartNameString (void) const		{return mBitfilePartName;}
	virtual const std::string &			GetBitfileTimeString (void) const			{return mBitfileTime;}
	virtual std::string					GetMCSPackageVersionString (void) const;
	virtual std::string					GetMCSPackageDateString (void) const;
	virtual bool						GetMCSHeaderInfo (const std::string & inMCSFileName);

private:
	virtual bool						ParseCurrentRecord (IntelRecordInfo &recordInfo);
	virtual void						GetMCSInfo ();

	std::ifstream			mMCSFileStream;
	std::string				mCurrentRecord;
	uint32_t				mFileSize;
	NTV2StringList			mFileLines;
	NTV2StringListIter		mBaseELARLocation;
	NTV2StringListIter		mCurrentLocation;
	std::string				mCommentString;
	std::string				mMCSInfoString;

	//bitfile information contained in the MCS file
	std::string				mBitfileDate;
	std::string				mBitfileTime;
	std::string				mBitfileDesignName;
	std::string				mBitfilePartName;

};	//	CNTV2MCSfile

#endif // NTV2MCSFILE_H
