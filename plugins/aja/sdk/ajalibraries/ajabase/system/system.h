/* SPDX-License-Identifier: MIT */
/**
    @file		system.h
    @brief		System specific functions
    @copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_SYSTEM_H
#define AJA_SYSTEM_H

#include "ajabase/common/public.h"

#if defined(AJA_WINDOWS)
	#pragma warning (disable:4996)
	#if !defined(_WIN32_WINNT)
        #define _WIN32_WINNT 0x0600
	#endif
	#include <WinSock2.h>	//	NOTE:	This must be included BEFORE <Windows.h> to avoid "macro redefinition" errors.
    #include <Windows.h>	//			http://www.zachburlingame.com/2011/05/resolving-redefinition-errors-betwen-ws2def-h-and-winsock-h/
	#include <stdio.h>
	#include <tchar.h>
	#include <winioctl.h>
    #include <SetupAPI.h>
	#include <initguid.h>

    namespace aja
    {
        AJA_EXPORT bool write_registry_string(HKEY hkey, std::string key_path, std::string key, std::string value);

        AJA_EXPORT bool write_registry_dword(HKEY hkey, std::string key_path, std::string key, DWORD value);

        AJA_EXPORT std::string read_registry_string(HKEY hkey, std::string key_path, std::string key);

        AJA_EXPORT DWORD read_registry_dword(HKEY hkey, std::string key_path, std::string key);

    } //end aja namespace

#endif

#if defined(AJA_MAC)
	#include <Carbon/Carbon.h>
	#include <CoreServices/CoreServices.h>
#endif

#if defined(AJA_LINUX)

#endif

// Common to all
namespace aja
{
    AJA_EXPORT int reveal_file_in_file_manager(const std::string& filePath);
}

#endif
