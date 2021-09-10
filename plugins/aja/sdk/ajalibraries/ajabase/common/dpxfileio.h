/* SPDX-License-Identifier: MIT */
/**
	@file		dpxfileio.h
	@brief		Declaration of the AJADPXFileIO class, for low level file I/O.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_DPXFILEIO_H
#define AJA_DPXFILEIO_H

#include "ajabase/common/dpx_hdr.h"
#include "ajabase/common/types.h"

/**
 *	Class to support low level I/O for DPX files.
 *	@ingroup AJAFileIO
 */
class AJADPXFileIO : public DpxHdr
{
	//	Public Instance Methods
	public:
		/**
			@brief	Constructs a DPX file IO object.
			@note	Do not call this method without first calling the SetPath method.
		**/
		AJA_EXPORT AJADPXFileIO ();

		AJA_EXPORT virtual ~AJADPXFileIO ();

		/**
			@brief		Returns the list of DPX files that were found at the destination.
		**/
		AJA_EXPORT std::vector<std::string> &	GetFileList ();

		/**
			@brief		Returns the number of DPX files in the location specified by the path.
		**/
		AJA_EXPORT uint32_t						GetFileCount () const;

		/**
			@brief		Returns the index in the file list of the next file to be read.
		**/
		AJA_EXPORT uint32_t						GetIndex () const;

		/**
			@brief		Returns the current setting of the loop play control.
		**/
		AJA_EXPORT bool							GetLoopMode () const;

		/**
			@brief		Returns the current setting of the pause control.
		**/
		AJA_EXPORT bool							GetPauseMode () const;

		/**
			@brief		Returns the current path to the DPX files to be read.
		**/
		AJA_EXPORT std::string					GetPath () const;

		/**
			@brief		Read only the header from the specified file in the DPX sequence.
			@param[in]	inIndex		Specifies the index number of the file to read.
										0 <= outIndex <= FileCount
		**/
		AJA_EXPORT AJAStatus					Read (const uint32_t inIndex);

		/**
			@brief		Read the next file in the DPX sequence.
			@param[out]	outBuffer		Receives the DPX file image payload.
			@param[in]	inBufferSize	Specifies the maximum number of bytes to store in outBuffer.
			@param[out]	outIndex		Receives the index number of the file read.
										0 <= outIndex <= FileCount
		**/
		AJA_EXPORT AJAStatus				  Read (uint8_t  &		outBuffer,
											  const uint32_t	inBufferSize,
											  uint32_t &		outIndex);

		/**
			@brief		Specifies an ordered list of DPX files to read.
		**/
		void									SetFileList (std::vector<std::string> &	list);

		/**
			@brief		Specifies the index in the file list of the next file to be read.
		**/
		AJA_EXPORT AJAStatus					SetIndex (const uint32_t &	index);

		/**
			@brief		Specifies the setting of the loop play control.
						If true, the last file of a sequence is followed by the first file.
						If false, attempting to read beyond the end of a sequence is an error.
		**/
		AJA_EXPORT void							SetLoopMode (bool mode);

		/**
			@brief		Specifies the setting of the pause control.
						If true, pause mode will take effect.
						If false, pause mode is canceled.
		**/
		AJA_EXPORT void							SetPauseMode (bool mode);

		/**
			@brief		Change the path to the DPX files to be read.
			@param[in]	inPath			Specifies the path to where the DPX files are located.
										The file count will change to reflect the number of
										files present in the new location.
		**/
		AJA_EXPORT AJAStatus					SetPath (const std::string &	inPath);

		/**
			@brief		Write a DPX file.
			@param[in]	inBuffer		Specifies the DPX file image payload.
			#param[in]	inBufferSize	Specifies the number of payload bytes in inBuffer.
			#param[in]	inIndex			Specifies the index number to be appended to the file name.
		**/
		AJA_EXPORT AJAStatus					Write (const uint8_t  &	inBuffer,
											   const uint32_t	inBufferSize,
											   const uint32_t &	inIndex) const;


	// Protected Instance Methods
	protected:


	// Private Member Data
	private:
		bool						mPathSet;		/// True if the path to use has been set, else false
		bool						mLoopMode;		/// True if last sequence frame is followed by the first
		bool						mPauseMode;		/// True if currently paused, else false
		std::string					mPath;			/// Location of the files to read
		uint32_t					mFileCount;		/// Number of DPX files in the path
		uint32_t					mCurrentIndex;	/// Index into the vector below of the next file to read
		std::vector<std::string>	mFileList;		/// File names of all the DPX files in the path

};	//	AJADPXFileIO

#endif	// AJA_DPXFILEIO_H

