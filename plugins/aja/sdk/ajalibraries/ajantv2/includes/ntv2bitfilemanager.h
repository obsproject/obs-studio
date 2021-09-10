/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2bitfilemanager.h
	@brief		Declares the CNTV2BitfileManager class that manages Xilinx bitfiles.
	@copyright	(C) 2019-2021 AJA Video Systems, Inc.    All rights reserved.
**/

#ifndef NTV2BITMANAGER_H
#define NTV2BITMANAGER_H

#include <string>
#include <vector>
#include <fstream>
#ifdef AJALinux
	#include <stdint.h>
	#include <stdlib.h>
#endif
#include "ntv2publicinterface.h"

/**
	Bitfile information flags.
**/
#define NTV2_BITFILE_FLAG_TANDEM		BIT(0)		///< @brief	This is a tandem bitfile
#define NTV2_BITFILE_FLAG_PARTIAL		BIT(1)		///< @brief	This is a partial bitfile
#define NTV2_BITFILE_FLAG_CLEAR			BIT(2)		///< @brief	This is a clear bitfile


/**
	@brief	Bitfile information.
**/
struct AJAExport NTV2BitfileInfo
{
	std::string		bitfilePath;		///< @brief	The path where this bitfile was found.
	std::string		designName;			///< @brief	The design name for this bitfile.
	ULWord			designID;			///< @brief	Identifies the firmware core (the design base common to all its personalities).
	ULWord			designVersion;		///< @brief	Version of this core.
	ULWord			bitfileID;			///< @brief	Identifies the firmware personality.
	ULWord			bitfileVersion;		///< @brief	Version of this personality.
	ULWord			bitfileFlags;
	NTV2DeviceID	deviceID;			///< @brief	The NTV2DeviceID for this firmware core+personality.
};

typedef std::vector <NTV2BitfileInfo>		NTV2BitfileInfoList;
typedef NTV2BitfileInfoList::iterator		NTV2BitfileInfoListIter;
typedef NTV2BitfileInfoList::const_iterator	NTV2BitfileInfoListConstIter;


/**
	@brief	I manage and cache any number of bitfiles for any number of NTV2 devices/designs.
**/
class AJAExport CNTV2BitfileManager
{
public:
	/**
		@brief		My constructor.
	**/
	CNTV2BitfileManager ();

	/**
		@brief		My destructor.
	**/
	virtual								~CNTV2BitfileManager ();

	/**
		@brief		Add the bitfile at the given path to the list of bitfiles.
		@param[in]	inBitfilePath	Specifies the path name to the bitfile.
		@return		True if successful; otherwise false.
	**/
	virtual bool						AddFile (const std::string & inBitfilePath);

	/**
		@brief		Add the bitfile(s) at the given path to the list of bitfiles.
		@param[in]	inDirectory		Specifies the path name to the directory.
		@return		True if successful; otherwise false.
	**/
	virtual bool						AddDirectory (const std::string & inDirectory);

	/**
		@brief		Clear the list of bitfiles.
	**/
	virtual void						Clear (void);

	/**
		@brief		Returns the number of bitfiles.
		@return		Number of bitfiles.
	**/
	virtual size_t						GetNumBitfiles (void);

	/**
		@brief	Returns an NTV2BitfileInfoList standard C++ vector.
		@return	A reference to my NTV2BitfileInfoList.
	**/
	virtual NTV2BitfileInfoList &		GetBitfileInfoList (void);

	/**
		@brief		Retrieves the bitstream specified by design ID & version, and bitfile ID & version.
					It loads it into host memory, and updates/reallocates the given NTV2_POINTER to access it.
		@param[out]	outBitstream		Receives the bitstream in this NTV2_POINTER.
		@param[in]	inDesignID			Specifies the design ID.
		@param[in]	inDesignVersion		Specifies the design version.
		@param[in]	inBitfileID			Specifies the bitfile ID.
		@param[in]	inBitfileVersion	Specifies the bitfile version (0xff for latest).
		@param[in]	inBitfileFlags		Specifies the bitfile flags.
		@return		True if the bitfile is present and loads successfully; otherwise false.
	**/
	virtual bool						GetBitStream (NTV2_POINTER & outBitstream,
													  const ULWord inDesignID,
													  const ULWord inDesignVersion,
													  const ULWord inBitfileID,
													  const ULWord inBitfileVersion,
													  const ULWord inBitfileFlags);

private:

	/**
		@brief		Read the specified bitstream.
		@param[in]	inIndex		Specifies the index of the bitfile info.
		@return		True if the bitstream was read; otherwise false.
	**/
	bool ReadBitstream (const size_t inIndex);
		
	typedef std::vector <NTV2_POINTER>		NTV2BitstreamList;
	typedef NTV2BitstreamList::iterator		NTV2BitstreamListIter;

	NTV2BitfileInfoList		_bitfileList;
	NTV2BitstreamList		_bitstreamList;
};	//	CNTV2BitfileManager

#endif	//	NTV2BITMANAGER_H
