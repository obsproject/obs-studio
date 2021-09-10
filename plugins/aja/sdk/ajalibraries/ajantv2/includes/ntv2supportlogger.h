/* SPDX-License-Identifier: MIT */
/**
    @file		ntv2supportlogger.h
    @brief		Declares the CNTV2SupportLogger class.
    @copyright	(C) 2017-2021 AJA Video Systems, Inc.    All rights reserved.
**/

#ifndef NTV2SUPPORTLOGGER_H
#define NTV2SUPPORTLOGGER_H

#include "ntv2card.h"
#include <map>
#include <string>
#include "ntv2utils.h"

typedef enum
{
    NTV2_SupportLoggerSectionInfo           = 0x00000001 << 0,
    NTV2_SupportLoggerSectionAutoCirculate  = 0x00000001 << 1,
    NTV2_SupportLoggerSectionAudio          = 0x00000001 << 2,
    NTV2_SupportLoggerSectionRouting        = 0x00000001 << 3,
    NTV2_SupportLoggerSectionRegisters      = 0x00000001 << 4,
    NTV2_SupportLoggerSectionsAll           = 0xFFFFFFFF
} NTV2SupportLoggerSections;

/**
	@brief	Generates a standard support log (register log) for any NTV2 device attached to the host.
			To write the log into a file, open a std::ofstream, then stream this object into it.
**/
class AJAExport CNTV2SupportLogger
{
public:
	/**
		@brief		Construct from CNTV2Card instance.
		@param[in]	card		Specifies the CNTV2Card instance of the device to be logged. The instance should already be open.
		@param[in]	sections	Optionally specifies which sections to include in the log. Defaults to all sections.
	**/
	CNTV2SupportLogger (CNTV2Card & card,
						NTV2SupportLoggerSections sections = NTV2_SupportLoggerSectionsAll);

	/**
		@brief		Default constructor.
		@param[in]	cardIndex	Optionally specifies the zero-based index of the CNTV2Card device to be logged.
								Defaults to zero, the first device found.
		@param[in]	sections	Optionally specifies which sections to include in the log. Defaults to all sections.
	**/
	CNTV2SupportLogger (UWord cardIndex = 0,
						NTV2SupportLoggerSections sections = NTV2_SupportLoggerSectionsAll);

	virtual				~CNTV2SupportLogger	();	///< @brief	My default destructor

	static int			Version				(void);	///< @return	The log file version I will produce.

	/**
		@brief		Prepends arbitrary string data to my support log, ahead of a given section.
		@param[in]	section			Specifies the NTV2SupportLoggerSection to prepend.
		@param[in]	sectionData		Specifies the text data to prepend.
	**/
	virtual void		PrependToSection	(uint32_t section, const std::string & sectionData);

	/**
		@brief		Appends arbitrary string data to my support log, after a given section.
		@param[in]	section			Specifies the NTV2SupportLoggerSection to append.
		@param[in]	sectionData		Specifies the text data to append.
	**/
	virtual void		AppendToSection		(uint32_t section, const std::string & sectionData);

	/**
		@brief		Adds header text to my log.
		@param[in]	sectionName		Specifies the section name.
		@param[in]	sectionData		Specifies the header text.
	**/
	virtual void		AddHeader			(const std::string & sectionName, const std::string & sectionData);

	/**
		@brief		Adds footer text to my log.
		@param[in]	sectionName		Specifies the section name.
		@param[in]	sectionData		Specifies the footer text.
	**/
	virtual void		AddFooter			(const std::string & sectionName, const std::string & sectionData);

	virtual std::string	ToString			(void) const;	///< @return	My entire support log as a standard string.

	/**
		@brief		Writes my support log into a string object.
		@param[out]	outString	Receives my entire support log as a standard string, replacing its contents.
	**/
	virtual void		ToString			(std::string & outString) const;

	virtual bool		LoadFromLog			(const std::string & inLogFilePath, const bool bForceLoad);


private:
	void	FetchInfoLog			(std::ostringstream& oss) const;
	void	FetchRegisterLog		(std::ostringstream& oss) const;
	void	FetchAutoCirculateLog	(std::ostringstream& oss) const;
	void	FetchAudioLog			(std::ostringstream& oss) const;
	void	FetchRoutingLog			(std::ostringstream& oss) const;

private:
	CNTV2Card &						mDevice;
	bool							mDispose;
	NTV2SupportLoggerSections		mSections;
	std::string						mHeaderStr;
	std::string						mFooterStr;
	std::map<uint32_t, std::string>	mPrependMap;
	std::map<uint32_t, std::string>	mAppendMap;

public:
	static std::string	InventLogFilePathAndName (CNTV2Card & inDevice,
													std::string inPrefix = "aja_supportlog",
													std::string inExtension = "log");	//	New in SDK 16.0
	static bool			DumpDeviceSDRAM (CNTV2Card & inDevice,
										const std::string & inFilePath,
										std::ostream & msgStream);	//	New in SDK 16.0
};	//	CNTV2SupportLogger


/**
	@brief		Writes a given CNTV2SupportLogger's text into the given output stream.
	@param		outStream		Specifies the output stream to be written.
	@param[in]	inData			Specifies the CNTV2SupportLogger.
	@return		A reference to the same output stream that was specified in "outStream".
**/
AJAExport std::ostream & operator << (std::ostream & outStream, const CNTV2SupportLogger & inData);

#endif // NTV2SUPPORTLOGGER_H
