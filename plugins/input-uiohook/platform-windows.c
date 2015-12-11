/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "platform.h"

#include <util/platform.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <Dwmapi.h>

const char* GetDefaultEventsSavePath()
{
	wchar_t path_utf16[MAX_PATH];
	char    path_utf8[MAX_PATH]  = {};

	SHGetFolderPathW(NULL, CSIDL_MYVIDEO, NULL, SHGFP_TYPE_CURRENT,
	                 path_utf16);

	os_wcs_to_utf8(path_utf16, wcslen(path_utf16), path_utf8, MAX_PATH);
	return path_utf8;
}
