/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2utf8.h
	@brief		Declares the bare-bones UTF8 support functions (for the nub).
	@copyright	(C) 2008-2021 AJA Video Systems, Inc.
**/

#ifndef __NTV2UTF8_H
#define __NTV2UTF8_H

#include "ajatypes.h"	//	for NTV2_NUB_CLIENT_SUPPORT
#include "ajaexport.h"

#if defined (NTV2_NUB_CLIENT_SUPPORT)
	AJAExport bool map_utf8_to_codepage437(const char *src, int u8_len, unsigned char *cp437equiv);
	AJAExport void strncpyasutf8(char *dest, const char *src, int dest_buf_size);
	AJAExport void strncpyasutf8_map_cp437(char *dest, const char *src, int dest_buf_size);
#endif	//	defined (NTV2_NUB_CLIENT_SUPPORT)

#endif	//	__NTV2UTF8_H
