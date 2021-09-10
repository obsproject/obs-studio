/* SPDX-License-Identifier: MIT */
/**
	@file		diskstatus.cpp
	@brief		Implements the AJADiskStatus class.
	@copyright	(C) 2013-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ajabase/common/common.h"
#include "ajabase/system/lock.h"
#include "ajabase/system/system.h"

#include "ajabase/system/diskstatus.h"

#if defined(AJA_MAC)
	#include <sys/mount.h>
	#include <sys/stat.h>
#endif

#if defined(AJA_WINDOWS)
	#include <windows.h>
#endif

#if defined(AJA_LINUX)
	#include <sys/stat.h>
	#include <sys/vfs.h>
#endif 

AJADiskStatus::AJADiskStatus()
{
}


AJADiskStatus::~AJADiskStatus()
{
}

bool 
AJADiskStatus::GetFreeAndTotalSpace(const char* dirPath, double& freeSpace, double& totalSpace, AJADiskStatusUnitType unitType)
{
#if defined(AJA_MAC)
	// Use these numbers to match the Mac Finder
	const double b_size  = 1;
	const double kb_size = 1000;
	const double mb_size = 1000000;
	const double gb_size = 1000000000;
#else
	const double b_size  = 1;
	const double kb_size = 1024;
	const double mb_size = 1048576;
	const double gb_size = 1073741824;
#endif 

	double unitConv = 1;
	switch(unitType)
	{
		case AJADiskStatusUnitTypeByte:		unitConv = b_size;  break;
		case AJADiskStatusUnitTypeKiloByte: unitConv = kb_size; break;
		case AJADiskStatusUnitTypeMegaByte: unitConv = mb_size; break;
		
		default:
		case AJADiskStatusUnitTypeGigaByte: unitConv = gb_size; break;
	}

#if defined(AJA_WINDOWS)
	static AJALock changingCurrentDirLock;
	ULARGE_INTEGER free,total;
	//because this changes the working directory temporarily
	//use an auto lock to serialize access
	{
		AJAAutoLock	al(&changingCurrentDirLock);

		//save current dir
		char origDir[MAX_PATH];
		DWORD dwRet;
		dwRet = GetCurrentDirectoryA(MAX_PATH,origDir);

		//set target dir
		bool bSetResult = SetCurrentDirectoryA(dirPath);

		//get size info		
		bool bGetSpaceResult = ::GetDiskFreeSpaceExA( 0 , &free , &total , NULL );

		//restore original dir
		if (dwRet != 0)
		{
			bool bRet = SetCurrentDirectoryA(origDir);
		}

		if ( !bGetSpaceResult ) 
		{
			return false;
		}
	}

	freeSpace = static_cast<double>( static_cast<__int64>(free.QuadPart) ) / unitConv;
	totalSpace = static_cast<double>( static_cast<__int64>(total.QuadPart) ) / unitConv;

#else

	struct stat stst;
	struct statfs stfs;

	if ( ::stat(dirPath,&stst) == -1 ) return false;
	if ( ::statfs(dirPath,&stfs) == -1 ) return false;

	freeSpace = stfs.f_bavail * ( stst.st_blksize / unitConv );
	totalSpace = stfs.f_blocks * ( stst.st_blksize / unitConv );

#endif

	return true;
}
