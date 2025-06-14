//------------------------------------------------------------------------
// Project     : SDK Base
// Version     : 1.0
//
// Category    : Helpers
// Filename    : base/source/classfactoryhelpers.h
// Created by  : Steinberg, 03/2017
// Description : Class factory
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

//------------------------------------------------------------------------------
// Helper Macros. Not intended for direct use.
// Use:
//	META_CLASS(className),
//	META_CLASS_IFACE(className,Interface),
//	META_CLASS_SINGLE(className,Interface)
// instead.
//------------------------------------------------------------------------------
#define META_CREATE_FUNC(funcName) static FUnknown* funcName ()

#define CLASS_CREATE_FUNC(className)                                               \
	namespace Meta {                                                               \
	META_CREATE_FUNC (make##className) { return (NEW className)->unknownCast (); } \
	}

#define SINGLE_CREATE_FUNC(className)                                                     \
	namespace Meta {                                                                      \
	META_CREATE_FUNC (make##className) { return className::instance ()->unknownCast (); } \
	}

#define _META_CLASS(className)                                                         \
	namespace Meta {                                                                   \
	static Steinberg::MetaClass meta##className ((#className), Meta::make##className); \
	}

#define _META_CLASS_IFACE(className, Interface)                                                  \
	namespace Meta {                                                                             \
	static Steinberg::MetaClass meta##Interface##className ((#className), Meta::make##className, \
	                                                        Interface##_iid);                    \
	}

/** TODO
 */
#define META_CLASS(className)     \
	CLASS_CREATE_FUNC (className) \
	_META_CLASS (className)

/** TODO
 */
#define META_CLASS_IFACE(className, Interface) \
	CLASS_CREATE_FUNC (className)              \
	_META_CLASS_IFACE (className, Interface)

/** TODO
 */
#define META_CLASS_SINGLE(className, Interface) \
	SINGLE_CREATE_FUNC (className)              \
	_META_CLASS_IFACE (className, Interface)
