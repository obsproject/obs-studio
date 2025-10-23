//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Interfaces
// Filename    : pluginterfaces/vst/ivstchannelcontextinfo.h
// Created by  : Steinberg, 02/2014
// Description : VST Channel Context Info Interface
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/vst/vsttypes.h"
#include "pluginterfaces/vst/ivstattributes.h"

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpush.h"
//------------------------------------------------------------------------

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

/** For Channel Context Info Interface */
namespace ChannelContext {

//------------------------------------------------------------------------
//------------------------------------------------------------------------
//------------------------------------------------------------------------

//------------------------------------------------------------------------
//------------------------------------------------------------------------
/** Channel context interface: Vst::IInfoListener
\ingroup vstIHost vst365
- [plug imp]
- [extends IEditController]
- [released: 3.6.5]
- [optional]

Allows the host to inform the plug-in about the context in which the plug-in is instantiated,
mainly channel based info (color, name, index,...). Index can be defined inside a namespace 
(for example, index start from 1 to N for Type Input/Output Channel (Index namespace) and index 
start from 1 to M for Type Audio Channel).\n
As soon as the plug-in provides this IInfoListener interface, the host will call setChannelContextInfos 
for each change occurring to this channel (new name, new color, new indexation,...)

\section IChannelContextExample Example

\code{.cpp}
//------------------------------------------------------------------------
tresult PLUGIN_API MyPlugin::setChannelContextInfos (IAttributeList* list)
{
	if (list)
	{
		// optional we can ask for the Channel Name Length
		int64 length;
		if (list->getInt (ChannelContext::kChannelNameLengthKey, length) == kResultTrue)
		{
			...
		}
		
		// get the Channel Name where we, as plug-in, are instantiated
		String128 name;
		if (list->getString (ChannelContext::kChannelNameKey, name, sizeof (name)) == kResultTrue)
		{
			...
		}

		// get the Channel UID
		if (list->getString (ChannelContext::kChannelUIDKey, name, sizeof (name)) == kResultTrue)
		{
			...
		}

		// get Channel RuntimeID
		int64 runtimeId;
		if (list->getInt (ChannelContext::kChannelRuntimeIDKey, runtimeId) == kResultTrue)
		{
			...
		}

		// get Channel Index
		int64 index;
		if (list->getInt (ChannelContext::kChannelIndexKey, index) == kResultTrue)
		{
			...
		}
		
		// get the Channel Color
		int64 color;
		if (list->getInt (ChannelContext::kChannelColorKey, color) == kResultTrue)
		{
			uint32 channelColor = (uint32)color;
			String str;
			str.printf ("%x%x%x%x", ChannelContext::GetAlpha (channelColor),
			ChannelContext::GetRed (channelColor),
			ChannelContext::GetGreen (channelColor),
			ChannelContext::GetBlue (channelColor));
			String128 string128;
			Steinberg::UString (string128, 128).fromAscii (str);
			...
		}

		// get Channel Index Namespace Order of the current used index namespace
		if (list->getInt (ChannelContext::kChannelIndexNamespaceOrderKey, index) == kResultTrue)
		{
			...
		}
	
		// get the channel Index Namespace Length
		if (list->getInt (ChannelContext::kChannelIndexNamespaceLengthKey, length) == kResultTrue)
		{
			...
		}
		
		// get the channel Index Namespace
		String128 namespaceName;
		if (list->getString (ChannelContext::kChannelIndexNamespaceKey, namespaceName, sizeof (namespaceName)) == kResultTrue)
		{
			...
		}

		// get plug-in Channel Location
		int64 location;
		if (list->getInt (ChannelContext::kChannelPluginLocationKey, location) == kResultTrue)
		{
			String128 string128;
			switch (location)
			{
				case ChannelContext::kPreVolumeFader:
					Steinberg::UString (string128, 128).fromAscii ("PreVolFader");
				break;
				case ChannelContext::kPostVolumeFader:
					Steinberg::UString (string128, 128).fromAscii ("PostVolFader");
				break;
				case ChannelContext::kUsedAsPanner:
					Steinberg::UString (string128, 128).fromAscii ("UsedAsPanner");
				break;
				default: Steinberg::UString (string128, 128).fromAscii ("unknown!");
				break;
			}
		}
		
		// do not forget to call addRef () if you want to keep this list
	}
}
\endcode 
*/
class IInfoListener : public FUnknown
{
public:
	/** Receive the channel context infos from host.
	* \note [UI-thread & (Initialized | Connected | Setup Done | Activated | Processing)] */
	virtual tresult PLUGIN_API setChannelContextInfos (IAttributeList* list /*in*/) = 0;

	static const FUID iid;
};

DECLARE_CLASS_IID (IInfoListener, 0x0F194781, 0x8D984ADA, 0xBBA0C1EF, 0xC011D8D0)


//------------------------------------------------------------------------
/** Values used for kChannelPluginLocationKey */
enum ChannelPluginLocation
{
	kPreVolumeFader = 0,
	kPostVolumeFader,
	kUsedAsPanner
};

//------------------------------------------------------------------------
//------------------------------------------------------------------------
// Colors
//------------------------------------------------------------------------
/** \ingroup vst3typedef */
/**@{*/
//------------------------------------------------------------------------
/** ARGB (Alpha-Red-Green-Blue) */
typedef uint32 ColorSpec;
typedef uint8 ColorComponent;
/**@}*/

/** Returns the Blue part of the given ColorSpec */
inline ColorComponent GetBlue (ColorSpec cs)
{
	return (ColorComponent) (cs & 0x000000FF);
}
/** Returns the Green part of the given ColorSpec */
inline ColorComponent GetGreen (ColorSpec cs)
{
	return (ColorComponent) ((cs >> 8) & 0x000000FF);
}
/** Returns the Red part of the given ColorSpec */
inline ColorComponent GetRed (ColorSpec cs)
{
	return (ColorComponent) ((cs >> 16) & 0x000000FF);
}
/** Returns the Alpha part of the given ColorSpec */
inline ColorComponent GetAlpha (ColorSpec cs)
{
	return (ColorComponent) ((cs >> 24) & 0x000000FF);
}

//------------------------------------------------------------------------
/** Keys used as AttrID (Attribute ID) in the return IAttributeList of
 * IInfoListener::setChannelContextInfos  */
//------------------------------------------------------------------------
/** string (TChar) [optional]: unique id string used to identify a channel */
const CString kChannelUIDKey = "channel uid";

/** integer (int64) [optional]: number of characters in kChannelUIDKey */
const CString kChannelUIDLengthKey = "channel uid length";

/** integer (int64) [optional]: runtime id to identify a channel (may change when reloading project)
 */
const CString kChannelRuntimeIDKey = "channel runtime id";

/** string (TChar) [optional]: name of the channel like displayed in the mixer */
const CString kChannelNameKey = "channel name";

/** integer (int64) [optional]: number of characters in kChannelNameKey */
const CString kChannelNameLengthKey = "channel name length";

/** color (ColorSpec) [optional]: used color for the channel in mixer or track */
const CString kChannelColorKey = "channel color";

/** integer (int64) [optional]: index of the channel in a channel index namespace,
 * start with 1 not 0! */
const CString kChannelIndexKey = "channel index";

/** integer (int64) [optional]: define the order of the current used index namespace,
 * start with 1 not 0!
 * For example:
 * index namespace is "Input"   -> order 1,
 * index namespace is "Channel" -> order 2,
 * index namespace is "Output"  -> order 3
 */
const CString kChannelIndexNamespaceOrderKey = "channel index namespace order";

/** string (TChar) [optional]: name of the channel index namespace
 * for example "Input", "Output","Channel", ... 
 */
const CString kChannelIndexNamespaceKey = "channel index namespace";

/** integer (int64) [optional]: number of characters in kChannelIndexNamespaceKey */
const CString kChannelIndexNamespaceLengthKey = "channel index namespace length";

/** PNG image representation as binary [optional] */
const CString kChannelImageKey = "channel image";

/** integer (int64) [optional]: routing position of the plug-in in the channel
 * (see ChannelPluginLocation) 
 */
const CString kChannelPluginLocationKey = "channel plugin location";

//------------------------------------------------------------------------
} // namespace ChannelContext
} // namespace Vst
} // namespace Steinberg

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpop.h"
//------------------------------------------------------------------------
