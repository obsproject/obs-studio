/*

This is provided under a GPLv2 license.  When using or
redistributing this, you may do so under either license.

GPL LICENSE SUMMARY

Copyright(c) 2025 Intel Corporation.

This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

Contact Information:

Thy-Lan Gale, thy-lan.gale@intel.com
5000 W Chandler Blvd, Chandler, AZ 85226

*/

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#include "winrt/Windows.Foundation.h"
#include "winrt/Windows.ApplicationModel.h"

#include <wil/cppwinrt.h> // must be before the first C++ WinRT header, ref:https://github.com/Microsoft/wil/wiki/Error-handling-helpers
#include <wil/result.h>
#include <wil/com.h>

#include <Ks.h>
#include <ksproxy.h>
#include <ksmedia.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfobjects.h>
#include <mferror.h>
#include <mfreadwrite.h>

#include <d3d11.h>
#include <mfidl.h>

#include <cfgmgr32.h>
#include <initguid.h>
