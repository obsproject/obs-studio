/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2bitfile.h
	@brief		Declares the CNTV2Bitfile class.
	@copyright	(C) 2010-2021 AJA Video Systems, Inc.    All rights reserved.
**/

#ifndef NTV2BITFILE_H
#define NTV2BITFILE_H

#include <fstream>
#ifdef AJALinux
	#include <stdint.h>
	#include <stdlib.h>
#endif
#include "ajatypes.h"
#include "ajaexport.h"
#include "ntv2enums.h"
#include "ntv2publicinterface.h"
#include "ntv2utils.h"


/**
	@brief	Instances of me can parse a bitfile.
**/
class AJAExport CNTV2Bitfile
{
	public:
		/**
			@brief		My constructor.
		**/
											CNTV2Bitfile ();

		/**
			@brief		My destructor.
		**/
		virtual								~CNTV2Bitfile ();

		/**
			@brief		Opens the bitfile at the given path, then parses its header.
			@param[in]	inBitfilePath	Specifies the path name of the bitfile to be parsed.
			@return		True if open & parse succeeds; otherwise false.
		**/
		virtual bool						Open (const std::string & inBitfilePath);

		#if !defined (NTV2_DEPRECATE)
			virtual NTV2_DEPRECATED_f(bool	Open (const char * const & inBitfilePath));	///< @deprecated	Use the std::string version of Open instead.
		#endif	//	!defined (NTV2_DEPRECATE)

		/**
			@brief	Closes bitfile (if open) and resets me.
		**/
		virtual void						Close (void);

		/**
			@brief		Parse a bitfile header that's stored in a buffer.
			@param[in]	inBitfileBuffer	Specifies the buffer of the bitfile to be parsed.
			@param[in]	inBufferSize	Specifies the size of the buffer to be parsed.
			@return		A std::string containing parsing errors. It will be empty if successful.
		**/
		virtual std::string					ParseHeaderFromBuffer (const uint8_t* inBitfileBuffer, const size_t inBufferSize);

		/**
			@brief		Parse a bitfile header that's stored in a buffer.
			@param[in]	inBitfileBuffer	Specifies the buffer of the bitfile to be parsed.
			@return		A std::string containing parsing errors. It will be empty if successful.
		**/
		virtual std::string					ParseHeaderFromBuffer (const NTV2_POINTER & inBitfileBuffer);

		/**
			@return		A string containing the extracted bitfile build date.
		**/
		virtual inline const std::string &	GetDate (void) const			{ return _date; }

		/**
			@return		A string containing the extracted bitfile build time.
		**/
		virtual inline const std::string &	GetTime (void) const			{ return _time; }

		/**
			@return		A string containing the extracted bitfile design name.
		**/
		virtual inline const std::string &	GetDesignName (void) const		{ return _designName; }

		/**
			@return		A string containing the extracted bitfile part name.
		**/
		virtual inline const std::string &	GetPartName (void) const		{ return _partName; }

		/**
			@return		A string containing the error message, if any, from the last function that could fail.
		**/
		virtual inline const std::string &	GetLastError (void) const		{ return _lastError; }

		/**
			@return		True if the bitfile header includes tandem flag; otherwise false.
		**/
		virtual inline bool		IsTandem (void) const						{ return _tandem; }

		/**
			@return		True if the bitfile header includes partial flag; otherwise false.
		**/
		virtual inline bool		IsPartial (void) const						{ return _partial; }

		/**
			@return		True if the bitfile header includes clear flag; otherwise false.
		**/
		virtual inline bool		IsClear (void) const						{ return _clear; }

		/**
			@return		True if the bitfile header includes compress flag; otherwise false.
		**/
		virtual inline bool		IsCompress (void) const						{ return _compress; }

		/**
			@return		A ULWord containing the design design ID as extracted from the bitfile.
		**/
		virtual inline ULWord	GetDesignID (void) const					{ return _designID; }

		/**
			@return		A ULWord containing the design version as extracted from the bitfile.
		**/
		virtual inline ULWord	GetDesignVersion (void) const				{ return _designVersion; }

		/**
			@return		A ULWord containing the design ID as extracted from the bitfile.
		**/
		virtual inline ULWord	GetBitfileID (void) const					{ return _bitfileID; }

		/**
			@return		A ULWord containing the design version as extracted from the bitfile.
		**/
		virtual inline ULWord	GetBitfileVersion (void) const				{ return _bitfileVersion; }

		/**
			@brief		Answers with the design user ID, as extracted from the bitfile.
			@return		A ULWord containing the design user ID.
		**/
		virtual inline ULWord	GetUserID (void) const						{ return _userID; }

		/**
			@return		True if the bitfile can be flashed onto the device; otherwise false.
		**/
		virtual bool			CanFlashDevice (const NTV2DeviceID inDeviceID) const;

		/**
			@return		My instrinsic NTV2DeviceID.
		**/
		virtual NTV2DeviceID	GetDeviceID (void) const;

		/**
			@return		Program stream length in bytes, or zero if error/invalid.
		**/
		virtual inline size_t	GetProgramStreamLength (void) const		{return _fileReady ? _numBytes : 0;}

		/**
			@return		File stream length in bytes, or zero if error/invalid.
		**/
		virtual inline size_t	GetFileStreamLength (void) const		{return _fileReady ? _fileSize : 0;}
		
		/**
			@brief		Retrieves the program bitstream.
			@param[out]	outBuffer		Specifies the buffer that will receive the data.
			@return		Program stream length, in bytes, or zero upon failure.
		**/
		virtual size_t			GetProgramByteStream (NTV2_POINTER & outBuffer);

		/**
			@brief		Retrieves the file bitstream.
			@param[out]	outBuffer		Specifies the buffer that will receive the data.
			@return		File stream length, in bytes, or zero upon failure.
		**/
		virtual size_t			GetFileByteStream (NTV2_POINTER & outBuffer);

		//	Older, non-NTV2_POINTER-based functions:
		virtual size_t			GetProgramByteStream (unsigned char * pOutBuffer, const size_t inBufferLength);
		virtual size_t			GetFileByteStream (unsigned char * pOutBuffer, const size_t inBufferLength);

		// NO IMPLEMENTATION YET:	static NTV2StringList &	GetPartialDesignNames (const ULWord deviceID);
		static ULWord			GetDesignID			(const ULWord userID)		{ return (userID & 0xff000000) >> 24; }
		static ULWord			GetDesignVersion	(const ULWord userID)		{ return (userID & 0x00ff0000) >> 16; }
		static ULWord			GetBitfileID		(const ULWord userID)		{ return (userID & 0x0000ff00) >> 8; }
		static ULWord			GetBitfileVersion	(const ULWord userID)		{ return (userID & 0x000000ff) >> 0; }
		static NTV2DeviceID		ConvertToDeviceID	(const ULWord inDesignID, const ULWord inBitfileID);
		static ULWord			ConvertToDesignID	(const NTV2DeviceID inDeviceID);
		static ULWord			ConvertToBitfileID	(const NTV2DeviceID inDeviceID);

	private:	//	Private Member Functions
		virtual std::string		ParseHeader (void);
		virtual bool			SetDesignName (const std::string & inBuffer);
		virtual bool			SetDesignFlags (const std::string & inBuffer);
		virtual bool			SetDesignUserID (const std::string & inBuffer);

	private:	//	Private Member Data
		std::ifstream	_bitFileStream;
		UByteSequence	_fileHeader;
		int				_fileProgrammingPosition;
		std::string		_date;
		std::string		_time;
		std::string		_designName;
		std::string		_partName;
		std::string		_lastError;
		size_t			_numBytes;
		size_t			_fileSize;
		size_t			_programStreamPos;
		size_t			_fileStreamPos;
		bool			_fileReady;
		bool			_tandem;
		bool			_partial;
		bool			_clear;
		bool			_compress;
		ULWord			_userID;
		ULWord			_designID;
		ULWord			_designVersion;
		ULWord			_bitfileID;
		ULWord			_bitfileVersion;

};	//	CNTV2Bitfile

#endif // NTV2BITFILE_H
