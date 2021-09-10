/* SPDX-License-Identifier: MIT */
/**
	@file		dllmain.cpp
	@brief		Implementation of Windows DLL entry.
	@copyright	(C) 2008-2021 AJA Video Systems, Inc.
**/

#ifdef AJADLL_BUILD

#include "windows.h"

extern "C"
BOOL APIENTRY DllMain(HANDLE hModule, 
					  DWORD  ul_reason_for_call, 
					  LPVOID lpReserved)
{
	switch(ul_reason_for_call) 
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls((HMODULE)hModule);
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	default:
		break;
	}

	return TRUE;
}

#endif