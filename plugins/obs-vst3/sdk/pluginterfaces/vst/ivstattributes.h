//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Interfaces
// Filename    : pluginterfaces/vst/ivstattributes.h
// Created by  : Steinberg, 05/2006
// Description : VST Attribute Interfaces
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
#include "pluginterfaces/vst/vsttypes.h"

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpush.h"
//------------------------------------------------------------------------

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** Attribute list used in IMessage and IStreamAttributes: Vst::IAttributeList
\ingroup vstIHost vst300
- [host imp]
- [released: 3.0.0]
- [mandatory]

An attribute list associates values with a key (id: some predefined keys can be found in \ref
presetAttributes).
*/
class IAttributeList : public FUnknown
{
public:
//------------------------------------------------------------------------
	/** \ingroup vst3typedef */
	typedef const char* AttrID;

	/** Sets integer value. */
	virtual tresult PLUGIN_API setInt (AttrID id /*in*/, int64 value /*in*/) = 0;

	/** Gets integer value. */
	virtual tresult PLUGIN_API getInt (AttrID id /*in*/, int64& value /*out*/) = 0;

	/** Sets float value. */
	virtual tresult PLUGIN_API setFloat (AttrID id /*in*/, double value /*in*/) = 0;

	/** Gets float value. */
	virtual tresult PLUGIN_API getFloat (AttrID id /*in*/, double& value /*out*/) = 0;

	/** Sets string value (UTF16) (must be null-terminated!). */
	virtual tresult PLUGIN_API setString (AttrID id /*in*/, const TChar* string /*in*/) = 0;

	/** Gets string value (UTF16). Note that Size is in Byte, not the string Length!
	    Do not forget to multiply the length by sizeof (TChar)! */
	virtual tresult PLUGIN_API getString (AttrID id /*in*/, TChar* string /*out*/,
	                                      uint32 sizeInBytes /*in*/) = 0;

	/** Sets binary data. */
	virtual tresult PLUGIN_API setBinary (AttrID id /*in*/, const void* data /*in*/,
	                                      uint32 sizeInBytes /*in*/) = 0;

	/** Gets binary data. */
	virtual tresult PLUGIN_API getBinary (AttrID id /*in*/, const void*& data /*out*/,
	                                      uint32& sizeInBytes) = 0;
//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IAttributeList, 0x1E5F0AEB, 0xCC7F4533, 0xA2544011, 0x38AD5EE4)

//------------------------------------------------------------------------
/** Meta attributes of a stream: Vst::IStreamAttributes
\ingroup vstIHost vst360
- [host imp]
- [extends IBStream]
- [released: 3.6.0]
- [optional]

Interface to access preset meta information from stream, for example used in setState\getState
in order to inform the plug-in about the current context in which the preset loading\saving occurs
(Project context or Preset load\save (see \ref StateType)) or used to get the full file path of the
loaded preset (if available).

\code{.cpp}
//------------------------------------------------------------------------
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/vstpresetkeys.h"
...

tresult PLUGIN_API MyPlugin::setState (IBStream* state)
{
	FUnknownPtr<IStreamAttributes> stream (state);
	if (stream)
	{
		if (IAttributeList* list = stream->getAttributes ())
		{
			// get the current type (project/Default..) of this state
			String128 string;
			if (list->getString (PresetAttributes::kStateType, string, 128 * sizeof (TChar)) ==
				kResultTrue)
			{
				UString128 tmp (string);
				char ascii[128];
				tmp.toAscii (ascii, 128);
				if (strncmp (ascii, StateType::kProject, strlen (StateType::kProject)) == 0)
				{
					// we are in project loading context...
				}
			}

			// get the full file path of this state
			TChar fullPath[1024];
			if (list->getString (PresetAttributes::kFilePathStringType, fullPath,
								 1024 * sizeof (TChar)) == kResultTrue)
			{
				// here we have the full path ...
			}
		}
	}

    //...read the state here.....
    return kResultTrue;
}
\endcode
*/
class IStreamAttributes : public FUnknown
{
public:
	//------------------------------------------------------------------------
	/** Gets filename (without file extension) of the stream. */
	virtual tresult PLUGIN_API getFileName (String128 name /*inout*/) = 0;

	/** Gets meta information list. */
	virtual IAttributeList* PLUGIN_API getAttributes () = 0;
	//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IStreamAttributes, 0xD6CE2FFC, 0xEFAF4B8C, 0x9E74F1BB, 0x12DA44B4)

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpop.h"
//------------------------------------------------------------------------
