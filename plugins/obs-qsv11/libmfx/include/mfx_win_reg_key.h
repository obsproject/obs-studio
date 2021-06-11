// Copyright (c) 2012-2019 Intel Corporation
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

#if !defined(__MFX_WIN_REG_KEY_H)
#define __MFX_WIN_REG_KEY_H

#include <windows.h>
#include "mfxplugin.h"
#include "mfx_dispatcher_log.h"

#if !defined(MEDIASDK_UWP_DISPATCHER)
namespace MFX {

template<class T> struct RegKey{};
template<> struct RegKey<bool>{enum {type = REG_DWORD};};
template<> struct RegKey<mfxU32>{enum {type = REG_DWORD};};
template<> struct RegKey<mfxPluginUID>{enum {type = REG_BINARY};};
template<> struct RegKey<mfxVersion>{enum {type = REG_DWORD};};
template<> struct RegKey<char*>{enum {type = REG_SZ};};
template<> struct RegKey<wchar_t*>{enum {type = REG_SZ};};


class WinRegKey
{
public:
    // Default constructor
    WinRegKey(void);
    // Destructor
    ~WinRegKey(void);

    // Open a registry key
    bool Open(HKEY hRootKey, const wchar_t *pSubKey, REGSAM samDesired);
    bool Open(WinRegKey &rootKey, const wchar_t *pSubKey, REGSAM samDesired);

    // Query value
    bool QueryInfo(LPDWORD lpcSubkeys);

    bool QueryValueSize(const wchar_t *pValueName, DWORD type, LPDWORD pcbData);
    bool Query(const wchar_t *pValueName, DWORD type, LPBYTE pData, LPDWORD pcbData);

    bool Query(const wchar_t *pValueName, wchar_t *pData, mfxU32 &nData) {
        DWORD dw = (DWORD)nData;
        if (!Query(pValueName, RegKey<wchar_t*>::type, (LPBYTE)pData, &dw)){
            return false;
        }
        nData = dw;
        return true;
    }

    // Enumerate value names
    bool EnumValue(DWORD index, wchar_t *pValueName, LPDWORD pcchValueName, LPDWORD pType);
    bool EnumKey(DWORD index, wchar_t *pValueName, LPDWORD pcchValueName);

protected:

    // Release the object
    void Release(void);

    HKEY m_hKey;                                                // (HKEY) handle to the opened key

private:
    // unimplemented by intent to make this class non-copyable
    WinRegKey(const WinRegKey &);
    void operator=(const WinRegKey &);

};


template<class T>
inline bool QueryKey(WinRegKey & key, const wchar_t *pValueName, T &data ) {
    DWORD size = sizeof(data);
    return key.Query(pValueName, RegKey<T>::type, (LPBYTE) &data, &size);
}

template<>
inline bool QueryKey<bool>(WinRegKey & key, const wchar_t *pValueName, bool &data ) {
    mfxU32 value = 0;
    bool bRes = QueryKey(key, pValueName, value);
    data = (1 == value);
    return bRes;
}


} // namespace MFX
#endif // #if !defined(MEDIASDK_UWP_DISPATCHER)

#endif // __MFX_WIN_REG_KEY_H
