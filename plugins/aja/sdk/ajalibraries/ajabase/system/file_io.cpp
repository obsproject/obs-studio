/* SPDX-License-Identifier: MIT */
/**
	@file		file_io.cpp
	@brief		Implements the AJAFileIO class based on platform, Windows/Posix.
	@copyright	(C) 2011-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ajabase/common/common.h"
#include "ajabase/system/file_io.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sstream>

#if defined(AJA_WINDOWS)
	// Windows includes
	#include <direct.h>
#else
	// Posix includes
	#include <fcntl.h>
	#include <dirent.h>
	#include <fnmatch.h>
	#include <limits.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <unistd.h>
#endif

using std::string;
using std::wstring;
using std::vector;

#if defined(AJA_WINDOWS)
	// Windows helper functions

	// found at: http://www.frenk.com/2009/12/convert-filetime-to-unix-timestamp/
	int64_t FileTime_to_POSIX(FILETIME ft)
	{
		// takes the last modified date
		LARGE_INTEGER date, adjust;
		date.HighPart = ft.dwHighDateTime;
		date.LowPart = ft.dwLowDateTime;

		// 100-nanoseconds = milliseconds * 10000
		adjust.QuadPart = 11644473600000 * 10000;

		// removes the diff between 1970 and 1601
		date.QuadPart -= adjust.QuadPart;

		// converts back from 100-nanoseconds to seconds
		return (int64_t)(date.QuadPart / 10000000);
	}
#else
	// Posix helper functions
	static string GetEnvVar(string const & key)
	{
		char * val = getenv( key.c_str() );
		return val == NULL ? string("") : string(val);
	}
#endif


AJAFileIO::AJAFileIO()
{
#if defined(AJA_WINDOWS)
	mFileDescriptor = INVALID_HANDLE_VALUE;
#else
	mpFile          = NULL;
#endif
}


AJAFileIO::~AJAFileIO()
{
	Close();
}


bool
AJAFileIO::FileExists(const std::wstring& fileName)
{
#if defined(AJA_WINDOWS)
	struct _stat dummy;
	return _wstat(fileName.c_str(), &dummy) != -1;
#else
	string aString;
	aja::wstring_to_string(fileName, aString);
	return FileExists(aString);
#endif
}


bool
AJAFileIO::FileExists(const std::string& fileName)
{
#if defined(AJA_WINDOWS)
	struct _stat dummy;
	return _stat(fileName.c_str(), &dummy) != -1;
#else
	struct stat dummy;
	bool bExists = stat(fileName.c_str(), &dummy) != -1;
	return bExists;
#endif
}


AJAStatus
AJAFileIO::Open(
	const std::wstring&	fileName,
	const int			flags,
	const int			properties)
{
#if defined(AJA_WINDOWS)
	DWORD     desiredAccess       = 0;
	DWORD     creationDisposition = 0;
	DWORD     flagsAndAttributes  = 0;
	DWORD	  shareMode			  = 0;
	AJAStatus status              = AJA_STATUS_FAIL;

	if ((INVALID_HANDLE_VALUE == mFileDescriptor) &&
		(0 != fileName.length()))
	{
		// If the flags are not compatable, we will let
		// Windows provide the error checking.
		if ((eAJAReadOnly & flags) || (eAJAReadWrite & flags))  //  O_RDONLY, O_RDWR
		{
			desiredAccess |= GENERIC_READ;
			shareMode = FILE_SHARE_READ;
		}
		if ((eAJAWriteOnly & flags) || (eAJAReadWrite & flags)) //  O_WRONLY
        {
			desiredAccess |= GENERIC_WRITE;
            shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
        }

		if (eAJACreateAlways & flags)
			creationDisposition |= CREATE_ALWAYS;     //  O_CREAT
		if (eAJACreateNew & flags)
			creationDisposition |= CREATE_NEW;        // (O_CREAT || O_EXCL)
		if (eAJATruncateExisting & flags)
			creationDisposition |= TRUNCATE_EXISTING; //  O_TRUNC
		if (eAJAReadOnly & flags)
			creationDisposition |= OPEN_EXISTING;

		if (eAJAUnbuffered & properties)
			flagsAndAttributes |= FILE_FLAG_NO_BUFFERING;
		
		mFileDescriptor = CreateFileW(
							fileName.c_str(),
							desiredAccess,
							shareMode, 
							NULL,
							creationDisposition,
							flagsAndAttributes,
							NULL);


		if (INVALID_HANDLE_VALUE != mFileDescriptor)
		{
			status = AJA_STATUS_SUCCESS;
		}
	}
	return status;
#else
	string aString;
	aja::wstring_to_string(fileName,aString);
	AJAStatus status = Open(aString,flags,properties);

	return status;
#endif
}


AJAStatus
AJAFileIO::Open(
	const std::string&	fileName,
	const int			flags,
	const int			properties)
{
#if defined(AJA_WINDOWS)
	wstring wString;
	aja::string_to_wstring(fileName,wString);
	AJAStatus status = Open(wString,flags,properties);

	return status;
#else
	AJAStatus status = AJA_STATUS_FAIL;
	string    flagsAndAttributes;

	if ((mpFile == NULL) && (0 != fileName.length()))
	{
		// If the flags are not compatable, we will let
		// Linux provide the error checking.
		if (eAJAReadOnly & flags)
		{
			flagsAndAttributes = "r";
		}
		else if (eAJAWriteOnly & flags)
		{
			if (eAJATruncateExisting & flags)
			{
				flagsAndAttributes = "w";
			}
			else
			{
				flagsAndAttributes = "w+";
			}
		}
		else if (eAJAReadWrite & flags)
		{
			if (eAJATruncateExisting & flags)
			{
				flagsAndAttributes = "w+";
			}
			else
			{
				if (eAJACreateAlways & flags)
				{
					flagsAndAttributes = "a+";
				}
				if (eAJACreateNew & flags)
				{
					flagsAndAttributes = "w+";
				}
			}
		}

		if (true == flagsAndAttributes.empty())
		{
			return AJA_STATUS_BAD_PARAM;
		}

		// One can also change the buffering behavior via:
		// setvbuf(FILE*, char* pBuffer, _IOFBF,  size_t size);
		//printf ("fopen name=%s attr=%s\n", fileName.c_str(), flagsAndAttributes.c_str());
		mpFile = fopen(fileName.c_str(), flagsAndAttributes.c_str());

		if (NULL != mpFile)
		{
			int fd = fileno(mpFile);
#if defined(AJA_MAC)			
			if (eAJANoCaching & properties)
			{
				fcntl(fd, F_NOCACHE, 1);
			}
#endif
			if (eAJAUnbuffered & properties)
			{
				if (-1 != fd)
				{
					status = AJA_STATUS_SUCCESS;
				}
			}
			else
			{
				status = AJA_STATUS_SUCCESS;
			}
		}
	}
	return status;
#endif
}


AJAStatus
AJAFileIO::Close()
{
#if defined(AJA_WINDOWS)
	AJAStatus status = AJA_STATUS_FAIL;

	if (INVALID_HANDLE_VALUE != mFileDescriptor)
	{
		if (TRUE == CloseHandle(mFileDescriptor))
		{
			status = AJA_STATUS_SUCCESS;
		}
		mFileDescriptor = INVALID_HANDLE_VALUE;
	}
	return status;
#else
	AJAStatus status = AJA_STATUS_FAIL;

	if (NULL != mpFile)
	{
		if (0 == fclose(mpFile))
		{
			status = AJA_STATUS_SUCCESS;
		}
		mpFile = NULL;
	}
	return status;
#endif
}


bool
AJAFileIO::IsOpen()
{
#if defined(AJA_WINDOWS)
	return (INVALID_HANDLE_VALUE != mFileDescriptor);
#else
	return (NULL != mpFile);
#endif
}


uint32_t
AJAFileIO::Read(uint8_t* pBuffer, const uint32_t length)
{
#if defined(AJA_WINDOWS)
	DWORD bytesRead = 0;

	if (INVALID_HANDLE_VALUE != mFileDescriptor)
	{
		ReadFile(mFileDescriptor, pBuffer, length, &bytesRead, NULL);
	}
	return bytesRead;
#else
	uint32_t retVal = 0;
	if (NULL != mpFile)
	{
		size_t bytesRead = 0;
		if ((bytesRead = fread(pBuffer, 1, length, mpFile)) > 0)
		{
			retVal = uint32_t(bytesRead);
		}
	}
	return retVal;
#endif
}


uint32_t
AJAFileIO::Read(std::string& buffer, const uint32_t length)
{
#if defined(AJA_WINDOWS)
	buffer.resize(length);
	uint32_t actual_bytes = Read((uint8_t*) &buffer[0], length);
	buffer.resize(actual_bytes);
	return actual_bytes;
#else
	buffer.resize(length);
	uint32_t actual_bytes = Read((uint8_t*) &buffer[0], length);
	buffer.resize(actual_bytes);
	return actual_bytes;
#endif
}


uint32_t
AJAFileIO::Write(const uint8_t* pBuffer, const uint32_t length) const
{
#if defined(AJA_WINDOWS)
	DWORD bytesWritten = 0;

	if (INVALID_HANDLE_VALUE != mFileDescriptor)
	{
		WriteFile(mFileDescriptor, pBuffer, length, &bytesWritten, NULL);
	}
	return bytesWritten;
#else
	uint32_t retVal = 0;
	if (NULL != mpFile)
	{
		size_t bytesWritten = 0;
		if ((bytesWritten = fwrite(pBuffer, 1, length, mpFile)) > 0)
		{
			retVal = uint32_t(bytesWritten);
		}
	}
	return retVal;
#endif
}


uint32_t
AJAFileIO::Write(const std::string& buffer) const
{
#if defined(AJA_WINDOWS)
	return Write((uint8_t*) buffer.c_str(), (uint32_t)buffer.length());
#else
	return Write((uint8_t*) buffer.c_str(), (uint32_t)buffer.length());
#endif
}


AJAStatus
AJAFileIO::Sync()
{
#if defined(AJA_WINDOWS)
	AJAStatus status = AJA_STATUS_FAIL;
	if (INVALID_HANDLE_VALUE != mFileDescriptor)
	{
		if (FlushFileBuffers(mFileDescriptor) != 0)
			status = AJA_STATUS_SUCCESS;
	}
	return status;
#else
	AJAStatus status = AJA_STATUS_FAIL;
	if (IsOpen())
	{
		int fd = fileno(mpFile);
		if (-1 != fd)
		{
			if (fsync(fd) == 0)
				status = AJA_STATUS_SUCCESS;
		}
	}
	return status;
#endif
}

AJAStatus
AJAFileIO::Truncate(int32_t size)
{
#if defined(AJA_WINDOWS)
	AJAStatus status = AJA_STATUS_FAIL;
	if (INVALID_HANDLE_VALUE != mFileDescriptor)
	{
		// save off current offset
		int64_t offset = Tell();
		// move file pointer to size from start of file
		status = Seek(size, eAJASeekSet);
		if (status == AJA_STATUS_SUCCESS)
		{
			// truncates file at current file pointer
			if (SetEndOfFile(mFileDescriptor) != 0)
			{
				status = AJA_STATUS_SUCCESS;
			}
			// set the offset back to original position (mimmic ftruncate)
			status = Seek(offset, eAJASeekSet);
		}
	}
	return status;
#else
	AJAStatus status = AJA_STATUS_FAIL;
	if (IsOpen())
	{
		int fd = fileno(mpFile);
		if (-1 != fd)
		{
			int res = ftruncate(fd, size);
			if (res == 0)
			{
				status = AJA_STATUS_SUCCESS;
			}
		}
	}
	return status;
#endif
}

int64_t
AJAFileIO::Tell()
{
#if defined(AJA_WINDOWS)
	int64_t retVal = 0;
	if (INVALID_HANDLE_VALUE != mFileDescriptor)
	{
		LARGE_INTEGER liDistanceToMove;
		liDistanceToMove.HighPart = 0;
		liDistanceToMove.LowPart = 0;

		LARGE_INTEGER liCurrentFilePointer;

		BOOL status = SetFilePointerEx(mFileDescriptor, liDistanceToMove, &liCurrentFilePointer, FILE_CURRENT);
		
		if (status == 0)
		{
			retVal = (int64_t)-1;
		}
		else
		{
			retVal = liCurrentFilePointer.QuadPart;
		}
	}
	return retVal;
#else
	int64_t retVal = 0;
	if (IsOpen())
	{
		retVal = (int64_t)ftello(mpFile);
	}
	return retVal;
#endif
}


AJAStatus
AJAFileIO::Seek(const int64_t distance, const AJAFileSetFlag flag) const
{
#if defined(AJA_WINDOWS)
	DWORD     moveMethod;
	DWORD     retVal;
	AJAStatus status = AJA_STATUS_FAIL;

	if (INVALID_HANDLE_VALUE != mFileDescriptor)
	{
		switch (flag)
		{
			case eAJASeekSet:
				moveMethod = FILE_BEGIN;
				break;

			case eAJASeekCurrent:
				moveMethod = FILE_CURRENT;
				break;

			case eAJASeekEnd:
				moveMethod = FILE_END;
				break;

			default:
				return (AJA_STATUS_BAD_PARAM);
		}
		LARGE_INTEGER liDistanceToMove;
		liDistanceToMove.HighPart = (LONG)(distance>>32);
		liDistanceToMove.LowPart = (DWORD)distance;

		retVal = SetFilePointerEx(mFileDescriptor, liDistanceToMove, NULL, moveMethod);
		if ( retVal	== TRUE )
		{
			status = AJA_STATUS_SUCCESS;
		}

	}
	return status;
#else
	AJAStatus status = AJA_STATUS_FAIL;
	int       whence;
	long int  retVal;

	if (NULL != mpFile)
	{
		switch (flag)
		{
			case eAJASeekSet:
				whence = SEEK_SET;
				break;

			case eAJASeekCurrent:
				whence = SEEK_CUR;
				break;

			case eAJASeekEnd:
				whence = SEEK_END;
				break;

			default:
				return (AJA_STATUS_BAD_PARAM);
		}
		retVal = fseeko(mpFile, (off_t)distance, whence);

		if (-1 != retVal)
		{
			status = AJA_STATUS_SUCCESS;
		}
	}
	return status;
#endif
}


AJAStatus
AJAFileIO::FileInfo(int64_t& createTime, int64_t& modTime, int64_t& size)
{
#if defined(AJA_WINDOWS)
	string filePath;
	return FileInfo(createTime, modTime, size, filePath);
#else
	string filePath;
	return FileInfo(createTime, modTime, size, filePath);
#endif
}

AJAStatus 
AJAFileIO::FileInfo(int64_t& createTime, int64_t& modTime, int64_t& size, std::string& filePath)
{
	createTime = modTime = size = 0;
	filePath = "";
#if defined(AJA_WINDOWS)
	AJAStatus status = AJA_STATUS_FAIL;
	
	createTime = modTime = size = 0;

	if(IsOpen())
	{
		FILETIME cTime;
		FILETIME aTime;
		FILETIME wTime;
		if( GetFileTime(mFileDescriptor,&cTime,&aTime,&wTime) )
		{
			LARGE_INTEGER sizeInfo;
			if(GetFileSizeEx(mFileDescriptor,&sizeInfo))
			{
				size = (int64_t)sizeInfo.QuadPart;
				createTime = FileTime_to_POSIX(cTime);
				modTime = FileTime_to_POSIX(wTime);

				const DWORD maxFilePathLen = 2048;
				filePath.resize(maxFilePathLen);
				DWORD retVal;
				retVal = GetFinalPathNameByHandleA(mFileDescriptor, &filePath[0], maxFilePathLen, FILE_NAME_NORMALIZED);
				if (retVal != 0)
				{
					status = AJA_STATUS_SUCCESS;
					filePath.resize(retVal);
				}
				else
				{
					status = AJA_STATUS_NOT_FOUND;
				}
			}
		}
	}
	return status;
#else
	AJAStatus status = AJA_STATUS_FAIL;

	if (IsOpen())
	{
		struct stat fileStatus;
		int fd = fileno(mpFile);
		int fErr = fstat(fd, &fileStatus);

		if (fErr == 0)
		{
			size = fileStatus.st_size;
			createTime = fileStatus.st_ctime;
			modTime = fileStatus.st_mtime;

#if defined(AJA_LINUX)
			// Linux way to get path of file descriptor
			ssize_t n = 0;
			if (fd != -1)
			{
				string procPath = "/proc/self/fd/" + aja::to_string(fd);
				filePath.resize(PATH_MAX);
				n = readlink(procPath.c_str(), &filePath[0], PATH_MAX);
				if (n < 0)
				{
					n = 0;
					status = AJA_STATUS_NOT_FOUND;
				}
				else
				{
					status = AJA_STATUS_SUCCESS;
				}
			}
			filePath.resize(n);
#elif defined(AJA_MAC)
			// Mac way to get path of file descriptor
			if (fd != -1)
			{
				filePath.resize(PATH_MAX);
				if (fcntl(fd, F_GETPATH, &filePath[0]) != -1)
				{
					status = AJA_STATUS_SUCCESS;
					filePath.resize(strlen(filePath.c_str()));
				}
				else
				{
					status = AJA_STATUS_NOT_FOUND;
				}
			}
#else
			#warning "'AJAFileIO::FileInfo' does not support path retreval."
			filePath = "";
			status = AJA_STATUS_SUCCESS;
#endif
		}
	}
	return status;
#endif
}


AJAStatus
AJAFileIO::Delete(const string& fileName)
{
#if defined(AJA_WINDOWS)
	AJAStatus status = AJA_STATUS_FAIL;

	if (0 != fileName.length())
	{
		if (DeleteFileA(fileName.c_str()))
		{
			status = AJA_STATUS_SUCCESS;
		}
	}
	return status;
#else
	AJAStatus status = AJA_STATUS_FAIL;

	if (0 != fileName.length())
	{
		if (0 == unlink(fileName.c_str()))
		{
			status = AJA_STATUS_SUCCESS;
		}
	}
	return status;
#endif
}


AJAStatus
AJAFileIO::Delete(const wstring& fileName)
{
#if defined(AJA_WINDOWS)
	AJAStatus status = AJA_STATUS_FAIL;

	if (0 != fileName.length())
	{
		if (DeleteFileW(fileName.c_str()))
		{
			status = AJA_STATUS_SUCCESS;
		}
	}
	return status;
#else
	AJAStatus status = AJA_STATUS_FAIL;

	string aString;
	aja::wstring_to_string(fileName,aString);
	status = Delete(aString);

	return status;
#endif
}


AJAStatus
AJAFileIO::ReadDirectory(
				const std::string&			directory,
				const std::string&			filePattern,
				std::vector<std::string>&	fileContainer)
{
#if defined(AJA_WINDOWS)
	WIN32_FIND_DATAA fileData;
	HANDLE           hSearch;
	string           qualifiedName;
	AJAStatus        status = AJA_STATUS_FAIL;

	fileContainer.clear();

	if ((0 != directory.length()) && (0 != filePattern.length()))
	{
		if (TRUE == SetCurrentDirectoryA(directory.c_str()))
		{
			if (INVALID_HANDLE_VALUE !=
				(hSearch = FindFirstFileA(filePattern.c_str(), &fileData)))
			{
				qualifiedName = directory + "/" + fileData.cFileName;
				fileContainer.push_back(qualifiedName);
	
				while (FindNextFileA(hSearch, &fileData) != 0)
				{
					qualifiedName = directory + "/" + fileData.cFileName;
					fileContainer.push_back(qualifiedName);
				}
				FindClose(hSearch);

				if (0 != fileContainer.size())
				{
					status = AJA_STATUS_SUCCESS;
				}
			}
		}
	}
	return status;
#else
	AJAStatus       status = AJA_STATUS_FAIL;
	struct dirent** ppNamelist;
	int             nEntries;
	string          fileEntry;
	string          convertedPath;
	string			upperPattern;
	char            resolvedPath[PATH_MAX];

	if ((0 != directory.length()) && (0 != filePattern.length()))
	{
		// Convert any Windows path chars to Linux
		convertedPath = directory;
		for (string::iterator it = convertedPath.begin();
				it < convertedPath.end();
				++it)
		{
			if( *it == '\\' )
				*it = '/';
		}

		// Force the pattern to upper case
		upperPattern = filePattern;
		aja::upper(upperPattern);

		// Make sure directory path is cleaned up
		if (!realpath(convertedPath.c_str(), resolvedPath))
			return status;  // Path is bad

		nEntries = scandir(resolvedPath, &ppNamelist, 0, alphasort);

		if (nEntries > 0)
		{
			for (int ndx = 0; ndx < nEntries; ndx++)
			{
				char* pName = ppNamelist[ndx]->d_name;

				// Make an upper case copy of the file name
				char upperName[PATH_MAX];
				char* pChar = pName;
				size_t length = strlen( pName );
				size_t i;
				for (i = 0; i < length; i++)
				{
					upperName[i] = toupper( *pChar++ );
				}
				upperName[i] = '\0';

				if (!fnmatch(upperPattern.c_str(), upperName, FNM_PERIOD))
				{
					fileEntry  = (directory + "/");
					fileEntry += pName;

					fileContainer.push_back(fileEntry);
				}
			}
			free(ppNamelist);
			status = AJA_STATUS_SUCCESS;
		}
	}
	return status;
#endif
}


AJAStatus
AJAFileIO::ReadDirectory(
				const std::wstring&			directory,
				const std::wstring&			filePattern,
				std::vector<std::wstring>&	fileContainer)
{
#if defined(AJA_WINDOWS)
	WIN32_FIND_DATAW fileData;
	HANDLE           hSearch;
	wstring          qualifiedName;
	AJAStatus        status = AJA_STATUS_FAIL;

	fileContainer.clear();

	if ((0 != directory.length()) && (0 != filePattern.length()))
	{
		if (TRUE == SetCurrentDirectoryW(directory.c_str()))
		{
			if (INVALID_HANDLE_VALUE !=
				(hSearch = FindFirstFileW(filePattern.c_str(), &fileData)))
			{
				qualifiedName = directory + L"/" + fileData.cFileName;
				fileContainer.push_back(qualifiedName);
	
				while (FindNextFileW(hSearch, &fileData) != 0)
				{
					qualifiedName = directory + L"/" + fileData.cFileName;
					fileContainer.push_back(qualifiedName);
				}
				FindClose(hSearch);

				if (0 != fileContainer.size())
				{
					status = AJA_STATUS_SUCCESS;
				}
			}
		}
	}
	return status;
#else
	AJAStatus status = AJA_STATUS_FAIL;

	string aDir,aPat;
	aja::wstring_to_string(directory,aDir);
	aja::wstring_to_string(filePattern,aPat);
	vector<string> aContainer;
	status = ReadDirectory(aDir,aPat,aContainer);
	for(vector<string>::iterator i = aContainer.begin(); i != aContainer.end(); ++i)
	{
		wstring tmp;
		aja::string_to_wstring(*i,tmp);
		fileContainer.push_back(tmp);
	}

	return status;
#endif
}


AJAStatus
AJAFileIO::DoesDirectoryContain(
				const std::string& directory,
				const std::string& filePattern)
{
#if defined(AJA_WINDOWS)
	WIN32_FIND_DATAA fileData;
	HANDLE           hSearch;
	char             savePath[MAX_PATH+1];
	AJAStatus        status = AJA_STATUS_FAIL;

	if ((0 != directory.length()) && (0 != filePattern.length()))
	{
		if( !GetCurrentDirectoryA(MAX_PATH, savePath) )
			return (status);

		if (TRUE == SetCurrentDirectoryA(directory.c_str()))
		{
			if (INVALID_HANDLE_VALUE !=
				(hSearch = FindFirstFileA(filePattern.c_str(), &fileData)))
			{
				FindClose(hSearch);

				status = AJA_STATUS_SUCCESS;
			}

			SetCurrentDirectoryA(savePath);
		}
	}
	return status;
#else
	AJAStatus       status = AJA_STATUS_FAIL;
	vector<string>  fileList;

	if ((0 != directory.length()) && (0 != filePattern.length()))
	{
		AJAStatus readStatus = ReadDirectory( directory, filePattern, fileList );
		if( readStatus == AJA_STATUS_SUCCESS )
		{
			if( fileList.size() >= 2 )	// Don't count "." and ".."
				status = AJA_STATUS_SUCCESS;
		}
	}
	return status;
#endif
}


AJAStatus
AJAFileIO::DoesDirectoryContain(
				const std::wstring& directory,
				const std::wstring& filePattern)
{
#if defined(AJA_WINDOWS)
	WIN32_FIND_DATAW fileData;
	HANDLE           hSearch;
	wchar_t          savePath[MAX_PATH+1];
	AJAStatus        status = AJA_STATUS_FAIL;

	if ((0 != directory.length()) && (0 != filePattern.length()))
	{
		if( !GetCurrentDirectoryW(MAX_PATH, savePath) )
			return (status);

		if (TRUE == SetCurrentDirectoryW(directory.c_str()))
		{
			if (INVALID_HANDLE_VALUE !=
				(hSearch = FindFirstFileW(filePattern.c_str(), &fileData)))
			{
				FindClose(hSearch);

				status = AJA_STATUS_SUCCESS;
			}

			SetCurrentDirectoryW(savePath);
		}
	}
	return (status);
#else
	AJAStatus status = AJA_STATUS_FAIL;
	string aDir,aPat;
	aja::wstring_to_string(directory,aDir);
	aja::wstring_to_string(filePattern,aPat);
	status = DoesDirectoryContain(aDir,aPat);

	return status;
#endif
}


AJAStatus
AJAFileIO::DoesDirectoryExist(const std::string& directory)
{
#if defined(AJA_WINDOWS)
	return( (::GetFileAttributesA(directory.c_str()) != INVALID_FILE_ATTRIBUTES) ? AJA_STATUS_SUCCESS : AJA_STATUS_FAIL);
#else
	AJAStatus status = AJA_STATUS_FAIL;

	if (0 != directory.length())
	{
		DIR* pDir = opendir( directory.c_str() );
		if( pDir )
		{
			closedir( pDir );
			status = AJA_STATUS_SUCCESS;
		}
	}
	return status;
#endif
}


AJAStatus
AJAFileIO::DoesDirectoryExist(const std::wstring& directory)
{
#if defined(AJA_WINDOWS)
	return( (::GetFileAttributesW(directory.c_str()) != INVALID_FILE_ATTRIBUTES) ? AJA_STATUS_SUCCESS : AJA_STATUS_FAIL);
#else
	AJAStatus status = AJA_STATUS_FAIL;
	string aDir;
	aja::wstring_to_string(directory,aDir);
	status = DoesDirectoryExist(aDir);

	return status;
#endif
}


AJAStatus
AJAFileIO::IsDirectoryEmpty(const std::string& directory)
{
#if defined(AJA_WINDOWS)
	AJAStatus status = AJA_STATUS_FAIL;
	if (DoesDirectoryContain(directory, "*") != AJA_STATUS_SUCCESS)
		status = AJA_STATUS_SUCCESS;
	return status;
#else
	AJAStatus status = AJA_STATUS_FAIL;
	if (DoesDirectoryContain(directory, "*") != AJA_STATUS_SUCCESS)
		status = AJA_STATUS_SUCCESS;
	return status;
#endif
}


AJAStatus
AJAFileIO::IsDirectoryEmpty(const std::wstring& directory)
{
#if defined(AJA_WINDOWS)
	AJAStatus status = AJA_STATUS_FAIL;
	if (DoesDirectoryContain(directory, L"*") != AJA_STATUS_SUCCESS)
		status = AJA_STATUS_SUCCESS;
	return status;
#else
	AJAStatus status = AJA_STATUS_FAIL;
	if (DoesDirectoryContain(directory, L"*") != AJA_STATUS_SUCCESS)
		status = AJA_STATUS_SUCCESS;
	return status;
#endif
}


AJAStatus
AJAFileIO::TempDirectory(std::string& directory)
{
#if defined(AJA_WINDOWS)
	AJAStatus status = AJA_STATUS_FAIL;
	directory = "";
	vector<char> temp;
	temp.resize(MAX_PATH);
	if (GetTempPathA(MAX_PATH, &temp[0]))
	{
		if (AJAFileIO::FileExists(&temp[0]))
		{
			directory = &temp[0];
			status = AJA_STATUS_SUCCESS;
		}
		else
		{
			status = AJA_STATUS_NOT_FOUND;
		}
	}
	return status;
#else
	AJAStatus status = AJA_STATUS_FAIL;
	directory = "";
	vector<string> environ_vars;
	environ_vars.push_back("TMPDIR");
	environ_vars.push_back("TMP");
	environ_vars.push_back("TEMP");
	environ_vars.push_back("TEMPDIR");
	vector<string>::iterator it;
	string temp;
	for(it=environ_vars.begin() ; it != environ_vars.end() ; ++it)
	{
		temp = GetEnvVar(*it);
		if (!temp.empty() && AJAFileIO::FileExists(temp))
		{
			directory = temp;
			status = AJA_STATUS_SUCCESS;
			break;
		}
	}

	// last ditch effort
	if (status != AJA_STATUS_SUCCESS)
	{
		temp = "/tmp";
		if (AJAFileIO::FileExists(temp))
		{
			directory = temp;
			status = AJA_STATUS_SUCCESS;
		}
		else
		{
			status = AJA_STATUS_NOT_FOUND;
		}
	}
	return status;
#endif
}


AJAStatus
AJAFileIO::TempDirectory(std::wstring& directory)
{
#if defined(AJA_WINDOWS)
	AJAStatus status = AJA_STATUS_FAIL;
	directory = L"";
	vector<wchar_t> temp;
	temp.resize(MAX_PATH);
	if (GetTempPathW(MAX_PATH, &temp[0]))
	{
		if (AJAFileIO::FileExists(&temp[0]))
		{
			directory = &temp[0];
			status = AJA_STATUS_SUCCESS;
		}
		else
		{
			status = AJA_STATUS_NOT_FOUND;
		}
	}
	return status;
#else
	string temp;
	AJAStatus status = AJAFileIO::TempDirectory(temp);
	if (status == AJA_STATUS_SUCCESS)
	{
		aja::string_to_wstring(temp, directory);
	}
	else
	{
		directory = L"";
	}
	return status;
#endif
}

AJAStatus
AJAFileIO::GetWorkingDirectory(std::string& cwd)
{
	char buf[AJA_MAX_PATH+1] = "";

#if defined(AJA_WINDOWS)
	_getcwd(buf, AJA_MAX_PATH);
#else
	getcwd(buf, AJA_MAX_PATH);
#endif

	cwd = std::string(buf);
	return AJA_STATUS_SUCCESS;
}

AJAStatus
AJAFileIO::GetWorkingDirectory(std::wstring& directory)
{
	bool ok = false;
	std::string buf;
	if (GetWorkingDirectory(buf) == AJA_STATUS_SUCCESS)
		ok = aja::string_to_wstring(buf, directory);
	else
		directory = L"";

	return ok ? AJA_STATUS_SUCCESS : AJA_STATUS_FAIL;
}

AJAStatus
AJAFileIO::GetDirectoryName(const std::string& path, std::string& directory)
{
	const size_t lastSlashIndex = path.rfind(AJA_PATHSEP);

	directory = "";

	if (std::string::npos != lastSlashIndex) {
		directory = path.substr(0, lastSlashIndex);
		return AJA_STATUS_SUCCESS;
	}

	return AJA_STATUS_NOT_FOUND;
}

AJAStatus
AJAFileIO::GetDirectoryName(const std::wstring& path, std::wstring& directory)
{
	const size_t lastSlashIndex = path.rfind(AJA_PATHSEP_WIDE);

	directory = L"";

	if (std::wstring::npos != lastSlashIndex) {
		directory = path.substr(0, lastSlashIndex);
		return AJA_STATUS_SUCCESS;
	}

	return AJA_STATUS_NOT_FOUND;
}

AJAStatus
AJAFileIO::GetFileName(const std::string& path, std::string& filename)
{
	const size_t lastSlashIndex = path.rfind(AJA_PATHSEP);

	filename = "";

	if (std::string::npos != lastSlashIndex) {
		filename = path.substr(lastSlashIndex + 1, path.length() - lastSlashIndex);
		return AJA_STATUS_SUCCESS;
	}

	return AJA_STATUS_NOT_FOUND;
}

AJAStatus
AJAFileIO::GetFileName(const std::wstring& path, std::wstring& filename)
{
	const size_t lastSlashIndex = path.rfind(AJA_PATHSEP_WIDE);

	filename = L"";

	if (std::wstring::npos != lastSlashIndex) {
		filename = path.substr(lastSlashIndex + 1, path.length() - lastSlashIndex);
		return AJA_STATUS_SUCCESS;
	}

	return AJA_STATUS_NOT_FOUND;
}
