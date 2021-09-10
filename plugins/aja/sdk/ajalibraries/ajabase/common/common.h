/* SPDX-License-Identifier: MIT */
/**
	@file		common.h
	@brief		Private include file for all ajabase sources.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_COMMON_H
#define AJA_COMMON_H

#if defined(AJA_WINDOWS)
	#pragma warning(disable:4996)
	#pragma warning(disable:4800)
#endif

#include "ajabase/common/export.h"
#include "ajabase/common/public.h"
#include "ajabase/system/debug.h"

namespace aja
{

static std::string WHITESPACE = " \t\n\r";

/**
 *	Replaces all occurrences of a substring within a string with a new string
 *
 *  @param[in,out]  str  The string to modify
 *  @param[in]      from The substring to look for
 *  @param[in]      to   The string to replace the substring with
 *	@return		    Reference to the modified STL string
 */
AJA_EXPORT std::string& replace(std::string& str, const std::string& from, const std::string& to);

// NOTE: these sto* functions are ment to be similar to the C++11 functions of the same name
//       one big difference is that these do not throw exceptions on errors and instead return
//       0 like the underlying strtol function does.

/**
 *	Convert string to integer
 *
 *  @param[in]      str  The string to get the int value of
 *  @param[in,out]  idx  Pointer to an object of type size_t, whose value is set by the function
 *                       to the position of the next character in str after the numerical value.
 *                       This parameter can also be a null pointer, in which case it is not used.
 *  @param[in]      base Numerical base that determines the valid characters and their interpretation.
 *                       If this is 0, the base is determined by the format (like strtol).
 *	@return         The int value of the input str
 */
AJA_EXPORT int stoi(const std::string& str, std::size_t* idx=0, int base = 10);

/**
 *	Convert string to long
 *
 *  @param[in]      str  The string to get the long value of
 *  @param[in,out]  idx  Pointer to an object of type size_t, whose value is set by the function
 *                       to the position of the next character in str after the numerical value.
 *                       This parameter can also be a null pointer, in which case it is not used.
 *  @param[in]      base Numerical base that determines the valid characters and their interpretation.
 *                       If this is 0, the base is determined by the format (like strtol).
 *	@return         The long value of the input str
 */
AJA_EXPORT long stol(const std::string& str, std::size_t* idx=0, int base = 10);

/**
 *	Convert string to long long
 *
 *  @param[in]      str  The string to get the long long value of
 *  @param[in,out]  idx  Pointer to an object of type size_t, whose value is set by the function
 *                       to the position of the next character in str after the numerical value.
 *                       This parameter can also be a null pointer, in which case it is not used.
 *  @param[in]      base Numerical base that determines the valid characters and their interpretation.
 *                       If this is 0, the base is determined by the format (like strtol).
 *	@return         The long long value of the input str
 */
//AJA_EXPORT long long stoll(const std::string& str, std::size_t* idx=0, int base = 10);

/**
 *	Convert string to unsigned long
 *
 *  @param[in]      str  The string to get the unsigned long value of
 *  @param[in,out]  idx  Pointer to an object of type size_t, whose value is set by the function
 *                       to the position of the next character in str after the numerical value.
 *                       This parameter can also be a null pointer, in which case it is not used.
 *  @param[in]      base Numerical base that determines the valid characters and their interpretation.
 *                       If this is 0, the base is determined by the format (like strtol).
 *	@return         The unsigned long value of the input str
 */
AJA_EXPORT unsigned long stoul(const std::string& str, std::size_t* idx=0, int base = 10);

/**
 *	Convert string to unsigned long long
 *
 *  @param[in]      str  The string to get the unsigned long long value of
 *  @param[in,out]  idx  Pointer to an object of type size_t, whose value is set by the function
 *                       to the position of the next character in str after the numerical value.
 *                       This parameter can also be a null pointer, in which case it is not used.
 *  @param[in]      base Numerical base that determines the valid characters and their interpretation.
 *                       If this is 0, the base is determined by the format (like strtol).
 *	@return         The unsigned long long value of the input str
 */
//AJA_EXPORT unsigned long long stoull(const std::string& str, std::size_t* idx=0, int base = 10);

/**
 *	Convert string to float
 *
 *  @param[in]      str  The string to get the float value of
 *  @param[in,out]  idx  Pointer to an object of type size_t, whose value is set by the function
 *                       to the position of the next character in str after the numerical value.
 *                       This parameter can also be a null pointer, in which case it is not used.
 *	@return         The float value of the input str
 */
AJA_EXPORT float stof(const std::string& str, std::size_t* idx=0);

/**
 *	Convert string to double
 *
 *  @param[in]      str  The string to get the double value of
 *  @param[in,out]  idx  Pointer to an object of type size_t, whose value is set by the function
 *                       to the position of the next character in str after the numerical value.
 *                       This parameter can also be a null pointer, in which case it is not used.
 *	@return         The double value of the input str
 */
AJA_EXPORT double stod(const std::string& str, std::size_t* idx=0);

/**
 *	Convert string to long double
 *
 *  @param[in]      str  The string to get the long double value of
 *  @param[in,out]  idx  Pointer to an object of type size_t, whose value is set by the function
 *                       to the position of the next character in str after the numerical value.
 *                       This parameter can also be a null pointer, in which case it is not used.
 *	@return         The long double value of the input str
 */
AJA_EXPORT long double stold(const std::string& str, std::size_t* idx=0);

/**
 *	Convert numerical value to string
 *
 *  @param[in]  val The numerical value to convert
 *	@return		A string representing the passed value
 */
AJA_EXPORT std::string to_string(bool val);
AJA_EXPORT std::string to_string(int val);
AJA_EXPORT std::string to_string(long val);
//AJA_EXPORT std::string to_string(long long val);
AJA_EXPORT std::string to_string(unsigned val);
AJA_EXPORT std::string to_string(unsigned long val);
//AJA_EXPORT std::string to_string(unsigned long long val);
AJA_EXPORT std::string to_string(float val);
AJA_EXPORT std::string to_string(double val);
AJA_EXPORT std::string to_string(long double val);

/**
 *	Convert string to wstring
 *
 *  @param[in]  str The string to convert
 *  @param[out] wstr The wstring to convert to
 *	@return		true if success else false
 */
AJA_EXPORT bool string_to_wstring(const std::string& str, std::wstring& wstr);

/**
 *	Convert wstring to string
 *
 *  @param[in]  wstr The wstring to convert
 *  @param[out] str  The string to convert to
 *	@return		true if success else false
 */
AJA_EXPORT bool wstring_to_string(const std::wstring& wstr, std::string& str);

/**
 *	Convert string to cstring
 *
 *  @param[in]  str The string to convert
 *  @param[out] c_str  The char buffer to use as a c string
 *  @param[in]  c_str_size  The size of the passed in c_str buffer in bytes
 *	@return		true if success else false
 */
AJA_EXPORT bool string_to_cstring (const std::string & str, char * c_str, size_t c_str_size);

/**
 *	Splits a string into substrings at a character delimiter
 *
 *  @param[in]   str   The string to split into parts
 *  @param[in]   delim The character delimiter to split the string at
 *  @param[out]  elems A vector of strings that contains all the substrings
 */
AJA_EXPORT void split (const std::string & str, const char delim, std::vector<std::string> & elems);

/**
 *	Splits a string into substrings at a character delimiter
 *
 *  @param[in]  str   The string to split into parts
 *  @param[in]  delim The character delimiter to split the string at
 *	@return		A vector of strings that contains all the substrings
 */
AJA_EXPORT std::vector<std::string> split (const std::string & str, const char delim);

/**
 *	Splits a string into substrings at a string delimiter
 *
 *  @param[in]  inStr   The string to split into parts
 *  @param[in]  inDelim The delimiter string to split the string at
 *	@return		A vector of strings that contains all the substrings
 */
AJA_EXPORT std::vector<std::string> split (const std::string & inStr, const std::string & inDelim);

/**
 *	Converts the passed string to lowercase
 *
 *  @param[in,out]  str   The string to make lowercase
 *	@return		    Reference to the modified STL string
 */
AJA_EXPORT std::string & lower (std::string & str);

/**
 *	Converts the passed string to uppercase
 *
 *  @param[in,out]  str   The string to make uppercase
 *	@return		    Reference to the modified STL string
 */
AJA_EXPORT std::string & upper (std::string & str);

/**
 *	Strips the leading whitespace characters from the string
 *
 *  @param[in,out]  str  The string to strip leading characters from
 *  @param[in]      ws   The whitespace characters to strip
 *	@return		    Reference to the modified STL string
 */
AJA_EXPORT std::string & lstrip (std::string & str, const std::string & ws = aja::WHITESPACE);

/**
 *	Strips the trailing whitespace characters from the string
 *
 *  @param[in,out]  str  The string to strip trailing characters from
 *  @param[in]      ws   The whitespace characters to strip
 *	@return		    Reference to the modified STL string
 */
AJA_EXPORT std::string & rstrip (std::string & str, const std::string & ws = aja::WHITESPACE);

/**
 *	Strips the leading & trailing whitespace characters from the string
 *
 *  @param[in,out]  str  The string to strip leading & trailing characters from
 *  @param[in]      ws   The whitespace characters to strip
 *	@return		    Reference to the modified STL string
 */
AJA_EXPORT std::string & strip (std::string & str, const std::string & ws = aja::WHITESPACE);

/**
 *	Join a vector of strings separated by a string delimeter
 *
 *  @param[in]  parts  The vector of strings to join together
 *  @param[in]  delim  The string delimeter that will separate the strings
 *	@return		The joined string made up of the parts concatinated with delimeter string
 */
AJA_EXPORT std::string join (const std::vector<std::string> & parts, const std::string & delim=" ");

/**
 *	Like strncpy() but always adds a null-character at last index of target string
 *
 *  @param[in,out]  target  Pointer to the destination array where the content is to be copied.
 *  @param[in]      source  C string to be copied.
 *  @param[in]      num     Maximum number of characters to be copied from source.
 *  @param[in]      maxSize Maximum size of the destination array pointed to by target.
 *	@return target is returned
 */
AJA_EXPORT char* safer_strncpy(char* target, const char* source, size_t num, size_t maxSize);

} //end aja namespace

#endif	//	AJA_COMMON_H
