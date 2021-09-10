/* SPDX-License-Identifier: MIT */
/**
    @file		system.cpp
    @brief		System specific helper functions.
    @copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#include "system.h"

#include <cstdlib>
#include <sstream>

#if defined(AJA_WINDOWS)

namespace aja
{
    bool
    write_registry_string(HKEY hkey, std::string key_path, std::string key, std::string value)
    {
        bool outValue = false;

        DWORD dwType = REG_EXPAND_SZ;
        DWORD dwInSize = (DWORD)value.size();

        DWORD disposition;

        HKEY openedPathHkey;
        long lResult = RegCreateKeyExA(hkey, key_path.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_WOW64_64KEY, NULL, &openedPathHkey, &disposition);
        if(ERROR_SUCCESS == lResult)
        {
            lResult = RegSetValueExA(openedPathHkey, key.c_str(), NULL, dwType, (BYTE*)value.c_str(), dwInSize);
            if (lResult == ERROR_SUCCESS)
            {
                outValue = true;
            }
            RegCloseKey(openedPathHkey);
        }

        return outValue;
    }

    bool
    write_registry_dword(HKEY hkey, std::string key_path, std::string key, DWORD value)
    {
        bool outValue = false;

        DWORD dwType = REG_DWORD;
        DWORD dwInSize = sizeof(value);

        DWORD disposition;

        HKEY openedPathHkey;
        long lResult = RegCreateKeyExA(hkey, key_path.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_WOW64_64KEY, NULL, &openedPathHkey, &disposition);
        if(ERROR_SUCCESS == lResult)
        {
            lResult = RegSetValueExA(openedPathHkey, key.c_str(), NULL, dwType, (BYTE*)&value, dwInSize);
            if (lResult == ERROR_SUCCESS)
            {
                outValue = true;
            }
            RegCloseKey(openedPathHkey);
        }

        return outValue;
    }

    std::string
    read_registry_string(HKEY hkey, std::string key_path, std::string key)
    {
        std::string outValue;

        const DWORD buffSize = 128;
        char szBuff[buffSize];
        memset(szBuff, 0, buffSize);

        DWORD dwType = 0;
        DWORD dwInOutSize = buffSize;

        HKEY openedPathHkey;
        long lResult = RegOpenKeyExA(hkey, key_path.c_str(), 0, KEY_QUERY_VALUE|KEY_WOW64_64KEY, &openedPathHkey);
        if(ERROR_SUCCESS == lResult)
        {
            lResult = RegQueryValueExA(openedPathHkey, key.c_str(), NULL, &dwType, (BYTE*)szBuff, &dwInOutSize);
            if (lResult == ERROR_SUCCESS)
            {
                outValue = szBuff;
            }

            RegCloseKey(openedPathHkey);
        }

        return outValue;
    }

    DWORD
    read_registry_dword(HKEY hkey, std::string key_path, std::string key)
    {
        DWORD outValue=0;

        DWORD dwType = 0;
        DWORD dwInOutSize = sizeof(outValue);

        HKEY openedPathHkey;
        long lResult = RegOpenKeyExA(hkey, key_path.c_str(), 0, KEY_QUERY_VALUE|KEY_WOW64_64KEY, &openedPathHkey);
        if(ERROR_SUCCESS == lResult)
        {
            lResult = RegQueryValueExA(openedPathHkey, key.c_str(), NULL, &dwType, (BYTE*)&outValue, &dwInOutSize);
            RegCloseKey(openedPathHkey);
        }

        return outValue;
    }

    int reveal_file_in_file_manager(const std::string& filePath)
    {
        std::ostringstream oss;
        oss << "explorer /select," << "\"" << filePath << "\"";
        return ::system(oss.str().c_str());
    }

} //end aja namespace

#endif //end AJA_WINDOWS

#if defined(AJA_MAC)
namespace aja
{
    int reveal_file_in_file_manager(const std::string& filePath)
    {
        std::ostringstream oss;
        oss << "/usr/bin/osascript" << " -e \"" << "tell application \\\"Finder\\\"" << "\""
                                    << " -e \"" << "activate" << "\""
                                    << " -e \"" << "reveal \\\"" + filePath + "\\\" as POSIX file" << "\""
                                    << " -e \"" << "end tell" << "\"";
        return ::system(oss.str().c_str());
    }
} //end aja namespace
#endif //end AJA_MAC

#if defined(AJA_LINUX)
#include <libgen.h> // for dirname()
namespace aja
{
    int reveal_file_in_file_manager(const std::string& filePath)
    {
        // need to pass the directory of the file to open in file manager, otherwise
        // will open in the default application for file type
        std::ostringstream oss;
        oss << "xdg-open " << "\"" << dirname((char*)filePath.c_str()) << "\"";
        return ::system(oss.str().c_str());
    }
} //end aja namespace
#endif //end AJA_LINUX
