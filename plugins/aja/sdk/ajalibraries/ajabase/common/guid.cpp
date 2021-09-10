/* SPDX-License-Identifier: MIT */
/**
	@file		guid.cpp
	@brief		Generates a new, unique UUID as an STL string.
	@copyright	(C) 2015-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#include "guid.h"

std::string CreateGuid (void)
{
	#if defined (AJA_WINDOWS)
		UUID	uuid;
		UuidCreate (&uuid);

		unsigned char *	str	(NULL);
		UuidToStringA (&uuid, &str);

		std::string	result (reinterpret_cast <char *> (str));
		RpcStringFreeA (&str);
	#elif defined (AJA_LINUX)
		#include <stdio.h>
		#define GUID_LENGTH 36

		std::string result ("");

		FILE *fp = fopen ("/proc/sys/kernel/random/uuid", "r");
		if (fp)
		{
			char guid[GUID_LENGTH + 1];

			size_t readSize = fread (guid, 1, GUID_LENGTH, fp);
			if (readSize == GUID_LENGTH)
			{
				guid [GUID_LENGTH] = '\0';
				result = std::string((const char *) guid);
			}
			fclose (fp);
		}
	#else
		uuid_t	uuid;
		uuid_generate_random (uuid);
		char	result [37];
		uuid_unparse (uuid, result);
	#endif
    return result;
}
