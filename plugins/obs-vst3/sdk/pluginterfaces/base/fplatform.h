//-----------------------------------------------------------------------------
// Project     : SDK Core
//
// Category    : SDK Core Interfaces
// Filename    : pluginterfaces/base/fplatform.h
// Created by  : Steinberg, 01/2004
// Description : Detect platform and set define
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

// values for BYTEORDER according to the used platform
#define kLittleEndian	0
#define kBigEndian		1

#undef PLUGIN_API

#if !defined (__INTEL_CXX11_MODE__)
#define SMTG_INTEL_CXX11_MODE 0
#else
#define SMTG_INTEL_CXX11_MODE __INTEL_CXX11_MODE__
#endif

#if !defined (__INTEL_COMPILER)
#define SMTG_INTEL_COMPILER 0
#else
#define SMTG_INTEL_COMPILER __INTEL_COMPILER
#endif

//-----------------------------------------------------------------------------
// WIN32 AND WIN64 (WINDOWS)
//-----------------------------------------------------------------------------
#if defined (_WIN32)
	//-----------------------------------------------------------------------------
	#define SMTG_OS_LINUX	0
	#define SMTG_OS_MACOS	0
	#define SMTG_OS_WINDOWS	1
	#define SMTG_OS_IOS		0
	#define SMTG_OS_OSX		0

	#if defined (_M_IX86)
		#define SMTG_CPU_X86	1
	#else
		#define SMTG_CPU_X86	0
	#endif
	#if defined (_M_AMD64)
		#define SMTG_CPU_X86_64	1
	#else
		#define SMTG_CPU_X86_64	0
	#endif
	#if defined (_M_ARM)
		#define SMTG_CPU_ARM	1
	#else
		#define SMTG_CPU_ARM	0
	#endif
	#if defined (_M_ARM64)
		#define SMTG_CPU_ARM_64	1
	#else
		#define SMTG_CPU_ARM_64	0
	#endif
	#if defined (_M_ARM64EC)
		#define SMTG_CPU_ARM_64EC	1
	#else
		#define SMTG_CPU_ARM_64EC	0
	#endif

	#define SMTG_OS_WINDOWS_ARM	(SMTG_CPU_ARM_64EC || SMTG_CPU_ARM_64 || SMTG_CPU_ARM)

	#define BYTEORDER kLittleEndian
	
	#define COM_COMPATIBLE	1
	#define PLUGIN_API __stdcall
	#define SMTG_PTHREADS	0

	#define SMTG_EXPORT_SYMBOL __declspec (dllexport)
	#define SMTG_HIDDEN_SYMBOL

	#ifdef _MSC_VER
		#if !defined (SMTG_DISABLE_DEFAULT_DIAGNOSTICS)
			#pragma warning (disable : 4244) //! @brief warning C4244: Conversion from 'type1' to 'type2', possible loss of data.
			#pragma warning (disable : 4250) //! @brief warning C4250: Inheritance via dominance is allowed

			#pragma warning (3 : 4189) //! @brief warning C4189: local variable is initialized but not referenced
			#pragma warning (3 : 4238) //! @brief warning C4238: nonstandard extension used: class rvalue used as lvalue
		#endif
	#endif // _MSC_VER

	#if defined (_WIN64) || defined (_M_ARM64)
		#define SMTG_PLATFORM_64 1
	#else
		#define SMTG_PLATFORM_64 0
	#endif

	#ifndef WIN32
		#define WIN32	1
	#endif

	#ifdef __cplusplus
		#define SMTG_CPP11 (__cplusplus >= 201103L || _MSC_VER > 1600 || SMTG_INTEL_CXX11_MODE)
		#define SMTG_CPP11_STDLIBSUPPORT SMTG_CPP11
		#define SMTG_CPP14 (__cplusplus >= 201402L || ((_MSC_FULL_VER >= 190024210L) && (_MSVC_LANG >= 201402L)))
		#define SMTG_CPP17 (__cplusplus >= 201703L || ((_MSC_FULL_VER >= 190024210L) && (_MSVC_LANG >= 201703L)))
		#define SMTG_CPP20 (__cplusplus >= 202002L)
		#define SMTG_HAS_NOEXCEPT ((_MSC_FULL_VER >= 190023026L) || (SMTG_INTEL_CXX11_MODE && SMTG_INTEL_COMPILER >= 1300))
		#if ((_MSC_FULL_VER >= 190024210L) || (SMTG_INTEL_CXX11_MODE && SMTG_INTEL_COMPILER >= 1500) || (defined (__MINGW32__) && SMTG_CPP11))
			#define SMTG_HAS_CPP11_CONSTEXPR 1
		#else
			#define SMTG_HAS_CPP11_CONSTEXPR 0
		#endif
		#if (((_MSC_VER >= 1915L) && (_MSVC_LANG >= 201402L)) || (SMTG_INTEL_CXX11_MODE && SMTG_INTEL_COMPILER > 1700) || (defined (__MINGW32__) && SMTG_CPP14))
			#define SMTG_HAS_CPP14_CONSTEXPR 1
		#else
			#define SMTG_HAS_CPP14_CONSTEXPR 0
		#endif
	#endif //__cplusplus

	#define SMTG_DEPRECATED_ATTRIBUTE(message) __declspec (deprecated ("Is Deprecated: " message))
//-----------------------------------------------------------------------------
// LINUX
//-----------------------------------------------------------------------------
#elif (defined (__gnu_linux__) && __gnu_linux__) || (defined (__linux__) && __linux__)
	#define SMTG_OS_LINUX		1
	#define SMTG_OS_MACOS		0
	#define SMTG_OS_WINDOWS		0
	#define SMTG_OS_WINDOWS_ARM	0
	#define SMTG_OS_IOS			0
	#define SMTG_OS_OSX			0

	#define SMTG_CPU_X86	__i386__
	#define SMTG_CPU_X86_64	__x86_64__
	#if defined (__arm__)
		#define SMTG_CPU_ARM	__arm__
	#else
		#define SMTG_CPU_ARM	0
	#endif
	#if defined (__aarch64__)
		#define SMTG_CPU_ARM_64	__aarch64__
	#else
		#define SMTG_CPU_ARM_64	0
	#endif
	#define SMTG_CPU_ARM_64EC 0

	#include <endian.h>
	#if __BYTE_ORDER == __LITTLE_ENDIAN
		#define BYTEORDER kLittleEndian
	#else
		#define BYTEORDER kBigEndian
	#endif

	#define COM_COMPATIBLE	0
	#define PLUGIN_API
	#define SMTG_PTHREADS	1

	#define SMTG_EXPORT_SYMBOL __attribute__ ((visibility ("default")))
	#define SMTG_HIDDEN_SYMBOL __attribute__ ((visibility ("hidden")))

	#if __LP64__
		#define SMTG_PLATFORM_64 1
	#else
		#define SMTG_PLATFORM_64 0
	#endif
	#ifdef __cplusplus
		#include <cstddef>
		#define SMTG_CPP11 (__cplusplus >= 201103L)
		#ifndef SMTG_CPP11
			#error unsupported compiler
		#endif
		#define SMTG_CPP14 (__cplusplus >= 201402L)
		#define SMTG_CPP17 (__cplusplus >= 201703L)
		#define SMTG_CPP20 (__cplusplus >= 202002L)
		#if defined (__GNUG__) && __GNUG__ < 8
			#define SMTG_CPP11_STDLIBSUPPORT 0
		#else
			#define SMTG_CPP11_STDLIBSUPPORT 1
		#endif
		#define SMTG_HAS_NOEXCEPT 1
		#define SMTG_HAS_CPP11_CONSTEXPR SMTG_CPP11
		#define SMTG_HAS_CPP14_CONSTEXPR SMTG_CPP14
	#endif // __cplusplus
//-----------------------------------------------------------------------------
// Mac and iOS
//-----------------------------------------------------------------------------
#elif __APPLE__
	#include <TargetConditionals.h>
	#define SMTG_OS_LINUX		0
	#define SMTG_OS_MACOS		1
	#define SMTG_OS_WINDOWS		0
	#define SMTG_OS_WINDOWS_ARM	0
	#define SMTG_OS_IOS			TARGET_OS_IPHONE
	#define SMTG_OS_OSX			TARGET_OS_MAC && !TARGET_OS_IPHONE

	#define SMTG_CPU_X86		TARGET_CPU_X86
	#define SMTG_CPU_X86_64		TARGET_CPU_X86_64
	#define SMTG_CPU_ARM		TARGET_CPU_ARM
	#define SMTG_CPU_ARM_64		TARGET_CPU_ARM64
	#define SMTG_CPU_ARM_64EC 	0

	#if !SMTG_OS_IOS
		#ifndef __CF_USE_FRAMEWORK_INCLUDES__
			#define __CF_USE_FRAMEWORK_INCLUDES__
		#endif
		#ifndef TARGET_API_MAC_CARBON
			#define TARGET_API_MAC_CARBON 1
		#endif
	#endif
	#if __LP64__
		#define SMTG_PLATFORM_64 1
	#else
		#define SMTG_PLATFORM_64 0
	#endif
	#if defined (__BIG_ENDIAN__)
		#define BYTEORDER kBigEndian
	#else
		#define BYTEORDER kLittleEndian
	#endif

	#define COM_COMPATIBLE	0
	#define PLUGIN_API
	#define SMTG_PTHREADS	1

	#define SMTG_EXPORT_SYMBOL __attribute__ ((visibility ("default")))
	#define SMTG_HIDDEN_SYMBOL __attribute__ ((visibility ("hidden")))

	#if !defined (__PLIST__) && !defined (SMTG_DISABLE_DEFAULT_DIAGNOSTICS)
		#ifdef __clang__
			#pragma GCC diagnostic ignored "-Wswitch-enum"
			#pragma GCC diagnostic ignored "-Wparentheses"
			#pragma GCC diagnostic ignored "-Wuninitialized"
			#if __clang_major__ >= 3
				#pragma GCC diagnostic ignored "-Wtautological-compare"
				#pragma GCC diagnostic ignored "-Wunused-value"
				#if __clang_major__ >= 4 || __clang_minor__ >= 1
					#pragma GCC diagnostic ignored "-Wswitch"
					#pragma GCC diagnostic ignored "-Wcomment"
				#endif
				#if __clang_major__ >= 5
					#pragma GCC diagnostic ignored "-Wunsequenced"
					#if __clang_minor__ >= 1
						#pragma GCC diagnostic ignored "-Wunused-const-variable"
					#endif
				#endif
			#endif
		#endif
	#endif
	#ifdef __cplusplus
		#include <cstddef>
		#define SMTG_CPP11 (__cplusplus >= 201103L || SMTG_INTEL_CXX11_MODE)
		#define SMTG_CPP14 (__cplusplus >= 201402L)
		#define SMTG_CPP17 (__cplusplus >= 201703L)
		#define SMTG_CPP20 (__cplusplus >= 202002L)
		#if defined (_LIBCPP_VERSION) && SMTG_CPP11
			#define SMTG_CPP11_STDLIBSUPPORT 1
			#define SMTG_HAS_NOEXCEPT 1
		#else
			#define SMTG_CPP11_STDLIBSUPPORT 0
			#define SMTG_HAS_NOEXCEPT 0
		#endif
		#define SMTG_HAS_CPP11_CONSTEXPR SMTG_CPP11
		#define SMTG_HAS_CPP14_CONSTEXPR SMTG_CPP14
	#endif // __cplusplus
//-----------------------------------------------------------------------------
// Unknown Platform
//-----------------------------------------------------------------------------
#else
	#pragma error unknown platform
#endif

//-----------------------------------------------------------------------------
#if !SMTG_RENAME_ASSERT
#undef WINDOWS
#undef MAC
#undef PTHREADS
#undef PLATFORM_64

#if SMTG_OS_WINDOWS
#define WINDOWS			SMTG_OS_WINDOWS
#endif
#if SMTG_OS_MACOS
#define MAC				SMTG_OS_MACOS
#endif
#define PLATFORM_64		SMTG_PLATFORM_64
#define PTHREADS		SMTG_PTHREADS
#endif
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
#if SMTG_CPP11
#define SMTG_OVERRIDE override
#else
#define SMTG_OVERRIDE
#endif

#if SMTG_HAS_CPP11_CONSTEXPR
#define SMTG_CONSTEXPR constexpr
#else
#define SMTG_CONSTEXPR
#endif

#if SMTG_HAS_CPP14_CONSTEXPR
#define SMTG_CONSTEXPR14 constexpr
#else
#define SMTG_CONSTEXPR14
#endif

#if SMTG_HAS_NOEXCEPT
#define SMTG_NOEXCEPT noexcept
#else
#define SMTG_NOEXCEPT
#endif

//-----------------------------------------------------------------------------
// Deprecation setting
//-----------------------------------------------------------------------------
#ifndef SMTG_DEPRECATED_ATTRIBUTE
#if SMTG_CPP17
#define SMTG_DEPRECATED_ATTRIBUTE(msg) [[deprecated(msg)]]
#else
#define SMTG_DEPRECATED_ATTRIBUTE(msg)
#endif
#endif

#define SMTG_DEPRECATED_MSG(msg) SMTG_DEPRECATED_ATTRIBUTE(msg)
//-----------------------------------------------------------------------------
