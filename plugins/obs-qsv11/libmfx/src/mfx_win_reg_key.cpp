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

#if !defined(MEDIASDK_UWP_DISPATCHER)
#include "mfx_win_reg_key.h"
#include "mfx_dispatcher_log.h"

#define TRACE_WINREG_ERROR(str, ...) DISPATCHER_LOG_ERROR((("[WINREG]: " str), __VA_ARGS__))

namespace MFX
{

WinRegKey::WinRegKey(void)
{
    m_hKey = (HKEY) 0;

} // WinRegKey::WinRegKey(void)

WinRegKey::~WinRegKey(void)
{
    Release();

} // WinRegKey::~WinRegKey(void)

void WinRegKey::Release(void)
{
    // close the opened key
    if (m_hKey)
    {
        RegCloseKey(m_hKey);
    }

    m_hKey = (HKEY) 0;

} // void WinRegKey::Release(void)

bool WinRegKey::Open(HKEY hRootKey, const wchar_t *pSubKey, REGSAM samDesired)
{
    LONG lRes;
    HKEY hTemp;

    //
    // All operation are performed in this order by intention.
    // It makes possible to reopen the keys, using itself as a base.
    //

    // try to the open registry key
    lRes = RegOpenKeyExW(hRootKey, pSubKey, 0, samDesired, &hTemp);
    if (ERROR_SUCCESS != lRes)
    {
        DISPATCHER_LOG_OPERATION(SetLastError(lRes));
        TRACE_WINREG_ERROR("Opening key \"%s\\%S\" : RegOpenKeyExW()==0x%x\n"
            , (HKEY_LOCAL_MACHINE == hRootKey) ? ("HKEY_LOCAL_MACHINE") 
            : (HKEY_CURRENT_USER == hRootKey)  ? ("HKEY_CURRENT_USER") 
            : "UNSUPPORTED_KEY", pSubKey, GetLastError());
        return false;
    }

    // release the object before initialization
    Release();

    // save the handle
    m_hKey = hTemp;

    return true;

} // bool WinRegKey::Open(HKEY hRootKey, const wchar_t *pSubKey, REGSAM samDesired)

bool WinRegKey::Open(WinRegKey &rootKey, const wchar_t *pSubKey, REGSAM samDesired)
{
    return Open(rootKey.m_hKey, pSubKey, samDesired);

} // bool WinRegKey::Open(WinRegKey &rootKey, const wchar_t *pSubKey, REGSAM samDesired)

bool WinRegKey::QueryValueSize(const wchar_t *pValueName, DWORD type, LPDWORD pcbData) {
    DWORD keyType = type;
    LONG lRes;

    // query the value
    lRes = RegQueryValueExW(m_hKey, pValueName, NULL, &keyType, 0, pcbData);
    if (ERROR_SUCCESS != lRes)
    {
        DISPATCHER_LOG_OPERATION(SetLastError(lRes));
        TRACE_WINREG_ERROR("Querying \"%S\" : RegQueryValueExA()==0x%x\n", pValueName, GetLastError());
        return false;
    }

    return true;
}

bool WinRegKey::Query(const wchar_t *pValueName, DWORD type, LPBYTE pData, LPDWORD pcbData)
{
    DWORD keyType = type;
    LONG lRes;
    DWORD dstSize = (pcbData) ? (*pcbData) : (0);

    // query the value
    lRes = RegQueryValueExW(m_hKey, pValueName, NULL, &keyType, pData, pcbData);
    if (ERROR_SUCCESS != lRes)
    {
        DISPATCHER_LOG_OPERATION(SetLastError(lRes));
        TRACE_WINREG_ERROR("Querying \"%S\" : RegQueryValueExA()==0x%x\n", pValueName, GetLastError());
        return false;
    }

    // check the type
    if (keyType != type)
    {
        TRACE_WINREG_ERROR("Querying \"%S\" : expectedType=%d, returned=%d\n", pValueName, type, keyType);
        return false;
    }

    // terminate the string only if pointers not NULL
    if ((REG_SZ == type || REG_EXPAND_SZ == type) && NULL != pData && NULL != pcbData)
    {
        wchar_t *pString = (wchar_t *) pData;
        size_t NullEndingSizeBytes = sizeof(wchar_t); // size of string termination null character
        if (dstSize < NullEndingSizeBytes)
        {
            TRACE_WINREG_ERROR("Querying \"%S\" : buffer is too small for null-terminated string", pValueName);
            return false;
        }
        size_t maxStringLengthBytes = dstSize - NullEndingSizeBytes;
        size_t maxStringIndex = dstSize / sizeof(wchar_t) - 1;

        size_t lastIndex = (maxStringLengthBytes < *pcbData) ? (maxStringIndex) : (*pcbData) / sizeof(wchar_t);

        pString[lastIndex] = (wchar_t) 0;
    }
    else if(REG_MULTI_SZ == type && NULL != pData && NULL != pcbData)
    {
        wchar_t *pString = (wchar_t *) pData;
        size_t NullEndingSizeBytes = sizeof(wchar_t)*2; // size of string termination null characters
        if (dstSize < NullEndingSizeBytes)
        {
            TRACE_WINREG_ERROR("Querying \"%S\" : buffer is too small for multi-line null-terminated string", pValueName);
            return false;
        }
        size_t maxStringLengthBytes = dstSize - NullEndingSizeBytes;
        size_t maxStringIndex = dstSize / sizeof(wchar_t) - 1;

        size_t lastIndex = (maxStringLengthBytes < *pcbData) ? (maxStringIndex) : (*pcbData) / sizeof(wchar_t) + 1;

        // last 2 bytes should be 0 in case of REG_MULTI_SZ
        pString[lastIndex] = pString[lastIndex - 1] = (wchar_t) 0;
    }

    return true;

} // bool WinRegKey::Query(const wchar_t *pValueName, DWORD type, LPBYTE pData, LPDWORD pcbData)

bool WinRegKey::EnumValue(DWORD index, wchar_t *pValueName, LPDWORD pcchValueName, LPDWORD pType)
{
    LONG lRes;

    // enum the values
    lRes = RegEnumValueW(m_hKey, index, pValueName, pcchValueName, 0, pType, NULL, NULL);
    if (ERROR_SUCCESS != lRes)
    {
        DISPATCHER_LOG_OPERATION(SetLastError(lRes));
        return false;
    }

    return true;

} // bool WinRegKey::EnumValue(DWORD index, wchar_t *pValueName, LPDWORD pcchValueName, LPDWORD pType)

bool WinRegKey::EnumKey(DWORD index, wchar_t *pValueName, LPDWORD pcchValueName)
{
    LONG lRes;

    // enum the keys
    lRes = RegEnumKeyExW(m_hKey, index, pValueName, pcchValueName, NULL, NULL, NULL, NULL);
    if (ERROR_SUCCESS != lRes)
    {
        DISPATCHER_LOG_OPERATION(SetLastError(lRes));
        TRACE_WINREG_ERROR("EnumKey with index=%d: RegEnumKeyExW()==0x%x\n", index, GetLastError());
        return false;
    }

    return true;

} // bool WinRegKey::EnumKey(DWORD index, wchar_t *pValueName, LPDWORD pcchValueName)

bool WinRegKey::QueryInfo(LPDWORD lpcSubkeys)
{
    LONG lRes;

    lRes = RegQueryInfoKeyW(m_hKey, NULL, 0, 0, lpcSubkeys, 0, 0, 0, 0, 0, 0, 0);
    if (ERROR_SUCCESS != lRes) {
        TRACE_WINREG_ERROR("RegQueryInfoKeyW()==0x%x\n", lRes);
        return false;
    }
    return true;

} //bool QueryInfo(LPDWORD lpcSubkeys);

} // namespace MFX

#endif // #if !defined(MEDIASDK_UWP_DISPATCHER)