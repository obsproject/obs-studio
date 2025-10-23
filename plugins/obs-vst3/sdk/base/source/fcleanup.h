//------------------------------------------------------------------------
// Project     : SDK Base
// Version     : 1.0
//
// Category    : Helpers
// Filename    : base/source/fcleanup.h
// Created by  : Steinberg, 2008
// Description : Template classes for automatic resource cleanup
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include <cstdlib>

namespace Steinberg {

/** Template definition for classes that help guarding against memory leaks.
A stack allocated object of this type automatically deletes an at construction time passed
dynamically allocated single object when it reaches the end of its scope. \n\n
Intended usage:
\code
    {
        int* pointerToInt = new int;
        Steinberg::FDeleter<int> deleter (pointerToInt);

        // Do something with the variable behind pointerToInt.

    } // No memory leak here, destructor of deleter cleans up the integer.
\endcode
*/
//------------------------------------------------------------------------
template <class T>
struct FDeleter
{
	/// Constructor. _toDelete is a pointer to the dynamically allocated object that is to be
	/// deleted when this FDeleter object's destructor is executed.
	FDeleter (T* _toDelete) : toDelete (_toDelete) {}
	/// Destructor. Calls delete on the at construction time passed pointer.
	~FDeleter ()
	{
		if (toDelete)
			delete toDelete;
	}
	T* toDelete; ///< Remembers the object that is to be deleted during destruction.
};

/** Template definition for classes that help guarding against memory leaks.
A stack allocated object of this type automatically deletes an at construction time passed
dynamically allocated array of objects when it reaches the end of its scope. \n\n
Intended usage:
\code
    {
        int* pointerToIntArray = new int[10];
        Steinberg::FArrayDeleter<int> deleter (pointerToIntArray);

        // Do something with the array behind pointerToIntArray.

    } // No memory leak here, destructor of deleter cleans up the integer array.
\endcode
*/
//------------------------------------------------------------------------
template <class T>
struct FArrayDeleter
{
	/// Constructor. _arrayToDelete is a pointer to the dynamically allocated array of objects that
	/// is to be deleted when this FArrayDeleter object's destructor is executed.
	FArrayDeleter (T* _arrayToDelete) : arrayToDelete (_arrayToDelete) {}

	/// Destructor. Calls delete[] on the at construction time passed pointer.
	~FArrayDeleter ()
	{
		if (arrayToDelete)
			delete[] arrayToDelete;
	}

	T* arrayToDelete; ///< Remembers the array of objects that is to be deleted during destruction.
};

/** Template definition for classes that help guarding against dangling pointers.
A stack allocated object of this type automatically resets an at construction time passed
pointer to null when it reaches the end of its scope. \n\n
Intended usage:
\code
    int* pointerToInt = 0;
    {
        int i = 1;
        pointerToInt = &i;
        Steinberg::FPtrNuller<int> ptrNuller (pointerToInt);

        // Do something with pointerToInt.

    } // No dangling pointer here, pointerToInt is reset to 0 by destructor of ptrNuller.
\endcode
*/
//------------------------------------------------------------------------
template <class T>
struct FPtrNuller
{
	/// Constructor. _toNull is a reference to the pointer that is to be reset to NULL when this
	/// FPtrNuller object's destructor is executed.
	FPtrNuller (T*& _toNull) : toNull (_toNull) {}
	/// Destructor. Calls delete[] on the at construction time passed pointer.
	~FPtrNuller () { toNull = 0; }

	T*& toNull; ///< Remembers the pointer that is to be set to NULL during destruction.
};

/** Template definition for classes that help resetting an object's value.
A stack allocated object of this type automatically resets the value of an at construction time
passed object to null when it reaches the end of its scope. \n\n Intended usage: \code int theObject
= 0;
    {
        Steinberg::FNuller<int> theNuller (theObject);

        theObject = 1;

    } // Here the destructor of theNuller resets the value of theObject to 0.
\endcode
*/
//------------------------------------------------------------------------
template <class T>
struct FNuller
{
	/// Constructor. _toNull is a reference to the object that is to be assigned 0 when this FNuller
	/// object's destructor is executed.
	FNuller (T& _toNull) : toNull (_toNull) {}
	/// Destructor. Assigns 0 to the at construction time passed object reference.
	~FNuller () { toNull = 0; }

	T& toNull; ///< Remembers the object that is to be assigned 0 during destruction.
};

/** Class definition for objects that help resetting boolean variables.
A stack allocated object of this type automatically sets an at construction time passed
boolean variable immediately to TRUE and resets the same variable to FALSE when it
reaches the end of its own scope. \n\n
Intended usage:
\code
    bool theBoolean = false;
    {
        Steinberg::FBoolSetter theBoolSetter (theBoolean);
        // Here the constructor of theBoolSetter sets theBoolean to TRUE.

        // Do something.

    } // Here the destructor of theBoolSetter resets theBoolean to FALSE.
\endcode
*/
//------------------------------------------------------------------------
template <class T>
struct FBooleanSetter
{
	/// Constructor. _toSet is a reference to the boolean that is set to TRUE immediately in this
	/// constructor call and gets reset to FALSE when this FBoolSetter object's destructor is
	/// executed.
	FBooleanSetter (T& _toSet) : toSet (_toSet) { toSet = true; }
	/// Destructor. Resets the at construction time passed boolean to FALSE.
	~FBooleanSetter () { toSet = false; }

	T& toSet; ///< Remembers the boolean that is to be reset during destruction.
};

using FBoolSetter = FBooleanSetter<bool>;

/** Class definition for objects that help setting boolean variables.
A stack allocated object of this type automatically sets an at construction time passed
boolean variable to TRUE if the given condition is met. At the end of its own scope the
stack object will reset the same boolean variable to FALSE, if it wasn't set so already. \n\n
Intended usage:
\code
    bool theBoolean = false;
    {
        bool creativityFirst = true;
        Steinberg::FConditionalBoolSetter theCBSetter (theBoolean, creativityFirst);
        // Here the constructor of theCBSetter sets theBoolean to the value of creativityFirst.

        // Do something.

    } // Here the destructor of theCBSetter resets theBoolean to FALSE.
\endcode
*/
//------------------------------------------------------------------------
struct FConditionalBoolSetter
{
	/// Constructor. _toSet is a reference to the boolean that is to be set. If the in the second
	/// parameter given condition is TRUE then also _toSet is set to TRUE immediately.
	FConditionalBoolSetter (bool& _toSet, bool condition) : toSet (_toSet)
	{
		if (condition)
			toSet = true;
	}
	/// Destructor. Resets the at construction time passed boolean to FALSE.
	~FConditionalBoolSetter () { toSet = false; }

	bool& toSet; ///< Remembers the boolean that is to be reset during destruction.
};

/** Template definition for classes that help closing resources.
A stack allocated object of this type automatically calls the close method of an at
construction time passed object when it reaches the end of its scope.
It goes without saying that the given type needs to have a close method. \n\n
Intended usage:
\code
    struct CloseableObject
    {
        void close() {};
    };

    {
        CloseableObject theObject;
        Steinberg::FCloser<CloseableObject> theCloser (&theObject);

        // Do something.

    } // Here the destructor of theCloser calls the close method of theObject.
\endcode
*/
template <class T>
struct FCloser
{
	/// Constructor. _obj is the pointer on which close is to be called when this FCloser object's
	/// destructor is executed.
	FCloser (T* _obj) : obj (_obj) {}
	/// Destructor. Calls the close function on the at construction time passed pointer.
	~FCloser ()
	{
		if (obj)
			obj->close ();
	}

	T* obj; ///< Remembers the pointer on which close is to be called during destruction.
};

/** Class definition for objects that help guarding against memory leaks.
A stack allocated object of this type automatically frees the "malloced" memory behind an at
construction time passed pointer when it reaches the end of its scope.
*/
//------------------------------------------------------------------------
/*! \class FMallocReleaser
 */
//------------------------------------------------------------------------
class FMallocReleaser
{
public:
	/// Constructor. _data is the pointer to the memory on which free is to be called when this
	/// FMallocReleaser object's destructor is executed.
	FMallocReleaser (void* _data) : data (_data) {}
	/// Destructor. Calls the free function on the at construction time passed pointer.
	~FMallocReleaser ()
	{
		if (data)
			free (data);
	}
//------------------------------------------------------------------------
protected:
	void* data; ///< Remembers the pointer on which free is to be called during destruction.
};

//------------------------------------------------------------------------
} // namespace Steinberg


#if SMTG_OS_MACOS
typedef const void* CFTypeRef;
extern "C" {
	extern void CFRelease (CFTypeRef cf);
}

namespace Steinberg {

/** Class definition for objects that helps releasing CoreFoundation objects.
A stack allocated object of this type automatically releases an at construction time
passed CoreFoundation object when it reaches the end of its scope.

Only available on Macintosh platform.
*/
//------------------------------------------------------------------------
/*! \class CFReleaser
*/
//------------------------------------------------------------------------
class CFReleaser
{
public:
	/// Constructor. _obj is the reference to an CoreFoundation object which is to be released when this CFReleaser object's destructor is executed. 
	CFReleaser (CFTypeRef _obj) : obj (_obj) {}
	/// Destructor. Releases the at construction time passed object.
	~CFReleaser () { if (obj) CFRelease (obj); }
protected:
	CFTypeRef obj; ///< Remembers the object which is to be released during destruction.
};

//------------------------------------------------------------------------
} // namespace Steinberg

#endif // SMTG_OS_MACOS
