//------------------------------------------------------------------------
// Project     : SDK Base
// Version     : 1.0
//
// Category    : Helpers
// Filename    : base/source/fstdmethods.h
// Created by  : Steinberg, 2007
// Description : Convenient macros to create setter and getter methods.
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------
/** @file base/source/fstdmethods.h
	Convenient macros to create setter and getter methods. */
//------------------------------------------------------------------------
#pragma once

//----------------------------------------------------------------------------------
/**	@name Methods for flags.
	Macros to create setter and getter methods for flags.

	Usage example with DEFINE_STATE: 
										\code
	class MyClass
	{
	public:
		MyClass () : flags (0) {}
		DEFINE_FLAG (flags, isFlagged, 1<<0)
		DEFINE_FLAG (flags, isMine, 1<<1)
	private:
		uint32 flags;
	};
	void someFunction ()
	{
		MyClass c;
		if (c.isFlagged ())        // check the flag
			c.isFlagged (false);   // set the flag
	}
										\endcode
*/
//----------------------------------------------------------------------------------
///@{

/** Create Methods with @c get and @c set prefix. */
#define DEFINE_STATE(flagVar,methodName,value)\
void set##methodName (bool state) { if (state) flagVar |= (value); else flagVar &= ~(value); }\
bool get##methodName ()const { return (flagVar & (value)) != 0; }

/** Create Methods with @c get prefix.
	There is only a 'get' method. */
#define DEFINE_GETSTATE(flagVar,methodName,value)\
bool get##methodName ()const { return (flagVar & (value)) != 0; }

/** Create Methods. 
	Same name for the getter and setter. */
#define DEFINE_FLAG(flagVar,methodName,value)\
void methodName (bool state) { if (state) flagVar |= (value); else flagVar &= ~(value); }\
bool methodName ()const { return (flagVar & (value)) != 0; }

/** Create Methods.
	There is only a 'get' method. */
#define DEFINE_GETFLAG(flagVar,methodName,value)\
bool methodName ()const { return (flagVar & (value)) != 0; }


/** Create @c static Methods. 
	Same name for the getter and setter. */
#define DEFINE_FLAG_STATIC(flagVar,methodName,value)\
static void methodName (bool __state) { if (__state) flagVar |= (value); else flagVar &= ~(value); }\
static bool methodName () { return (flagVar & (value)) != 0; }
///@}

//----------------------------------------------------------------------------------
/**	@name Methods for data members.
	Macros to create setter and getter methods for class members.
	
	<b>Examples:</b>
										\code	
	class MyClass
	{
	public:
		DATA_MEMBER (double, distance, Distance)
		STRING_MEMBER (Steinberg::String, name, Name)
		SHARED_MEMBER (FUnknown, userData, UserData)
		CLASS_MEMBER (Steinberg::Buffer, bufferData, BufferData)
		POINTER_MEMBER (Steinberg::FObject, refOnly, RefOnly)  
	};
										\endcode
*/
//--------------------------------------------------------------------------------------
///@{

/** Build-in member (pass by value). */
#define DATA_MEMBER(type,varName,methodName)\
public:void set##methodName (type v) { varName = v;}\
type get##methodName ()const { return varName; }\
protected: type varName; public:

//** Object member (pass by reference). */
#define CLASS_MEMBER(type,varName,methodName)\
public:void set##methodName (const type& v){ varName = v;}\
const type& get##methodName () const { return varName; }\
protected: type varName; public:

//** Simple pointer. */
#define POINTER_MEMBER(type,varName,methodName)\
public:void set##methodName (type* ptr){ varName = ptr;}\
type* get##methodName ()const { return varName; }\
private: type* varName; public:

//** Shared member - FUnknown / FObject /  etc */
#define SHARED_MEMBER(type,varName,methodName)\
public:void set##methodName (type* v){ varName = v;}\
type* get##methodName ()const { return varName; }\
private: IPtr<type> varName; public:

//** Owned member - FUnknown / FObject / CmObject etc */
#define OWNED_MEMBER(type,varName,methodName)\
public:void set##methodName (type* v){ varName = v;}\
type* get##methodName ()const { return varName; }\
private: OPtr<type> varName; public:

//** tchar* / String class member - set by const tchar*, return by reference */
#define STRING_MEMBER(type,varName,methodName)\
public:void set##methodName (const tchar* v){ varName = v;}\
const type& get##methodName () const { return varName; }\
protected: type varName; public:

//** char8* / String class member - set by const char8*, return by reference */
#define STRING8_MEMBER(type,varName,methodName)\
public:void set##methodName (const char8* v){ varName = v;}\
const type& get##methodName () const { return varName; }\
protected: type varName; public:

//** Standard String Member Steinberg::String */
#define STRING_MEMBER_STD(varName,methodName) STRING_MEMBER(Steinberg::String,varName,methodName)
#define STRING8_MEMBER_STD(varName,methodName) STRING8_MEMBER(Steinberg::String,varName,methodName)

///@}


// obsolete names
#define DEFINE_VARIABLE(type,varName,methodName) DATA_MEMBER(type,varName,methodName)
#define DEFINE_POINTER(type,varName,methodName) POINTER_MEMBER(type,varName,methodName)
#define DEFINE_MEMBER(type,varName,methodName) CLASS_MEMBER(type,varName,methodName)





//------------------------------------------------------------------------
// for implementing comparison operators using a class member or a compare method:
//------------------------------------------------------------------------
#define COMPARE_BY_MEMBER_METHODS(className,memberName) \
	bool operator == (const className& other) const {return (memberName == other.memberName);} \
	bool operator != (const className& other) const {return (memberName != other.memberName);} \
	bool operator < (const className& other) const {return (memberName < other.memberName);} \
	bool operator > (const className& other) const {return (memberName > other.memberName);} \
	bool operator <= (const className& other) const {return (memberName <= other.memberName);} \
	bool operator >= (const className& other) const {return (memberName >= other.memberName);}

#define COMPARE_BY_MEMORY_METHODS(className) \
	bool operator == (const className& other) const {return memcmp (this, &other, sizeof (className)) == 0;} \
	bool operator != (const className& other) const {return memcmp (this, &other, sizeof (className)) != 0;} \
	bool operator < (const className& other) const {return memcmp (this, &other, sizeof (className)) < 0;} \
	bool operator > (const className& other) const {return memcmp (this, &other, sizeof (className)) > 0;} \
	bool operator <= (const className& other) const {return memcmp (this, &other, sizeof (className)) <= 0;} \
	bool operator >= (const className& other) const {return memcmp (this, &other, sizeof (className)) >= 0;}

#define COMPARE_BY_COMPARE_METHOD(className,methodName) \
	bool operator == (const className& other) const {return methodName (other) == 0;} \
	bool operator != (const className& other) const {return methodName (other) != 0;} \
	bool operator < (const className& other) const {return methodName (other) < 0;} \
	bool operator > (const className& other) const {return methodName (other) > 0;} \
	bool operator <= (const className& other) const {return methodName (other) <= 0; } \
	bool operator >= (const className& other) const {return methodName (other) >= 0; }
