//------------------------------------------------------------------------
// Project     : SDK Base
// Version     : 1.0
//
// Category    : Helpers
// Filename    : base/source/fstring.h
// Created by  : Steinberg, 2008
// Description : String class
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/ftypes.h"
#include "pluginterfaces/base/fstrdefs.h"
#include "pluginterfaces/base/istringresult.h"
#include "pluginterfaces/base/ipersistent.h"

#include "base/source/fobject.h"

#include <cstdarg>
#include <cstdlib>

namespace Steinberg {

class FVariant;
class String;

#ifdef UNICODE
static const bool kWideStringDefault = true;
#else
static const bool kWideStringDefault = false;
#endif

static const uint16 kBomUtf16 = 0xFEFF;					///< UTF16 Byte Order Mark
static const char8* const kBomUtf8 = "\xEF\xBB\xBF";	///< UTF8 Byte Order Mark
static const int32 kBomUtf8Length = 3;


enum MBCodePage
{
	kCP_ANSI = 0,			///< Default ANSI codepage.
	kCP_MAC_ROMAN = 2,		///< Default Mac codepage.

	kCP_ANSI_WEL = 1252,	///< West European Latin Encoding.
	kCP_MAC_CEE = 10029,	///< Mac Central European Encoding.
	kCP_Utf8 = 65001,		///< UTF8 Encoding.
	kCP_ShiftJIS = 932,		///< Shifted Japan Industrial Standard Encoding.
	kCP_US_ASCII = 20127,	///< US-ASCII (7-bit).

	kCP_Default = kCP_ANSI	///< Default ANSI codepage.
};

enum UnicodeNormalization
{
	kUnicodeNormC,	///< Unicode normalization Form C, canonical composition. 
	kUnicodeNormD,	///< Unicode normalization Form D, canonical decomposition. 
	kUnicodeNormKC,	///< Unicode normalization form KC, compatibility composition. 
	kUnicodeNormKD 	///< Unicode normalization form KD, compatibility decomposition. 
};

//------------------------------------------------------------------------
// Helper functions to create hash codes from string data.
//------------------------------------------------------------------------
extern uint32 hashString8 (const char8* s, uint32 m);
extern uint32 hashString16 (const char16* s, uint32 m);
inline uint32 hashString (const tchar* s, uint32 m)
{
#ifdef UNICODE
	return hashString16 (s, m);
#else
	return hashString8 (s, m);
#endif
}


//-----------------------------------------------------------------------------
/** Invariant String. 
@ingroup adt

A base class which provides methods to work with its
member string. Neither of the operations allows modifying the member string and 
that is why all operation are declared as const. 

There are operations for access, comparison, find, numbers and conversion.

Almost all operations exist in three versions for char8, char16 and the 
polymorphic type tchar. The type tchar can either be char8 or char16 depending
on whether UNICODE is activated or not.*/
//-----------------------------------------------------------------------------
class ConstString
{
public:
//-----------------------------------------------------------------------------
	ConstString (const char8* str, int32 length = -1);	///< Assign from string of type char8 (length=-1: all)
	ConstString (const char16* str, int32 length = -1);	///< Assign from string of type char16 (length=-1: all)
	ConstString (const ConstString& str, int32 offset = 0, int32 length = -1); ///< Copy constructor (length=-1: all).
	ConstString (const FVariant& var);	///< Assign a string from FVariant
	ConstString ();
	virtual ~ConstString () {}			///< Destructor.

	// access -----------------------------------------------------------------
	virtual int32 length () const {return static_cast<int32> (len);}	///< Return length of string
	inline bool isEmpty () const {return buffer == nullptr || len == 0;}		///< Return true if string is empty

	operator const char8* () const {return text8 ();} 							///< Returns pointer to string of type char8 (no modification allowed)
	operator const char16* () const {return text16 ();}							///< Returns pointer to string of type char16(no modification allowed)
	inline tchar operator[] (short idx) const {return getChar (static_cast<uint32> (idx));} ///< Returns character at 'idx'
	inline tchar operator[] (long idx) const {return getChar (static_cast<uint32> (idx));}
	inline tchar operator[] (int idx) const {return getChar (static_cast<uint32> (idx));}
	inline tchar operator[] (unsigned short idx) const {return getChar (idx);}
	inline tchar operator[] (unsigned long idx) const {return getChar (static_cast<uint32> (idx));}
	inline tchar operator[] (unsigned int idx) const {return getChar (idx);}

	inline virtual const char8* text8 () const;		///< Returns pointer to string of type char8
	inline virtual const char16* text16 () const;	///< Returns pointer to string of type char16
	inline virtual const tchar* text () const;		///< Returns pointer to string of type tchar
	inline virtual const void* ptr () const {return buffer;}	///< Returns pointer to string of type void

	inline virtual char8 getChar8 (uint32 index) const;		///< Returns character of type char16 at 'index'
	inline virtual char16 getChar16 (uint32 index) const;	///< Returns character of type char8 at 'index'
	inline tchar getChar (uint32 index) const;				///< Returns character of type tchar at 'index'
	inline tchar getCharAt (uint32 index) const;			///< Returns character of type tchar at 'index', no conversion!

	bool testChar8 (uint32 index, char8 c) const;			///< Returns true if character is equal at position 'index'
	bool testChar16 (uint32 index, char16 c) const;
	inline bool testChar (uint32 index, char8 c) const {return testChar8 (index, c);}
	inline bool testChar (uint32 index, char16 c) const {return testChar16 (index, c);}

	bool extract (String& result, uint32 idx, int32 n = -1) const;		///< Get n characters long substring starting at index (n=-1: until end)
	int32 copyTo8 (char8* str, uint32 idx = 0, int32 n = -1) const;
	int32 copyTo16 (char16* str, uint32 idx = 0, int32 n = -1) const;
	int32 copyTo (tchar* str, uint32 idx = 0, int32 n = -1) const;
	void copyTo (IStringResult* result) const;							///< Copies whole member string
	void copyTo (IString& string) const;							    ///< Copies whole member string

	inline uint32 hash (uint32 tsize) const 
	{
		return isWide ? hashString16 (buffer16, tsize) : hashString8 (buffer8, tsize) ;  
	}
	//-------------------------------------------------------------------------

	// compare ----------------------------------------------------------------
	enum CompareMode
	{
		kCaseSensitive,		///< Comparison is done with regard to character's case
		kCaseInsensitive	///< Comparison is done without regard to character's case
	};

	int32 compareAt (uint32 index, const ConstString& str, int32 n = -1, CompareMode m = kCaseSensitive) const; 				///< Compare n characters of str with n characters of this starting at index (return: see above)
	int32 compare (const ConstString& str, int32 n, CompareMode m = kCaseSensitive) const;										///< Compare n characters of str with n characters of this (return: see above)
	int32 compare (const ConstString& str, CompareMode m = kCaseSensitive) const;												///< Compare all characters of str with this (return: see above)

	int32 naturalCompare (const ConstString& str, CompareMode mode = kCaseSensitive) const;

	bool startsWith (const ConstString& str, CompareMode m = kCaseSensitive) const;												///< Check if this starts with str
	bool endsWith (const ConstString& str, CompareMode m = kCaseSensitive) const;												///< Check if this ends with str
	bool contains (const ConstString& str, CompareMode m = kCaseSensitive) const;												///< Check if this contains str											

	// static methods
	static bool isCharSpace (char8 character);	   ///< Returns true if character is a space
	static bool isCharSpace (char16 character);    ///< @copydoc isCharSpace(const char8)
	static bool isCharAlpha (char8 character);	   ///< Returns true if character is an alphabetic character
	static bool isCharAlpha (char16 character);    ///< @copydoc isCharAlpha(const char8)
	static bool isCharAlphaNum (char8 character);  ///< Returns true if character is an alphanumeric character
	static bool isCharAlphaNum (char16 character); ///< @copydoc isCharAlphaNum(const char8)
	static bool isCharDigit (char8 character);	   ///< Returns true if character is a number
	static bool isCharDigit (char16 character);    ///< @copydoc isCharDigit(const char8)
	static bool isCharAscii (char8 character);     ///< Returns true if character is in ASCII range
	static bool isCharAscii (char16 character);    ///< Returns true if character is in ASCII range
	static bool isCharUpper (char8 character);
	static bool isCharUpper (char16 character);
	static bool isCharLower (char8 character);
	static bool isCharLower (char16 character);
	//-------------------------------------------------------------------------

	/** @name Find first occurrence of n characters of str in this (n=-1: all) ending at endIndex (endIndex = -1: all)*/
	///@{
	inline int32 findFirst (const ConstString& str, int32 n = -1, CompareMode m = kCaseSensitive, int32 endIndex = -1) const {return findNext (0, str, n, m, endIndex);}
	inline int32 findFirst (char8 c, CompareMode m = kCaseSensitive, int32 endIndex = -1) const {return findNext (0, c, m, endIndex);}
	inline int32 findFirst (char16 c, CompareMode m = kCaseSensitive, int32 endIndex = -1) const {return findNext (0, c, m, endIndex);}
	///@}
	/** @name Find next occurrence of n characters of str starting at startIndex in this (n=-1: all) ending at endIndex (endIndex = -1: all)*/
	///@{
	int32 findNext (int32 startIndex, const ConstString& str, int32 n = -1, CompareMode = kCaseSensitive, int32 endIndex = -1) const;
	int32 findNext (int32 startIndex, char8 c, CompareMode = kCaseSensitive, int32 endIndex = -1) const;
	int32 findNext (int32 startIndex, char16 c, CompareMode = kCaseSensitive, int32 endIndex = -1) const;
	///@}
	/** @name Find previous occurrence of n characters of str starting at startIndex in this (n=-1: all) */
	///@{
	int32 findPrev (int32 startIndex, const ConstString& str, int32 n = -1, CompareMode = kCaseSensitive) const;
	int32 findPrev (int32 startIndex, char8 c, CompareMode = kCaseSensitive) const;
	int32 findPrev (int32 startIndex, char16 c, CompareMode = kCaseSensitive) const;
	///@}
	
	inline int32 findLast (const ConstString& str, int32 n = -1, CompareMode m = kCaseSensitive) const {return findPrev (-1, str, n, m);}	///< Find last occurrence of n characters of str in this (n=-1: all)
	inline int32 findLast (char8 c, CompareMode m = kCaseSensitive) const {return findPrev (-1, c, m);}
	inline int32 findLast (char16 c, CompareMode m = kCaseSensitive) const {return findPrev (-1, c, m);}

	int32 countOccurences (char8 c, uint32 startIndex, CompareMode = kCaseSensitive) const; ///< Counts occurences of c within this starting at index
	int32 countOccurences (char16 c, uint32 startIndex, CompareMode = kCaseSensitive) const;
	int32 getFirstDifferent (const ConstString& str, CompareMode = kCaseSensitive) const;	///< Returns position of first different character
	//-------------------------------------------------------------------------

	// numbers ----------------------------------------------------------------
	bool isDigit (uint32 index) const;	///< Returns true if character at position is a digit
	bool scanFloat (double& value, uint32 offset = 0, bool scanToEnd = true) const;		///< Converts string to double value starting at offset
	bool scanInt64 (int64& value, uint32 offset = 0, bool scanToEnd = true) const;		///< Converts string to int64 value starting at offset
	bool scanUInt64 (uint64& value, uint32 offset = 0, bool scanToEnd = true) const;	///< Converts string to uint64 value starting at offset
	bool scanInt32 (int32& value, uint32 offset = 0, bool scanToEnd = true) const;		///< Converts string to int32 value starting at offset
	bool scanUInt32 (uint32& value, uint32 offset = 0, bool scanToEnd = true) const;	///< Converts string to uint32 value starting at offset
	bool scanHex (uint8& value, uint32 offset = 0, bool scanToEnd = true) const;		///< Converts string to hex/uint8 value starting at offset

	int32 getTrailingNumberIndex (uint32 width = 0) const;	///< Returns start index of trailing number
	int64 getTrailingNumber (int64 fallback = 0) const;		///< Returns result of scanInt64 or the fallback
	int64 getNumber () const;								///< Returns result of scanInt64

	// static methods
	static bool scanInt64_8 (const char8* text, int64& value, bool scanToEnd = true);	///< Converts string of type char8 to int64 value
	static bool scanInt64_16 (const char16* text, int64& value, bool scanToEnd = true);	///< Converts string of type char16 to int64 value
	static bool scanInt64 (const tchar* text, int64& value, bool scanToEnd = true);		///< Converts string of type tchar to int64 value

	static bool scanUInt64_8 (const char8* text, uint64& value, bool scanToEnd = true);		///< Converts string of type char8 to uint64 value
	static bool scanUInt64_16 (const char16* text, uint64& value, bool scanToEnd = true);	///< Converts string of type char16 to uint64 value
	static bool scanUInt64 (const tchar* text, uint64& value, bool scanToEnd = true);		///< Converts string of type tchar to uint64 value

	static bool scanInt32_8 (const char8* text, int32& value, bool scanToEnd = true);		///< Converts string of type char8 to int32 value
	static bool scanInt32_16 (const char16* text, int32& value, bool scanToEnd = true);		///< Converts string of type char16 to int32 value
	static bool scanInt32 (const tchar* text, int32& value, bool scanToEnd = true);			///< Converts string of type tchar to int32 value

	static bool scanUInt32_8 (const char8* text, uint32& value, bool scanToEnd = true);		///< Converts string of type char8 to int32 value
	static bool scanUInt32_16 (const char16* text, uint32& value, bool scanToEnd = true);		///< Converts string of type char16 to int32 value
	static bool scanUInt32 (const tchar* text, uint32& value, bool scanToEnd = true);			///< Converts string of type tchar to int32 value

	static bool scanHex_8 (const char8* text, uint8& value, bool scanToEnd = true);		///< Converts string of type char8 to hex/unit8 value
	static bool scanHex_16 (const char16* text, uint8& value, bool scanToEnd = true);	///< Converts string of type char16 to hex/unit8 value
	static bool scanHex (const tchar* text, uint8& value, bool scanToEnd = true);		///< Converts string of type tchar to hex/unit8 value
	//-------------------------------------------------------------------------

	// conversion -------------------------------------------------------------
	void toVariant (FVariant& var) const;

	static char8 toLower (char8 c);		///< Converts to lower case
	static char8 toUpper (char8 c);		///< Converts to upper case
	static char16 toLower (char16 c);
	static char16 toUpper (char16 c);

	static int32 multiByteToWideString (char16* dest, const char8* source, int32 wcharCount, uint32 sourceCodePage = kCP_Default);	///< If dest is zero, this returns the maximum number of bytes needed to convert source
	static int32 wideStringToMultiByte (char8* dest, const char16* source, int32 char8Count, uint32 destCodePage = kCP_Default);	///< If dest is zero, this returns the maximum number of bytes needed to convert source

	bool isWideString () const {return isWide != 0;}	///< Returns true if string is wide
	bool isAsciiString () const; 						///< Checks if all characters in string are in ascii range

	bool isNormalized (UnicodeNormalization = kUnicodeNormC); ///< On PC only kUnicodeNormC is working

#if SMTG_OS_WINDOWS
	ConstString (const wchar_t* str, int32 length = -1) : ConstString (wscast (str), length) {}
	operator const wchar_t* () const { return wscast (text16 ());}
#endif

#if SMTG_OS_MACOS
	virtual void* toCFStringRef (uint32 encoding = 0xFFFF, bool mutableCFString = false) const;	///< CFString conversion
#endif
//-------------------------------------------------------------------------

//-----------------------------------------------------------------------------
protected:

	union 
	{
		void* buffer;
		char8* buffer8;
		char16* buffer16;
	};
	uint32 len : 30;
	uint32 isWide : 1;
};

//-----------------------------------------------------------------------------
/** String.
@ingroup adt

Extends class ConstString by operations which allow modifications. 

If allocated externally and passed in via take() or extracted via pass(), memory
is expected to be allocated with C's malloc() (rather than new) and deallocated with free().
Use the NEWSTR8/16 and DELETESTR8/16 macros below.

\see ConstString */
//-----------------------------------------------------------------------------
class String : public ConstString
{
public:
	
//-----------------------------------------------------------------------------
	String ();
	String (const char8* str, MBCodePage codepage, int32 n = -1, bool isTerminated = true);							///< assign n characters of str and convert to wide string by using the specified codepage
	String (const char8* str, int32 n = -1, bool isTerminated = true);	///< assign n characters of str (-1: all)
	String (const char16* str, int32 n = -1, bool isTerminated = true);	///< assign n characters of str (-1: all)
	String (const String& str, int32 n = -1);							///< assign n characters of str (-1: all)
	String (const ConstString& str, int32 n = -1);		///< assign n characters of str (-1: all)
	String (const FVariant& var);						///< assign from FVariant
	String (IString* str);						///< assign from IString
	~String () SMTG_OVERRIDE;

#if SMTG_CPP11_STDLIBSUPPORT
	String (String&& str);
	String& operator= (String&& str);
#endif

	// access------------------------------------------------------------------
	void updateLength (); ///< Call this when the string is truncated outside (not recommended though)
	const char8* text8 () const SMTG_OVERRIDE;
	const char16* text16 () const SMTG_OVERRIDE;
	char8 getChar8 (uint32 index) const SMTG_OVERRIDE;
	char16 getChar16 (uint32 index) const SMTG_OVERRIDE;

	bool setChar8 (uint32 index, char8 c);
	bool setChar16 (uint32 index, char16 c);
	inline bool setChar (uint32 index, char8 c) {return setChar8 (index, c);}
	inline bool setChar (uint32 index, char16 c) {return setChar16 (index, c);}
	//-------------------------------------------------------------------------

	// assignment--------------------------------------------------------------
	String& operator= (const char8* str) {return assign (str);}	///< Assign from a string of type char8
	String& operator= (const char16* str) {return assign (str);}
	String& operator= (const ConstString& str) {return assign (str);}
	String& operator= (const String& str) {return assign (str);}
	String& operator= (char8 c) {return assign (c);}
	String& operator= (char16 c) {return assign (c);}

	String& assign (const ConstString& str, int32 n = -1);			///< Assign n characters of str (-1: all)
	String& assign (const char8* str, int32 n = -1, bool isTerminated = true);			///< Assign n characters of str (-1: all)
	String& assign (const char16* str, int32 n = -1, bool isTerminated = true);			///< Assign n characters of str (-1: all)
	String& assign (char8 c, int32 n = 1);
	String& assign (char16 c, int32 n = 1);
	//-------------------------------------------------------------------------

	// concat------------------------------------------------------------------
	String& append (const ConstString& str, int32 n = -1);		///< Append n characters of str to this (n=-1: all)
	String& append (const char8* str, int32 n = -1);			///< Append n characters of str to this (n=-1: all)
	String& append (const char16* str, int32 n = -1);			///< Append n characters of str to this (n=-1: all)
	String& append (const char8 c, int32 n = 1);                ///< Append char c n times
	String& append (const char16 c, int32 n = 1);               ///< Append char c n times

	String& insertAt (uint32 idx, const ConstString& str, int32 n = -1);	///< Insert n characters of str at position idx (n=-1: all)
	String& insertAt (uint32 idx, const char8* str, int32 n = -1);	///< Insert n characters of str at position idx (n=-1: all)
	String& insertAt (uint32 idx, const char16* str, int32 n = -1);	///< Insert n characters of str at position idx (n=-1: all)
	String& insertAt (uint32 idx, char8 c) {char8 str[] = {c, 0}; return insertAt (idx, str, 1);} 
	String& insertAt (uint32 idx, char16 c) {char16 str[] = {c, 0}; return insertAt (idx, str, 1);}

	String& operator+= (const String& str) {return append (str);}
	String& operator+= (const ConstString& str) {return append (str);}
	String& operator+= (const char8* str) {return append (str);}
	String& operator+= (const char16* str) {return append (str);}
	String& operator+= (const char8 c) {return append (c);}
	String& operator+= (const char16 c) {return append (c);}
	//-------------------------------------------------------------------------

	// replace-----------------------------------------------------------------
	String& replace (uint32 idx, int32 n1, const ConstString& str, int32 n2 = -1);		///< Replace n1 characters of this (starting at idx) with n2 characters of str (n1,n2=-1: until end)
	String& replace (uint32 idx, int32 n1, const char8* str, int32 n2 = -1);			///< Replace n1 characters of this (starting at idx) with n2 characters of str (n1,n2=-1: until end)
	String& replace (uint32 idx, int32 n1, const char16* str, int32 n2 = -1);			///< Replace n1 characters of this (starting at idx) with n2 characters of str (n1,n2=-1: until end)

	int32 replace (const char8* toReplace, const char8* toReplaceWith, bool all = false, CompareMode m = kCaseSensitive);			///< Replace find string with replace string - returns number of replacements
	int32 replace (const char16* toReplace, const char16* toReplaceWith, bool all = false, CompareMode m = kCaseSensitive);		///< Replace find string with replace string - returns number of replacements

	bool replaceChars8 (const char8* toReplace, char8 toReplaceBy); ///< Returns true when any replacement was done
	bool replaceChars16 (const char16* toReplace, char16 toReplaceBy);
	inline bool replaceChars8 (char8 toReplace, char8 toReplaceBy)  {char8 str[] = {toReplace, 0}; return replaceChars8 (str, toReplaceBy);}
	inline bool replaceChars16 (char16 toReplace, char16 toReplaceBy)  {char16 str[] = {toReplace, 0}; return replaceChars16 (str, toReplaceBy);}
	inline bool replaceChars (char8 toReplace, char8 toReplaceBy) {return replaceChars8 (toReplace, toReplaceBy);}
	inline bool replaceChars (char16 toReplace, char16 toReplaceBy) {return replaceChars16 (toReplace, toReplaceBy);}
	inline bool replaceChars (const char8* toReplace, char8 toReplaceBy) {return replaceChars8 (toReplace, toReplaceBy);}
	inline bool replaceChars (const char16* toReplace, char16 toReplaceBy) {return replaceChars16 (toReplace, toReplaceBy);}
	//-------------------------------------------------------------------------

	// remove------------------------------------------------------------------
	String& remove (uint32 index = 0, int32 n = -1);		///< Remove n characters from string starting at index (n=-1: until end)
	enum CharGroup {kSpace, kNotAlphaNum, kNotAlpha};
	bool trim (CharGroup mode = kSpace);					///< Trim lead/trail.
	void removeChars (CharGroup mode = kSpace);				///< Removes all of group.
	bool removeChars8 (const char8* which);					///< Remove all occurrences of each char in 'which'
	bool removeChars16 (const char16* which);				///< Remove all occurrences of each char in 'which'
	inline bool removeChars8 (const char8 which) {char8 str[] = {which, 0}; return removeChars8 (str); }     
	inline bool removeChars16 (const char16 which) {char16 str[] = {which, 0}; return removeChars16 (str); }                
	inline bool removeChars (const char8* which) {return removeChars8 (which);}
	inline bool removeChars (const char16* which) {return removeChars16 (which);}
	inline bool removeChars (const char8 which) {return removeChars8 (which);}
	inline bool removeChars (const char16 which) {return removeChars16 (which);}
	bool removeSubString (const ConstString& subString, bool allOccurences = true);
	//-------------------------------------------------------------------------

	// print-------------------------------------------------------------------
	String& printf (const char8* format, ...);					///< Print formatted data into string
	String& printf (const char16* format, ...);					///< Print formatted data into string
	String& vprintf (const char8* format, va_list args);
	String& vprintf (const char16* format, va_list args);
	//-------------------------------------------------------------------------

	// numbers-----------------------------------------------------------------
	String& printInt64 (int64 value);

	/**
	* @brief				print a float into a string, trailing zeros will be trimmed
	* @param value			the floating value to be printed
	* @param maxPrecision	(optional) the max precision allowed for this, num of significant digits after the comma
	*						For instance printFloat (1.234, 2) => 1.23
	*  @return				the resulting string.
	*/
	String& printFloat (double value, uint32 maxPrecision = 6);
	/** Increment the trailing number if present else start with minNumber, width specifies the string width format (width 2 for number 3 is 03),
		applyOnlyFormat set to true will only format the string to the given width without incrementing the founded trailing number */
	bool incrementTrailingNumber (uint32 width = 2, tchar separator = STR (' '), uint32 minNumber = 1, bool applyOnlyFormat = false);
	//-------------------------------------------------------------------------

	// conversion--------------------------------------------------------------
	bool fromVariant (const FVariant& var);		///< Assigns string from FVariant
	void toVariant (FVariant& var) const;
	bool fromAttributes (IAttributes* a, IAttrID attrID);	///< Assigns string from FAttributes
	bool toAttributes (IAttributes* a, IAttrID attrID);

	void swapContent (String& s); 								///< Swaps ownership of the strings pointed to
	void take (String& str);      								///< Take ownership of the string of 'str'
	void take (void* _buffer, bool wide);      					///< Take ownership of buffer
	void* pass ();
	void passToVariant (FVariant& var);							///< Pass ownership of buffer to Variant - sets Variant ownership

	void toLower (uint32 index);								///< Lower case the character.
	void toLower ();											///< Lower case the string.
	void toUpper (uint32 index);								///< Upper case the character.
	void toUpper ();											///< Upper case the string.

	unsigned char* toPascalString (unsigned char* buf);			///< Pascal string conversion
	const String& fromPascalString (const unsigned char* buf);	///< Pascal string conversion

	bool toWideString (uint32 sourceCodePage = kCP_Default);	///< Converts to wide string according to sourceCodePage
	bool toMultiByte (uint32 destCodePage = kCP_Default);

	void fromUTF8 (const char8* utf8String);				///< Assigns from UTF8 string
	bool normalize (UnicodeNormalization = kUnicodeNormC);	///< On PC only kUnicodeNormC is working

#if SMTG_OS_WINDOWS
	String (const wchar_t* str, int32 length = -1, bool isTerminated = true) : String (wscast (str), length, isTerminated) {}
	String& operator= (const wchar_t* str) {return String::operator= (wscast (str)); }
#endif

#if SMTG_OS_MACOS
	virtual bool fromCFStringRef (const void*, uint32 encoding = 0xFFFF);	///< CFString conversion
#endif
	//-------------------------------------------------------------------------

	//-----------------------------------------------------------------------------
protected:
	bool resize (uint32 newSize, bool wide, bool fill = false);

private:
	bool _toWideString (const char8* src, int32 length, uint32 sourceCodePage = kCP_Default);
	void tryFreeBuffer ();
	bool checkToMultiByte (uint32 destCodePage = kCP_Default) const; // to remove debug code from inline - const_cast inside!!!
};

// String memory allocation macros
#define NEWSTR8(len)   ((char8*)::malloc(len))      // len includes trailing zero
#define NEWSTR16(len)  ((char16*)::malloc(2*(len)))
#define DELETESTR8(p)  (::free((char*)(p)))         // cast to strip const
#define DELETESTR16(p) (::free((char16*)(p)))

// String concatenation functions.
inline String operator+ (const ConstString& s1, const ConstString& s2) {return String (s1).append (s2);}
inline String operator+ (const ConstString& s1, const char8* s2) {return String (s1).append (s2);}
inline String operator+ (const ConstString& s1, const char16* s2) {return String (s1).append (s2);}
inline String operator+ (const char8* s1, const ConstString& s2) {return String (s1).append (s2);}
inline String operator+ (const char16* s1, const ConstString& s2) {return String (s1).append (s2);}
inline String operator+ (const ConstString& s1, const String& s2) {return String (s1).append (s2);}
inline String operator+ (const String& s1, const ConstString& s2) {return String (s1).append (s2);}
inline String operator+ (const String& s1, const String& s2) {return String (s1).append (s2);}
inline String operator+ (const String& s1, const char8* s2) {return String (s1).append (s2);}
inline String operator+ (const String& s1, const char16* s2) {return String (s1).append (s2);}
inline String operator+ (const char8* s1, const String& s2) {return String (s1).append (s2);}
inline String operator+ (const char16* s1, const String& s2) {return String (s1).append (s2);}

//-----------------------------------------------------------------------------
// ConstString
//-----------------------------------------------------------------------------
inline const tchar* ConstString::text () const
{
#ifdef UNICODE
	return text16 ();
#else
	return text8 ();
#endif
}

//-----------------------------------------------------------------------------
inline const char8* ConstString::text8 () const
{
	return (!isWide && buffer8) ? buffer8: kEmptyString8;
}

//-----------------------------------------------------------------------------
inline const char16* ConstString::text16 () const
{
	return (isWide && buffer16) ? buffer16 : kEmptyString16;
}

//-----------------------------------------------------------------------------
inline char8 ConstString::getChar8 (uint32 index) const
{
	if (index < len && buffer8 && !isWide)
		return buffer8[index];
	return 0;
}

//-----------------------------------------------------------------------------
inline char16 ConstString::getChar16 (uint32 index) const
{
	if (index < len && buffer16 && isWide)
		return buffer16[index];
	return 0;
}

//-----------------------------------------------------------------------------
inline tchar ConstString::getChar (uint32 index) const
{
#ifdef UNICODE
	return getChar16 (index);
#else
	return getChar8 (index);
#endif
}

//-----------------------------------------------------------------------------
inline tchar ConstString::getCharAt (uint32 index) const
{
#ifdef UNICODE
	if (isWide)
		return getChar16 (index);
#endif

	return static_cast<tchar> (getChar8 (index));
}

//-----------------------------------------------------------------------------
inline int64 ConstString::getNumber () const
{
	int64 tmp = 0;
	scanInt64 (tmp);
	return tmp;
}

//-----------------------------------------------------------------------------
inline bool ConstString::scanInt32_8 (const char8* text, int32& value, bool scanToEnd)
{
	int64 tmp;
	if (scanInt64_8 (text, tmp, scanToEnd))
	{
		value = (int32)tmp;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
inline bool ConstString::scanInt32_16 (const char16* text, int32& value, bool scanToEnd)
{
	int64 tmp;
	if (scanInt64_16 (text, tmp, scanToEnd))
	{
		value = (int32)tmp;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
inline bool ConstString::scanInt32 (const tchar* text, int32& value, bool scanToEnd)
{
	int64 tmp;
	if (scanInt64 (text, tmp, scanToEnd))
	{
		value = (int32)tmp;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
inline bool ConstString::scanUInt32_8 (const char8* text, uint32& value, bool scanToEnd)
{
	uint64 tmp;
	if (scanUInt64_8 (text, tmp, scanToEnd))
	{
		value = (uint32)tmp;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
inline bool ConstString::scanUInt32_16 (const char16* text, uint32& value, bool scanToEnd)
{
	uint64 tmp;
	if (scanUInt64_16 (text, tmp, scanToEnd))
	{
		value = (uint32)tmp;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
inline bool ConstString::scanUInt32 (const tchar* text, uint32& value, bool scanToEnd)
{
	uint64 tmp;
	if (scanUInt64 (text, tmp, scanToEnd))
	{
		value = (uint32)tmp;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
inline const char8* String::text8 () const
{
	if (isWide && !isEmpty ())
		checkToMultiByte (); // this should be avoided, since it can lead to information loss

	return ConstString::text8 ();
}

//-----------------------------------------------------------------------------
inline const char16* String::text16 () const
{
	if (!isWide && !isEmpty ())
	{
		const_cast<String&> (*this).toWideString ();
	}
	return ConstString::text16 ();
}

//-----------------------------------------------------------------------------
inline char8 String::getChar8 (uint32 index) const
{
	if (isWide && !isEmpty ())
		checkToMultiByte (); // this should be avoided, since it can lead to information loss

	return ConstString::getChar8 (index);
}

//-----------------------------------------------------------------------------
inline char16 String::getChar16 (uint32 index) const
{
	if (!isWide && !isEmpty ())
	{
		const_cast<String&> (*this).toWideString ();
	}
	return ConstString::getChar16 (index);
}

//-----------------------------------------------------------------------------


inline bool operator<  (const ConstString& s1, const ConstString& s2) {return (s1.compare (s2) < 0) ? true : false;}
inline bool operator<= (const ConstString& s1, const ConstString& s2) {return (s1.compare (s2) <= 0) ? true : false;}
inline bool operator>  (const ConstString& s1, const ConstString& s2) {return (s1.compare (s2) > 0) ? true : false;}
inline bool operator>= (const ConstString& s1, const ConstString& s2) {return (s1.compare (s2) >= 0) ? true : false;}
inline bool operator== (const ConstString& s1, const ConstString& s2) {return (s1.compare (s2) == 0) ? true : false;}
inline bool operator!= (const ConstString& s1, const ConstString& s2) {return (s1.compare (s2) != 0) ? true : false;}

inline bool operator<  (const ConstString& s1, const char8* s2) {return (s1.compare (s2) < 0) ? true : false;}
inline bool operator<= (const ConstString& s1, const char8* s2) {return (s1.compare (s2) <= 0) ? true : false;}
inline bool operator>  (const ConstString& s1, const char8* s2) {return (s1.compare (s2) > 0) ? true : false;}
inline bool operator>= (const ConstString& s1, const char8* s2) {return (s1.compare (s2) >= 0) ? true : false;}
inline bool operator== (const ConstString& s1, const char8* s2) {return (s1.compare (s2) == 0) ? true : false;}
inline bool operator!= (const ConstString& s1, const char8* s2) {return (s1.compare (s2) != 0) ? true : false;}
inline bool operator<  (const char8* s1, const ConstString& s2) {return (s2.compare (s1) > 0) ? true : false;}
inline bool operator<= (const char8* s1, const ConstString& s2) {return (s2.compare (s1) >= 0) ? true : false;}
inline bool operator>  (const char8* s1, const ConstString& s2) {return (s2.compare (s1) < 0) ? true : false;}
inline bool operator>= (const char8* s1, const ConstString& s2) {return (s2.compare (s1) <= 0) ? true : false;}
inline bool operator== (const char8* s1, const ConstString& s2) {return (s2.compare (s1) == 0) ? true : false;}
inline bool operator!= (const char8* s1, const ConstString& s2) {return (s2.compare (s1) != 0) ? true : false;}

inline bool operator<  (const ConstString& s1, const char16* s2) {return (s1.compare (s2) < 0) ? true : false;}
inline bool operator<= (const ConstString& s1, const char16* s2) {return (s1.compare (s2) <= 0) ? true : false;}
inline bool operator>  (const ConstString& s1, const char16* s2) {return (s1.compare (s2) > 0) ? true : false;}
inline bool operator>= (const ConstString& s1, const char16* s2) {return (s1.compare (s2) >= 0) ? true : false;}
inline bool operator== (const ConstString& s1, const char16* s2) {return (s1.compare (s2) == 0) ? true : false;}
inline bool operator!= (const ConstString& s1, const char16* s2) {return (s1.compare (s2) != 0) ? true : false;}
inline bool operator<  (const char16* s1, const ConstString& s2) {return (s2.compare (s1) > 0) ? true : false;}
inline bool operator<= (const char16* s1, const ConstString& s2) {return (s2.compare (s1) >= 0) ? true : false;}
inline bool operator>  (const char16* s1, const ConstString& s2) {return (s2.compare (s1) < 0) ? true : false;}
inline bool operator>= (const char16* s1, const ConstString& s2) {return (s2.compare (s1) <= 0) ? true : false;}
inline bool operator== (const char16* s1, const ConstString& s2) {return (s2.compare (s1) == 0) ? true : false;}
inline bool operator!= (const char16* s1, const ConstString& s2) {return (s2.compare (s1) != 0) ? true : false;}

// The following functions will only work with European Numbers!
// (e.g. Arabic, Tibetan, and Khmer digits are not supported)
extern int32 strnatcmp8 (const char8* s1, const char8* s2, bool caseSensitive = true);
extern int32 strnatcmp16 (const char16* s1, const char16* s2, bool caseSensitive = true);
inline int32 strnatcmp (const tchar* s1, const tchar* s2, bool caseSensitive = true)
{
#ifdef UNICODE
	return strnatcmp16 (s1, s2, caseSensitive);
#else
	return strnatcmp8 (s1, s2, caseSensitive);
#endif
}

//-----------------------------------------------------------------------------
/** StringObject implements IStringResult and IString methods.
    It can therefore be exchanged with other Steinberg objects using one or both of these
interfaces.

\see String, ConstString
*/
//-----------------------------------------------------------------------------
class StringObject : public FObject, public String, public IStringResult, public IString
{
public:
//-----------------------------------------------------------------------------
	StringObject () {}										
	StringObject (const char16* str, int32 n = -1, bool isTerminated = true) : String (str, n, isTerminated) {}
	StringObject (const char8* str, int32 n = -1, bool isTerminated = true) : String (str, n, isTerminated) {}
	StringObject (const StringObject& str, int32 n = -1) : String (str, n) {}		
	StringObject (const String& str, int32 n = -1) : String (str, n) {}		
	StringObject (const FVariant& var) : String (var) {}		

	using String::operator=;

	// IStringResult ----------------------------------------------------------
	void PLUGIN_API setText (const char8* text) SMTG_OVERRIDE;
	//-------------------------------------------------------------------------

	// IString-----------------------------------------------------------------
	void PLUGIN_API setText8 (const char8* text) SMTG_OVERRIDE;
	void PLUGIN_API setText16 (const char16* text) SMTG_OVERRIDE;

	const char8* PLUGIN_API getText8 () SMTG_OVERRIDE;
	const char16* PLUGIN_API getText16 () SMTG_OVERRIDE;

	void PLUGIN_API take (void* s, bool _isWide) SMTG_OVERRIDE;
	bool PLUGIN_API isWideString () const SMTG_OVERRIDE;
	//-------------------------------------------------------------------------

	OBJ_METHODS (StringObject, FObject)
	FUNKNOWN_METHODS2 (IStringResult, IString, FObject)
};

//------------------------------------------------------------------------
} // namespace Steinberg
