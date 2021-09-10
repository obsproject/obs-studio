/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2konaflashprogram.h
	@brief		Declares the CNTV2KonaFlashProgram class.
	@copyright	(C) 2010-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef NTV2KONAFLASHPROGRAM_H
#define NTV2KONAFLASHPROGRAM_H

#include "ntv2card.h"
#include <fstream>
#include <vector>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include "ntv2debug.h"
#include "ntv2mcsfile.h"
#include "ntv2spiinterface.h"

#if defined(AJALinux) || defined(AJAMac)
#include <netinet/in.h>	// htons and friends
#endif

#define MAXBITFILE_HEADERSIZE 512
#define MAXMCSINFOSIZE 256
#define MAXMCSLICENSESIZE 256
#define MCS_STEPS      6

typedef enum 
{
	MAIN_FLASHBLOCK,
	FAILSAFE_FLASHBLOCK,
	AUTO_FLASHBLOCK,
	SOC1_FLASHBLOCK,
	SOC2_FLASHBLOCK,
	MAC_FLASHBLOCK,
	MCS_INFO_BLOCK,
	LICENSE_BLOCK
} FlashBlockID;

typedef enum
{
	BANK_0,
	BANK_1,
	BANK_2,
	BANK_3
} BankSelect;

struct MacAddr
{
	uint8_t mac[6];
	std::string AsString(void) const;
};


class AJAExport CNTV2FlashProgress
{
	public:
		static CNTV2FlashProgress &	nullUpdater;

						CNTV2FlashProgress()	{}
		virtual			~CNTV2FlashProgress()	{}
		virtual bool	UpdatePercentage (const size_t inPercentage)	{(void) inPercentage; return true;}
};	//	CNTV2PercentUpdater


class AJAExport CNTV2KonaFlashProgram : public CNTV2Card
{
public:
	CNTV2KonaFlashProgram();
	CNTV2KonaFlashProgram (const UWord boardNumber);
	virtual ~CNTV2KonaFlashProgram();
	static std::string	FlashBlockIDToString (const FlashBlockID inID, const bool inShortDisplay = false);	//	New in SDK 16.0

public:
	virtual bool	SetBoard (UWord boardNumber, uint32_t index = 0);
	bool			ReadHeader (FlashBlockID flashBlock);
	bool			ReadInfoString();
	void			SetBitFile (const char *bitFileName, FlashBlockID blockNumber = AUTO_FLASHBLOCK);
	bool			SetBitFile (const std::string & inBitfileName, const FlashBlockID blockNumber = AUTO_FLASHBLOCK);	//	New in SDK 16.0
	bool			SetMCSFile (const char *sMCSFileName);
	void			Program (bool fullVerify = false);
	bool            ProgramFromMCS(bool verify);
	bool            ProgramSOC(bool verify = true);
	void			ProgramCustom ( const char *sCustomFileName, const uint32_t addr);
	void			EraseBlock (FlashBlockID blockNumber);
	bool			EraseChip (UWord chip = 0);
	bool			CreateSRecord (bool bChangeEndian);
	bool			CreateEDIDIntelRecord ();
	void			SetQuietMode ();
	bool			VerifyFlash (FlashBlockID flashBlockNumber, bool fullVerify = false);
	bool			ReadFlash (NTV2_POINTER & outBuffer, const FlashBlockID flashID, CNTV2FlashProgress & inFlashProgress = CNTV2FlashProgress::nullUpdater);	//	New in SDK 16.0
	bool			SetBankSelect (BankSelect bankNumber);
	bool			SetFlashBlockIDBank(FlashBlockID blockID);
	bool            ROMHasBankSelect();
	uint32_t		ReadBankSelect ();
	void            SetMBReset();

	std::string & GetDesignName()
	{
		size_t index = _designName.find("HW_TIMEOUT");
		if(index != std::string::npos)
		{
			_designName.erase(index - 1, _designName.length()-1);
		}
		index = _designName.find("UserID");
		if(index != std::string::npos)
		{
			_designName.erase(index - 1, _designName.length() - 1);
		}
		return _designName;
	}
	const std::string & GetPartName (void) const	{return _partName;}
	const std::string & GetDate (void) const		{return _date;}
	const std::string & GetTime (void) const		{return _time;}
	uint32_t GetNumBytes(void) const				{return _numBytes;}
	std::string & GetMCSInfo(void)
	{
		std::string::size_type f = _mcsInfo.find("\xFF\xFF");
		_mcsInfo = _mcsInfo.substr(0, f);
		return _mcsInfo;
	}

	void ParsePartitionFromFileLines(uint32_t address, uint16_t & partitionOffset);
	bool CreateBankRecord(BankSelect bankID);

	bool ProgramMACAddresses(MacAddr * mac1, MacAddr * mac2);
	bool ReadMACAddresses(MacAddr & mac1, MacAddr & mac2);
	bool ProgramLicenseInfo(std::string licenseString);
	bool ReadLicenseInfo(std::string& licenseString);
	void DisplayData(uint32_t address, uint32_t len);
	bool ProgramInfoFromString(std::string infoString);
	void FullProgram(std::vector<uint8_t> & dataBuffer);

    int32_t  NextMcsStep() {return ++_mcsStep;}

	bool ParseHeader(char* headerAddress);
	bool WaitForFlashNOTBusy();
	bool ProgramFlashValue(uint32_t address, uint32_t value);
	bool FastProgramFlash256(uint32_t address, uint32_t* buffer);
	bool EraseSector(uint32_t sectorAddress);
	bool CheckFlashErasedWithBlockID(FlashBlockID flashBlockNumber);
	uint32_t ReadDeviceID();
	bool SetDeviceProperties();
	void DetermineFlashTypeAndBlockNumberFromFileName(const std::string & bitFileName);
	void SRecordOutput (const char *pSRecord);

	uint32_t GetSectorAddressForSector(FlashBlockID flashBlockNumber,uint32_t sectorNumber)
	{
		return GetBaseAddressForProgramming(flashBlockNumber)+(sectorNumber*_sectorSize);
	}

	uint32_t GetBaseAddressForProgramming(FlashBlockID flashBlockNumber)
	{
		switch ( flashBlockNumber )
		{
			default:
			case MAIN_FLASHBLOCK:		return _mainOffset;
			case FAILSAFE_FLASHBLOCK:	return _failSafeOffset;
			case SOC1_FLASHBLOCK:		return _soc1Offset;
			case SOC2_FLASHBLOCK:		return _soc2Offset;
			case MAC_FLASHBLOCK:		return _macOffset;
			case MCS_INFO_BLOCK:		return _mcsInfoOffset;
			case LICENSE_BLOCK:			return _licenseOffset;
		}

	}

	uint32_t GetNumberOfSectors(FlashBlockID flashBlockNumber)
	{
		switch ( flashBlockNumber )
		{
			default:
			case MAIN_FLASHBLOCK:		return _numSectorsMain;
			case FAILSAFE_FLASHBLOCK:	return _numSectorsFailSafe;
			case SOC1_FLASHBLOCK:		return _numSectorsSOC1;
			case SOC2_FLASHBLOCK:		return _numSectorsSOC2;
			case MAC_FLASHBLOCK:		return 1;
			case MCS_INFO_BLOCK:		return 1;
			case LICENSE_BLOCK:			return 1;
		}
	}

	bool VerifySOCPartition(FlashBlockID flashID, uint32_t FlashBlockOffset);
	bool CheckAndFixMACs();
	bool MakeMACsFromSerial( const char *sSerialNumber, MacAddr *pMac1, MacAddr *pMac2 );

protected:
	uint8_t *		_bitFileBuffer;
	uint8_t *		_customFileBuffer;
	uint32_t		_bitFileSize;
	std::string		_bitFileName;
	std::string		_date;
	std::string		_time;
	std::string		_designName;
	std::string		_partName;
	std::string		_mcsInfo;
	uint32_t		_spiDeviceID;
	uint32_t		_flashSize;
	uint32_t		_bankSize;
	uint32_t		_sectorSize;
	uint32_t		_mainOffset;
	uint32_t		_failSafeOffset;
	uint32_t		_macOffset;
    uint32_t		_mcsInfoOffset;
	uint32_t		_licenseOffset;
    uint32_t		_soc1Offset;
    uint32_t		_soc2Offset;
	uint32_t		_numSectorsMain;
	uint32_t		_numSectorsSOC1;
	uint32_t		_numSectorsSOC2;
	uint32_t		_numSectorsFailSafe;
	uint32_t		_numBytes;
	FlashBlockID	_flashID;
	uint32_t		_deviceID;
	bool			_bQuiet;
    int32_t			_mcsStep;
    CNTV2MCSfile	_mcsFile;
	std::vector<uint8_t> _partitionBuffer;
    uint32_t		_failSafePadding;
    CNTV2SpiFlash *	_spiFlash;

	typedef enum {
		READID_COMMAND			= 0x9F,
		WRITEENABLE_COMMAND		= 0x06,
		WRITEDISABLE_COMMAND	= 0x04,
		READSTATUS_COMMAND		= 0x05,
		WRITESTATUS_COMMAND		= 0x01,
		READFAST_COMMAND		= 0x0B,
		PAGEPROGRAM_COMMAND		= 0x02,
		SECTORERASE_COMMAND		= 0xD8,
		CHIPERASE_COMMAND		= 0xC7,
		BANKSELECT_COMMMAND		= 0x17,
		READBANKSELECT_COMMAND	= 0x16
	} _FLASH_STUFF;

};	//	CNTV2KonaFlashProgram

#endif	//	NTV2KONAFLASHPROGRAM_H
