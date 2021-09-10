/* SPDX-License-Identifier: MIT */
/**
	@file		file_io.h
	@brief		Declares the AJAFileIO class.
	@copyright	(C) 2011-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_FILE_IO_H
#define AJA_FILE_IO_H


#include "ajabase/common/types.h"
#include "ajabase/common/public.h"
#include "ajabase/system/system.h"
#include <vector>
#include <string>

#if defined(AJA_WINDOWS)
	const char AJA_PATHSEP = '\\';
	const wchar_t AJA_PATHSEP_WIDE = L'\\';
#else
	const char AJA_PATHSEP = '/';
	const wchar_t AJA_PATHSEP_WIDE = L'/';
#endif

typedef enum
{
	eAJACreateAlways     = 1,
	eAJACreateNew        = 2,
	eAJATruncateExisting = 4,

	eAJAReadOnly         = 8,
	eAJAWriteOnly        = 16,
	eAJAReadWrite        = 32
} AJAFileCreationFlags;


typedef enum
{
	eAJABuffered		 = 1,
	eAJAUnbuffered		 = 2,
	eAJANoCaching		 = 4
} AJAFileProperties;


typedef enum
{
	eAJASeekSet,
	eAJASeekCurrent,
	eAJASeekEnd
} AJAFileSetFlag;


/**
 *	The File I/O class proper.
 *	@ingroup AJAGroupSystem
 */
class  AJA_EXPORT AJAFileIO
{
public:
	AJAFileIO();
	~AJAFileIO();

	/**
	 *	Open a file.
	 *
	 *	@param[in]	fileName			The fully qualified file name
	 *	@param[in]	flags				The way in which the file is opened
	 *	@param[in]	properties			Indicates whether the file is buffered or not
	 *
	 *	@return		AJA_STATUS_SUCCESS	A file has been successfully opened
	 *				AJA_STATUS_FAIL		A file could not be opened
	 */
	AJAStatus Open(
				const std::string &		fileName,
				const int				flags,
				const int				properties);

	AJAStatus Open(
				const std::wstring &	fileName,
				const int				flags,
				const int				properties);

	/**
	 *	Close a file.
	 *
	 *	@return		AJA_STATUS_SUCCESS	The file was successfully closed
	 *				AJA_STATUS_FAIL		The file could not be closed
	 */
	AJAStatus Close();

	/**
	 *	Tests for a valid open file.
	 *
	 *	@return		bool				'true' if a valid file is available
	 */
	bool IsOpen();

	/**
	 *	Read the contents of the file.
	 *
	 *	@param[out]	pBuffer				The buffer to be written to
	 *	@param[in]	length				The number of bytes to be read
	 *
	 *	@return		uint32_t			The number of bytes actually read
	 */
	uint32_t Read(uint8_t* pBuffer, const uint32_t length);

	/**
	 *	Read the contents of the file.
	 *
	 *	@param[out]	buffer				The buffer to be written to
	 *	@param[in]	length				The number of bytes to be read
	 *
	 *	@return		uint32_t			The number of bytes actually read
	 */
	uint32_t Read(std::string& buffer, const uint32_t length);
	
	/**
	 *	Write the contents of the file.
	 *
	 *	@param[in]	pBuffer				The buffer to be written out
	 *	@param[in]	length				The number of bytes to be written
	 *
	 *	@return		uint32_t			The number of bytes actually written
	 */	
	uint32_t Write(const uint8_t* pBuffer, const uint32_t length) const;

	/**
	 *	Write the contents of the file.
	 *
	 *	@param[in]	buffer				The buffer to be written out
	 *
	 *	@return		uint32_t			The number of bytes actually written
	 */
	uint32_t Write(const std::string& buffer) const;

	/**
	 *	Flush the cache 
	 *
	 *	@return		AJA_STATUS_SUCCESS	Was able to sync file
	 */	
	AJAStatus Sync();

	/**
	 *	Truncates the file.
	 *
	 *	@param[in]	offset			The size offset of the file
	 *
	 *	@return		AJA_STATUS_SUCCESS	Was able to truncate file
	 */
	AJAStatus Truncate(int32_t offset);

	/**
	 *	Retrieves the offset of the file pointer from the start of a file.
	 *
	 *	@return		int64_t			The position of the file pointer, -1 if error
	 */
	int64_t Tell();

	/**
	 *	Moves the offset of the file pointer.
	 *
	 *	@param[in]	distance			The distance to move the file pointer
	 *	@param[in]	flag				Describes from whence to move the file pointer
	 *
	 *	@return		AJA_STATUS_SUCCESS	The position of the file pointer was moved
	 */
	AJAStatus Seek(const int64_t distance, const AJAFileSetFlag flag) const;

	/**
	 *	Get some basic file info
	 *
	 *	@param[out]	createTime			Time of file creation, measured in seconds since 1970
	 *	@param[out]	modTime				Last time file was modified, measured in seconds since 1970
	 *	@param[out]	size				Size of the file in bytes
	 *
	 *	@return		AJA_STATUS_SUCCESS	Was able to get info from the file
	 */
	AJAStatus FileInfo(int64_t& createTime, int64_t& modTime, int64_t& size);
	AJAStatus FileInfo(int64_t& createTime, int64_t& modTime, int64_t& size, std::string& filePath);

    /**
     *	Test file to see if it exists
     *
     *	@param[in]	fileName			The fully qualified file name
     *
     *	@return		bool				true if file exists
     */
    static bool FileExists(const std::wstring& fileName);
    static bool FileExists(const std::string& fileName);

	/**
	 *	Remove the file for the system
	 *
	 *	@param[in]	fileName			The fully qualified file name
	 *
	 *	@return		AJA_STATUS_SUCCESS	The file was successfully deleteed
	 *				AJA_STATUS_FAIL		The file could not be deleted
	 */
    static AJAStatus Delete(const std::string& fileName);
    static AJAStatus Delete(const std::wstring& fileName);

	/**
	 *	Retrieves a set of files from a directory.
	 *	Changes the current directory.
	 *
	 *	@param[in]	directory			The path to the directory
	 *	@param[in]	filePattern			The pattern within the directory to match
	 *	@param[out]	fileContainer		The files that match the file pattern
	 *
	 *	@return		AJA_STATUS_SUCCESS	The returned container has a size > 0 
	 */
    static AJAStatus ReadDirectory(
				const std::string&   directory,
				const std::string&   filePattern,
				std::vector<std::string>& fileContainer);

    static AJAStatus ReadDirectory(
				const std::wstring&   directory,
				const std::wstring&   filePattern,
				std::vector<std::wstring>& fileContainer);

	/**
	 *	Tests if a directory contains a file that matches the pattern.
	 *	Does not change the current directory.
	 *
	 *	@param[in]	directory			The path to the directory
	 *	@param[in]	filePattern			The pattern within the directory to match
	 *
	 *	@return		AJA_STATUS_SUCCESS	If the directory has at least one matching file
	 */
    static AJAStatus DoesDirectoryContain(
				const std::string& directory,
				const std::string& filePattern);

    static AJAStatus DoesDirectoryContain(
				const std::wstring& directory,
				const std::wstring& filePattern);

	/**
	 *	Tests if a directory exists.
	 *	Does not change the current directory.
	 *
	 *	@param[in]	directory			The path to the directory
	 *
	 *	@return		AJA_STATUS_SUCCESS	If and only if the directory exists
	 */
    static AJAStatus DoesDirectoryExist(const std::string& directory);
    static AJAStatus DoesDirectoryExist(const std::wstring& directory);

	/**
	 *	Tests if a directory is empty.
	 *	Does not change the current directory.
	 *
	 *	@param[in]	directory			The path to the directory
	 *
	 *	@return		AJA_STATUS_SUCCESS	If and only if the directory contains no files
	 */
    static AJAStatus IsDirectoryEmpty(const std::string& directory);
    static AJAStatus IsDirectoryEmpty(const std::wstring& directory);

	/**
	 *	Retrieves a path to the temp directory
	 *
	 *	@param[out]	directory	The path to the temp directory
	 *
	 *	@return		AJA_STATUS_SUCCESS	If and only if a temp directory found
	 */
	static AJAStatus TempDirectory(std::string& directory);		//	New in SDK 16.0
	static AJAStatus TempDirectory(std::wstring& directory);	//	New in SDK 16.0

	/**
	 * Retrieves the path to the current working directory
	 * 
	 * @param[out]	directory	Path of the current working directory
	 * 
	 * @return		AJA_STATUS_SUCCESS If and only if current working directory retrieved.
	 */
	static AJAStatus GetWorkingDirectory(std::string& directory);	//	New in SDK 16.0
	static AJAStatus GetWorkingDirectory(std::wstring& directory);	//	New in SDK 16.0

	/**
	 * Retrieves the directory name from the specified path.
	 *
	 * @param[in] 	path	Path from which to extract the directory name
	 * 
	 * @param[out]  directory	Path of the directory extracted from specified path
	 * 
	 * @return		AJA_STATUS_SUCCESS If and only if the directory name is extracted
	 */
	static AJAStatus GetDirectoryName(const std::string& path, std::string& directory);		//	New in SDK 16.0
	static AJAStatus GetDirectoryName(const std::wstring& path, std::wstring& directory);	//	New in SDK 16.0

	/**
	 * Retrieves the filename (with extension) from the specified path.
	 *
	 * @param[in] 	path	Path from which to extract the filename
	 * 
	 * @param[out]  filename	Filename extracted from specified path
	 * 
	 * @return		AJA_STATUS_SUCCESS If and only if the filename is extracted
	 */
	static AJAStatus GetFileName(const std::string& path, std::string& filename);	//	New in SDK 16.0
	static AJAStatus GetFileName(const std::wstring& path, std::wstring& filename);	//	New in SDK 16.0

#if defined(AJA_WINDOWS)
	void     *GetHandle(void) {return mFileDescriptor;}
#else
	void     *GetHandle(void) {return NULL;}
#endif

private:

#if defined(AJA_WINDOWS)
    HANDLE       mFileDescriptor;
#else
	FILE*        mpFile;
#endif
};

#endif // AJA_FILE_IO_H
