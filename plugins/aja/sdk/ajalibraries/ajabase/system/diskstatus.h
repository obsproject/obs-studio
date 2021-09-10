/* SPDX-License-Identifier: MIT */
/**
	@file		diskstatus.h
	@brief		Declares the AJADiskStatus class.
	@copyright	(C) 2013-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_DISK_STATUS_H
#define AJA_DISK_STATUS_H

#include "ajabase/common/public.h"

typedef enum
{
	AJADiskStatusUnitTypeByte,
	AJADiskStatusUnitTypeKiloByte,
	AJADiskStatusUnitTypeMegaByte,
	AJADiskStatusUnitTypeGigaByte
} AJADiskStatusUnitType;

/** 
 *	System independent disk status.
 *	@ingroup AJAGroupSystem
 */
class AJA_EXPORT AJADiskStatus
{
public:

	AJADiskStatus();
	virtual ~AJADiskStatus();

	/**
	 *	Get the available free space and total drive space for path.
	 *
	 *	@param[in]	dirPath		Path to the volume to get space info about.
	 *	@param[out]	freeSpace	Available free space on volume in unitType units, is untouched if return is false.
	 *	@param[out]	totalSpace	Total space on volume in unitType units, is untouched if return is false.
	 *	@param[in]	unitType	Unit of measure for space values, B, KB, MB, GB
	 *	@return					true if successfully got space info for path, false for failure.
	 */
	static bool GetFreeAndTotalSpace(const char* dirPath, double& freeSpace, double& totalSpace, AJADiskStatusUnitType unitType = AJADiskStatusUnitTypeGigaByte);
};

#endif	//	AJA_DISK_STATUS_H
