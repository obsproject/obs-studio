//-----------------------------------------------------------------------------
// Project     : SDK Core
//
// Category    : SDK Core Interfaces
// Filename    : pluginterfaces/base/ipersistent.h
// Created by  : Steinberg, 09/2004
// Description : Plug-In Storage Interfaces
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/funknown.h"

namespace Steinberg {

class FVariant;
class IAttributes;
//------------------------------------------------------------------------
/**  Persistent Object Interface. 
[plug imp] \n
This interface is used to store/restore attributes of an object.
An IPlugController can implement this interface to handle presets.
The gui-xml for a preset control looks like this:
\code
....
<view name="PresetView" data="Preset"/>
....
<template name="PresetView">
	<view name="preset control" size="0, 0, 100, 20"/>
	<switch name="store preset" size="125,0,80,20" style="push|immediate" title="Store"  />
	<switch name="remove preset" size="220,0,80,20" style="push|immediate" title="Delete"  />
</template>
\endcode
The tag data="Preset" tells the host to create a preset controller that handles the 
3 values named "preset control",  "store preset", and "remove preset".
*/
class IPersistent: public FUnknown
{
public:
//------------------------------------------------------------------------
	/** The class ID must be a 16 bytes unique id that is used to create the object. 
	This ID is also used to identify the preset list when used with presets. */
	virtual tresult PLUGIN_API getClassID (char8* uid) = 0;

	/** Store all members/data in the passed IAttributes. */
	virtual tresult PLUGIN_API saveAttributes (IAttributes* ) = 0;

	/** Restore all members/data from the passed IAttributes. */
	virtual tresult PLUGIN_API loadAttributes (IAttributes* ) = 0;
//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IPersistent, 0xBA1A4637, 0x3C9F46D0, 0xA65DBA0E, 0xB85DA829)


typedef FIDString IAttrID;
//------------------------------------------------------------------------
/**  Object Data Archive Interface. 
- [host imp]

- store data/objects/binary/subattributes in the archive
- read stored data from the archive

All data stored to the archive are identified by a string (IAttrID), which must be unique on each
IAttribute level.

The basic set/get methods make use of the FVariant class defined in 'funknown.h'.
For a more convenient usage of this interface, you should use the functions defined
in namespace PAttributes (public.sdk/source/common/pattributes.h+cpp) !! 

\ingroup frameworkHostClasses
*/
class IAttributes: public FUnknown
{
public:
//------------------------------------------------------------------------
	/*! \name Methods to write attributes
	********************************************************************************************************
   */
	//@{
	/** Store any data in the archive. It is even possible to store sub-attributes by creating
	 *  a new IAttributes instance via the IHostClasses interface and pass it to the parent in the
	 *  FVariant. In this case the archive must take the ownership of the newly created object,
	 * which is true for all objects that have been created only for storing. You tell the archive
	 * to take ownership by adding the FVariant::kOwner flag to the FVariant::type member (data.type
	 * |= FVariant::kOwner). When using the PAttributes functions, this is done through a function
	 * parameter.*/
	virtual tresult PLUGIN_API set (IAttrID attrID /*in*/, const FVariant& data /*in*/) = 0;

	/** Store a list of data in the archive. Please note that the type of data is not mixable! So
	 *  you can only store a list of integers or a list of doubles/strings/etc. You can also store a
	 * list of subattributes or other objects that implement the IPersistent interface.*/
	virtual tresult PLUGIN_API queue (IAttrID listID /*in*/, const FVariant& data /*in*/) = 0;

	/** Store binary data in the archive. Parameter 'copyBytes' specifies if the passed data should
	 * be copied. The archive cannot take the ownership of binary data. Either it just references a
	 * buffer in order to write it to a file (copyBytes = false) or it copies the data to its own
	 * buffers (copyBytes = true). When binary data should be stored in the default pool for
	 * example, you must always copy it!*/
	virtual tresult PLUGIN_API setBinaryData (IAttrID attrID /*in*/, void* data /*in*/,
	                                          uint32 bytes /*in*/, bool copyBytes /*in*/) = 0;
	//@}

	/*! \name Methods to read attributes
	********************************************************************************************************
   */
	//@{
	/** Get data previously stored to the archive. */
	virtual tresult PLUGIN_API get (IAttrID attrID /*in*/, FVariant& data /*out*/) = 0;

	/** Get list of data previously stored to the archive. As long as there are queue members the
	 * method will return kResultTrue. When the queue is empty, the methods returns kResultFalse.
	 * All lists except from object lists can be reset which means that the items can be read once
	 * again. \see IAttributes::resetQueue */
	virtual tresult PLUGIN_API unqueue (IAttrID listID /*in*/, FVariant& data /*out*/) = 0;

	/** Get the amount of items in a queue. */
	virtual int32 PLUGIN_API getQueueItemCount (IAttrID attrId /*in*/) = 0;

	/** Reset a queue. If you need to restart reading a queue, you have to reset it. You can reset a
	 * queue at any time.*/
	virtual tresult PLUGIN_API resetQueue (IAttrID attrID /*in*/) = 0;

	/** Reset all queues in the archive.*/
	virtual tresult PLUGIN_API resetAllQueues () = 0;

	/** Read binary data from the archive. The data is copied into the passed buffer. The size of
	 * that buffer must fit the size of data stored in the archive which can be queried via
	 * IAttributes::getBinaryDataSize  */
	virtual tresult PLUGIN_API getBinaryData (IAttrID attrID /*in*/, void* data /*inout*/,
	                                          uint32 bytes /*in*/) = 0;
	/** Get the size in bytes of binary data in the archive. */
	virtual uint32 PLUGIN_API getBinaryDataSize (IAttrID attrID /*in*/) = 0;
	//@}

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IAttributes, 0xFA1E32F9, 0xCA6D46F5, 0xA982F956, 0xB1191B58)

//------------------------------------------------------------------------
/**  Extended access to Attributes; supports Attribute retrieval via iteration. 
- [host imp]
- [released] C7/N6

\ingroup frameworkHostClasses
*/
class IAttributes2 : public IAttributes
{
public:
	/** Returns the number of existing attributes. */
	virtual int32 PLUGIN_API countAttributes () const = 0;

	/** Returns the attribute's ID for the given index. */
	virtual IAttrID PLUGIN_API getAttributeID (int32 index /*in*/) const = 0;
//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IAttributes2, 0x1382126A, 0xFECA4871, 0x97D52A45, 0xB042AE99)

//------------------------------------------------------------------------
} // namespace Steinberg
