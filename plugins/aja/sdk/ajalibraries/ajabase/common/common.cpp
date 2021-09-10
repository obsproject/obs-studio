/* SPDX-License-Identifier: MIT */
/**
    @file		common.cpp
    @brief		Generic helper functions.
    @copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#include "common.h"

#include <algorithm>
#include <cstring>
#include <sstream>
#include <string>

#include <stdlib.h>
#include <wchar.h>

// check for C++11 compatibility
#if defined(_MSC_VER) && _MSC_VER >= 1800
    // Visual Studio 2013 (_MSC_VER 1800) has the C++11 support we
    // need, even though it reports 199711L for __cplusplus
    #define AJA_BASE_USECPP_11  1
#elif defined(__clang__)
    // Note that the __clang__ test needs to go before the __GNUC__ test since it also defines __GNUC__
    #if __cplusplus >= 201402L
        #if __has_include(<codecvt>)
            #define AJA_BASE_USECPP_11  1
        #endif
    #elif __cplusplus >= 201103L
        // For clang only turn on C++11 support if using libc++
        // libstdc++ does not always have support for <codecvt>
        #if defined(_LIBCPP_VERSION)
            #define AJA_BASE_USECPP_11  1
        #endif
    #endif
#elif defined(__GNUC__)
    // GCC < 5 says it supports C++11 but does not support "Standard code conversion facets"
    // aka the stuff in <codecvt> we need. The facet support was added in 5.0
    #if __GNUC__ >= 5
        #define AJA_BASE_USECPP_11  1
    #endif
#elif __cplusplus >= 201103L
    // By this point we should be at a compiler that tells the truth (does that exist?)
    #define AJA_BASE_USECPP_11  1
#endif

// use C++11 functionality if available
#if defined(AJA_BASE_USECPP_11)
    #include <locale>
    #include <codecvt>
#else
    #if defined(AJA_WINDOWS)
        #include <Windows.h>
    #endif
#endif

namespace aja
{

std::string& replace(std::string& str, const std::string& from, const std::string& to)
{
    if (!from.empty())
    {
        for (size_t pos = 0;  (pos = str.find(from, pos)) != std::string::npos;  pos += to.size())
        {
            str.replace(pos, from.size(), to);
        }
    }
    return str;
}

int stoi (const std::string & str, std::size_t * idx, int base)
{
    return (int)aja::stol(str, idx, base);
}

long stol (const std::string & str, std::size_t * idx, int base)
{
    char* pEnd = NULL;
    long retVal = ::strtol(str.c_str(), &pEnd, base);
    if (idx && pEnd)
    {
        *idx = pEnd - str.c_str();
    }
    return retVal;
}
/*
long long stoll (const std::string & str, std::size_t * idx, int base)
{
    return (long long)aja::stol(str, idx, base);
}
*/
unsigned long stoul (const std::string & str, std::size_t * idx, int base)
{
    char* pEnd = NULL;
    unsigned long retVal = ::strtoul(str.c_str(), &pEnd, base);
    if (idx && pEnd)
    {
        *idx = pEnd - str.c_str();
    }
    return retVal;
}
/*
unsigned long long stoull (const std::string & str, std::size_t * idx, int base)
{
    return (unsigned long long)aja::stoul(str, idx, base);
}
*/
float stof (const std::string & str, std::size_t * idx)
{
    return (float)aja::stod(str, idx);
}

double stod (const std::string & str, std::size_t * idx)
{
    char* pEnd = NULL;
    double retVal = ::strtod(str.c_str(), &pEnd);
    if (idx && pEnd)
    {
        *idx = pEnd - str.c_str();
    }
    return retVal;
}

long double stold (const std::string & str, std::size_t * idx)
{
    return (long double)aja::stod(str, idx);
}

std::string to_string (bool val)
{
    return val ? "true" : "false";
}

std::string to_string (int val)
{
    std::ostringstream oss; oss << val;
    return oss.str();
}

std::string to_string (long val)
{
    std::ostringstream oss; oss << val;
    return oss.str();
}
/*
std::string to_string (long long val)
{
    std::ostringstream oss; oss << val;
    return oss.str();
}
*/
std::string to_string (unsigned val)
{
    std::ostringstream oss; oss << val;
    return oss.str();
}

std::string to_string (unsigned long val)
{
    std::ostringstream oss; oss << val;
    return oss.str();
}
/*
std::string to_string (unsigned long long val)
{
    std::ostringstream oss; oss << val;
    return oss.str();
}
*/
std::string to_string (float val)
{
    std::ostringstream oss;
    oss.precision(6);
    oss.setf(std::ios::fixed, std::ios::floatfield);
    oss << val;
    return oss.str();
}

std::string to_string (double val)
{
    std::ostringstream oss;
    oss.precision(6);
    oss.setf(std::ios::fixed, std::ios::floatfield);
    oss << val;
    return oss.str();
}

std::string to_string (long double val)
{
    std::ostringstream oss;
    oss.precision(6);
    oss.setf(std::ios::fixed, std::ios::floatfield);
    oss << val;
    return oss.str();
}

bool string_to_wstring (const std::string & str, std::wstring & wstr)
{
// use C++11 functionality if available
#if defined(AJA_BASE_USECPP_11)
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converterX;
    wstr = converterX.from_bytes(str);
    return true;
#else
    #if defined(AJA_WINDOWS)
        const char *tmpPtr = str.c_str();
        uint32_t codePage = CP_UTF8;
        uint32_t flags = 0;
        size_t len = MultiByteToWideChar(codePage, flags, tmpPtr, (int)str.length(), NULL, 0);
        wstr.resize(len);
        int retVal = MultiByteToWideChar(codePage, flags, tmpPtr, (int)str.length(), &wstr[0], (int)len);
        if (retVal == 0)
            return false;

        return true;
    #else
        std::mbstate_t state = std::mbstate_t();
        mbrlen(NULL, 0, &state);
        const char *tmpPtr = str.c_str();
        int len = (int)mbsrtowcs(NULL, &tmpPtr, 0, &state);
        if (len == -1)
            return false;

        wstr.resize((size_t)len);
        int num_chars = (int)mbsrtowcs(&wstr[0], &tmpPtr, wstr.length(), &state);
        if (num_chars < 0)
            return false;

        return true;
    #endif
#endif
}

bool wstring_to_string (const std::wstring & wstr, std::string & str)
{
// use C++11 functionality if available
#if defined(AJA_BASE_USECPP_11)
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converterX;
    str = converterX.to_bytes(wstr);
    return true;
#else
    #if defined(AJA_WINDOWS)
        const wchar_t *tmpPtr = wstr.c_str();
        uint32_t codePage = CP_UTF8;
        uint32_t flags = 0;
        size_t len = WideCharToMultiByte(codePage, flags, tmpPtr, (int)wstr.length(), NULL, 0, NULL, NULL);
        str.resize(len);
        int retVal = WideCharToMultiByte(codePage, flags, tmpPtr, (int)wstr.length(), &str[0], (int)len, NULL, NULL);
        if (retVal == 0)
            return false;

        return true;
    #else
        std::mbstate_t state = std::mbstate_t();
        mbrlen(NULL, 0, &state);
        const wchar_t *tmpPtr = wstr.c_str();
        int len = (int)wcsrtombs(NULL, &tmpPtr, 0, &state);
        if (len == -1)
            return false;

        str.resize((size_t)len);
        int num_chars = (int)wcsrtombs(&str[0], &tmpPtr, str.length(), &state);
        if (num_chars < 0)
            return false;

        return true;
    #endif
#endif
}

inline size_t local_min (const size_t & a, const size_t & b)
{
#if defined(AJA_WINDOWS) && !defined(AJA_BASE_USECPP_11)
    // By including the Windows.h header that brings in the min() macro which prevents us from using std::min()
    // so implement our own
    size_t size = b;
    if (a < b)
        size = a;
    return size;
#else
    return std::min(a, b);
#endif
}

bool string_to_cstring (const std::string & str, char * c_str, size_t c_str_size)
{
    if(c_str == NULL || c_str_size < 1)
        return false;

    size_t maxSize = local_min(str.size(), c_str_size-1);
    for(size_t i=0;i<maxSize;++i)
    {
        c_str[i] = str[i];
    }
    c_str[maxSize] = '\0';
    return true;
}

void split (const std::string & str, const char delim, std::vector<std::string> & elems)
{
	elems.clear();
    std::stringstream ss(str);
    std::string item;
    while(std::getline(ss, item, delim))
    {
        elems.push_back(item);
    }

    // if last character in string matches the split delim add an empty string
    if (str.length() > 0 && str[str.length()-1] == delim)
    {
        elems.push_back("");
    }
}

std::vector<std::string> split (const std::string & str, const char delim)
{
    std::vector<std::string> elems;
    split(str, delim, elems);
    return elems;
}

std::vector<std::string> split (const std::string & inStr, const std::string & inDelim)
{
	std::vector<std::string> result;
	size_t	startPos(0);
	size_t	delimPos(inStr.find(inDelim, startPos));
	while (delimPos != std::string::npos)
	{
		const std::string item (inStr.substr(startPos, delimPos - startPos));
		result.push_back(item);
		startPos = delimPos + inDelim.length();
		delimPos = inStr.find(inDelim, startPos);
	}
	if (startPos < inStr.length())			//	add last piece
		result.push_back(inStr.substr(startPos, inStr.length()-startPos));
	else if (startPos == inStr.length())	//	if last character in string matches the split delim add an empty string
		result.push_back(std::string());
	return result;
}

std::string & lower (std::string & str)
{
    std::transform (str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

std::string & upper (std::string & str)
{
    std::transform (str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

std::string & lstrip (std::string & str, const std::string & ws)
{
    str.erase(0, str.find_first_not_of(ws));
    return str;
}

std::string & rstrip (std::string & str, const std::string & ws)
{
    if (!str.empty())
        str.erase (str.find_last_not_of(ws)+1, str.length()-1);
    return str;
}

std::string & strip (std::string & str, const std::string & ws)
{
    lstrip(str,ws);
    rstrip(str,ws);
    return str;
}

std::string join (const std::vector<std::string> & parts, const std::string & delim)
{
    std::ostringstream oss;
    for (std::vector<std::string>::const_iterator it(parts.begin());  it != parts.end();  )
    {
        oss << *it;
        if (++it != parts.end())
            oss << delim;
    }
    return oss.str();
}

char* safer_strncpy(char* target, const char* source, size_t num, size_t maxSize)
{
    int32_t lastIndex = (int32_t)maxSize-1;

    if (lastIndex < 0 || target == NULL)
    {
        return target;
    }

    if (num >= maxSize)
        num = (size_t)lastIndex;

    char *retVal = strncpy(target, source, num);
    // make sure always null terminated
    target[num] = '\0';

    return retVal;
}

} //end aja namespace
