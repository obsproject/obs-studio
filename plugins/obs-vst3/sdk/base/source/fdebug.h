//------------------------------------------------------------------------
// Project     : SDK Base
// Version     : 1.0
//
// Category    : Helpers
// Filename    : base/source/fdebug.h
// Created by  : Steinberg, 1995
// Description : There are 2 levels of debugging messages:
//	             DEVELOPMENT               During development
//	             RELEASE                   Program is shipping.
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
/** @file base/source/fdebug.h
	Debugging tools.

	There are 2 levels of debugging messages:
	- DEVELOPMENT
	  - During development
	- RELEASE
	  - Program is shipping.
*/
//-----------------------------------------------------------------------------
#pragma once

#include "pluginterfaces/base/ftypes.h"
#include <cstring>

#if SMTG_OS_MACOS
#include <new>
#endif

/** Returns true if a debugger is attached. */
bool AmIBeingDebugged ();

//-----------------------------------------------------------------------------
// development / release
//-----------------------------------------------------------------------------
#if !defined (DEVELOPMENT) && !defined (RELEASE) 
	#ifdef _DEBUG
		#define DEVELOPMENT 1
	#elif defined (NDEBUG)
		#define RELEASE 1
	#else
		#error DEVELOPMENT, RELEASE, _DEBUG, or NDEBUG  must be defined!
	#endif
#endif

//-----------------------------------------------------------------------------
#if SMTG_OS_WINDOWS

/** Disable compiler warning:
 * C4291: "No matching operator delete found; memory will not be freed if initialization throws an
 * exception. A placement new is used for which there is no placement delete." */
#if DEVELOPMENT && defined(_MSC_VER)
#pragma warning(disable : 4291)
#pragma warning(disable : 4985)
#endif

#endif // SMTG_OS_WINDOWS

#if DEVELOPMENT
//-----------------------------------------------------------------------------
/** If "f" is not true and a debugger is present, send an error string to the debugger for display
   and cause a breakpoint exception to occur in the current process. SMTG_ASSERT is removed
   completely in RELEASE configuration. So do not pass methods calls to this macro that are expected
   to exist in the RELEASE build (for method calls that need to be present in a RELEASE build, use
   the VERIFY macros instead)*/
#define SMTG_ASSERT(f) \
	if (!(f))          \
		FDebugBreak ("%s(%d) : Assert failed: %s\n", __FILE__, __LINE__, #f);

#define SMTG_ASSERT_MSG(f, msg) \
	if (!(f))          \
		FDebugBreak ("%s(%d) : Assert failed: [%s] [%s]\n", __FILE__, __LINE__, #f, msg);

/** Send "comment" string to the debugger for display. */
#define SMTG_WARNING(comment) FDebugPrint ("%s(%d) : %s\n", __FILE__, __LINE__, comment);

/** Send the last error string to the debugger for display. */
#define SMTG_PRINTSYSERROR FPrintLastError (__FILE__, __LINE__);

/** If a debugger is present, send string "s" to the debugger for display and
    cause a breakpoint exception to occur in the current process. */
#define SMTG_DEBUGSTR(s) FDebugBreak (s);

/** Use VERIFY for calling methods "f" having a bool result (expecting them to return 'true')
     The call of "f" is not removed in RELEASE builds, only the result verification. eg: SMTG_VERIFY
   (isValid ()) */
#define SMTG_VERIFY(f) SMTG_ASSERT (f)

/** Use VERIFY_IS for calling methods "f" and expect a certain result "r".
    The call of "f" is not removed in RELEASE builds, only the result verification. eg:
   SMTG_VERIFY_IS (callMethod (), kResultOK) */
#define SMTG_VERIFY_IS(f, r) \
	if ((f) != (r))          \
		FDebugBreak ("%s(%d) : Assert failed: %s\n", __FILE__, __LINE__, #f);

/** Use VERIFY_NOT for calling methods "f" and expect the result to be anything else but "r".
     The call of "f" is not removed in RELEASE builds, only the result verification. eg:
   SMTG_VERIFY_NOT (callMethod (), kResultError) */
#define SMTG_VERIFY_NOT(f, r) \
	if ((f) == (r))           \
		FDebugBreak ("%s(%d) : Assert failed: %s\n", __FILE__, __LINE__, #f);

/** @name Shortcut macros for sending strings to the debugger for display.
	First parameter is always the format string (printf like).
*/

///@{
#define SMTG_DBPRT0(a) FDebugPrint (a);
#define SMTG_DBPRT1(a, b) FDebugPrint (a, b);
#define SMTG_DBPRT2(a, b, c) FDebugPrint (a, b, c);
#define SMTG_DBPRT3(a, b, c, d) FDebugPrint (a, b, c, d);
#define SMTG_DBPRT4(a, b, c, d, e) FDebugPrint (a, b, c, d, e);
#define SMTG_DBPRT5(a, b, c, d, e, f) FDebugPrint (a, b, c, d, e, f);
///@}

/** @name Helper functions for the above defined macros.

    You shouldn't use them directly (if you do so, don't forget "#if DEVELOPMENT")!
    It is recommended to use the macros instead.
*/
///@{
void FDebugPrint (const char* format, ...);
void FDebugBreak (const char* format, ...);
void FPrintLastError (const char* file, int line);
///@}

/** @name Provide a custom assertion handler and debug print handler, eg
        so that we can provide an assert with a custom dialog, or redirect
        the debug output to a file or stream.
*/
///@{
using AssertionHandler = bool (*) (const char* message);
extern AssertionHandler gAssertionHandler;
extern AssertionHandler gPreAssertionHook;
using DebugPrintLogger = void (*) (const char* message);
extern DebugPrintLogger gDebugPrintLogger;
///@}

/** Definition of memory allocation macros:
    Use "NEW" to allocate storage for individual objects.
    Use "NEWVEC" to allocate storage for an array of objects. */
#if SMTG_OS_MACOS
void* operator new (size_t, int, const char*, int);
void* operator new[] (size_t, int, const char*, int);
void operator delete (void* p, int, const char* file, int line);
void operator delete[] (void* p, int, const char* file, int line);
#ifndef NEW
#define NEW new (1, __FILE__, __LINE__)
#define NEWVEC new (1, __FILE__, __LINE__)
#endif

#define DEBUG_NEW DEBUG_NEW_LEAKS

#elif SMTG_OS_WINDOWS && defined(_MSC_VER)
#ifndef NEW
void* operator new (size_t, int, const char*, int);
#define NEW new (1, __FILE__, __LINE__)
#define NEWVEC new (1, __FILE__, __LINE__)
#endif

#else
#ifndef NEW
#define NEW new
#define NEWVEC new
#endif
#endif

#else
/** if DEVELOPMENT is not set, these macros will do nothing. */
#define SMTG_ASSERT(f)
#define SMTG_ASSERT_MSG(f, msg)
#define SMTG_WARNING(s)
#define SMTG_PRINTSYSERROR
#define SMTG_DEBUGSTR(s)
#define SMTG_VERIFY(f) f;
#define SMTG_VERIFY_IS(f, r) f;
#define SMTG_VERIFY_NOT(f, r) f;

#define SMTG_DBPRT0(a)
#define SMTG_DBPRT1(a, b)
#define SMTG_DBPRT2(a, b, c)
#define SMTG_DBPRT3(a, b, c, d)
#define SMTG_DBPRT4(a, b, c, d, e)
#define SMTG_DBPRT5(a, b, c, d, e, f)

#ifndef NEW
#define NEW new
#define NEWVEC new

#endif
#endif

// replace #if SMTG_CPPUNIT_TESTING
bool isSmtgUnitTesting ();
void setSmtgUnitTesting ();

#if !SMTG_RENAME_ASSERT
#if SMTG_OS_WINDOWS
#undef ASSERT
#endif

#define ASSERT				SMTG_ASSERT
#define WARNING				SMTG_WARNING
#define DEBUGSTR			SMTG_DEBUGSTR
#define VERIFY				SMTG_VERIFY
#define VERIFY_IS			SMTG_VERIFY_IS
#define VERIFY_NOT			SMTG_VERIFY_NOT
#define PRINTSYSERROR		SMTG_PRINTSYSERROR

#define DBPRT0				SMTG_DBPRT0
#define DBPRT1				SMTG_DBPRT1
#define DBPRT2				SMTG_DBPRT2
#define DBPRT3				SMTG_DBPRT3
#define DBPRT4				SMTG_DBPRT4
#define DBPRT5				SMTG_DBPRT5
#endif
