#pragma once

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>

#include "winrt\Windows.Foundation.h"
#include "winrt\Windows.ApplicationModel.h"

#include <wil\cppwinrt.h> // must be before the first C++ WinRT header, ref:https://github.com/Microsoft/wil/wiki/Error-handling-helpers
#include <wil\result.h>
#include <wil\com.h>

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
