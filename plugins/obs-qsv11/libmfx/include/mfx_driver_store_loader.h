// Copyright (c) 2019-2020 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#if !defined(__MFX_DRIVER_STORE_LOADER_H)
#define __MFX_DRIVER_STORE_LOADER_H

#include <windows.h>
#include <cfgmgr32.h>
#include <devguid.h>

#include "mfx_dispatcher_defs.h"

namespace MFX
{

typedef CONFIGRET(WINAPI *Func_CM_Get_Device_ID_List_SizeW)(PULONG pulLen, PCWSTR pszFilter, ULONG ulFlags);
typedef CONFIGRET(WINAPI *Func_CM_Get_Device_ID_ListW)(PCWSTR pszFilter, PZZWSTR Buffer, ULONG BufferLen, ULONG ulFlags);
typedef CONFIGRET(WINAPI *Func_CM_Locate_DevNodeW)(PDEVINST pdnDevInst, DEVINSTID_W pDeviceID, ULONG ulFlags);
typedef CONFIGRET(WINAPI *Func_CM_Open_DevNode_Key)(DEVINST dnDevNode, REGSAM samDesired, ULONG ulHardwareProfile, REGDISPOSITION Disposition, PHKEY phkDevice, ULONG ulFlags);

class DriverStoreLoader
{
public:
    DriverStoreLoader(void);
    ~DriverStoreLoader(void);

    bool GetDriverStorePath(wchar_t *path, DWORD dwPathSize, mfxU32 deviceID);

protected:
    bool LoadCfgMgr();
    bool LoadCmFuncs();

    mfxModuleHandle                  m_moduleCfgMgr;
    Func_CM_Get_Device_ID_List_SizeW m_pCM_Get_Device_ID_List_Size;
    Func_CM_Get_Device_ID_ListW      m_pCM_Get_Device_ID_List;
    Func_CM_Locate_DevNodeW          m_pCM_Locate_DevNode;
    Func_CM_Open_DevNode_Key         m_pCM_Open_DevNode_Key;

private:
    // unimplemented by intent to make this class non-copyable
    DriverStoreLoader(const DriverStoreLoader &);
    void operator=(const DriverStoreLoader &);

};

} // namespace MFX

#endif // __MFX_DRIVER_STORE_LOADER_H
