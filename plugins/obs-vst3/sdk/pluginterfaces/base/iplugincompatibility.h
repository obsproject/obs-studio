//-----------------------------------------------------------------------------
// Project     : SDK Core
//
// Category    : SDK Core Interfaces
// Filename    : pluginterfaces/base/iplugincompatibility.h
// Created by  : Steinberg, 02/2022
// Description : Basic Plug-in Interfaces
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "ibstream.h"

//------------------------------------------------------------------------
namespace Steinberg {

//------------------------------------------------------------------------
/** moduleinfo.json

The moduleinfo.json describes the contents of the plug-in in a JSON5 compatible format (See https://json5.org/).
It contains the factory info (see PFactoryInfo), the contained classes (see PClassInfo), the
included snapshots and a list of compatibility of the included classes.

An example moduleinfo.json:

\code
{
  "Name": "",
  "Version": "1.0",
  "Factory Info": {
    "Vendor": "Steinberg Media Technologies",
    "URL": "http://www.steinberg.net",
    "E-Mail": "mailto:info@steinberg.de",
    "Flags": {
      "Unicode": true,
      "Classes Discardable": false,
      "Component Non Discardable": false,
    },
  },
  "Compatibility": [
    {
		"New": "B9F9ADE1CD9C4B6DA57E61E3123535FD",
		"Old": [
		  "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", // just an example
		  "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB", // another example
		],
	},
  ],
  "Classes": [
    {
      "CID": "B9F9ADE1CD9C4B6DA57E61E3123535FD",
      "Category": "Audio Module Class",
      "Name": "AGainSimple VST3",
      "Vendor": "Steinberg Media Technologies",
      "Version": "1.3.0.1",
      "SDKVersion": "VST 3.7.4",
      "Sub Categories": [
        "Fx",
      ],
      "Class Flags": 0,
      "Cardinality": 2147483647,
      "Snapshots": [
      ],
    },
  ],
}
\endcode

*/

#define kPluginCompatibilityClass "Plugin Compatibility Class"

//------------------------------------------------------------------------
/** optional interface to query the compatibility of the plug-ins classes
- [plug imp]

A plug-in can add a class with this interface to its class factory if it cannot provide a
moduleinfo.json file in its plug-in package/bundle where the compatibility is normally part of.

If the module contains a moduleinfo.json the host will ignore this class.

The class must write into the stream an UTF-8 encoded json description of the compatibility of
the other classes in the factory.

It is expected that the JSON5 written starts with an array:
\code
[
    {
		"New": "B9F9ADE1CD9C4B6DA57E61E3123535FD",
		"Old": [
		  "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", // just an example
		  "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB", // another example
		],
	},
]
\endcode
*/
class IPluginCompatibility : public FUnknown
{
public:
	/** get the compatibility stream
	 *	@param stream	the stream the plug-in must write the UTF8 encoded JSON5 compatibility
	 *					string.
	 *	@return kResultTrue on success
	 */
	virtual tresult PLUGIN_API getCompatibilityJSON (IBStream* stream) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IPluginCompatibility, 0x4AFD4B6A, 0x35D7C240, 0xA5C31414, 0xFB7D15E6)

//------------------------------------------------------------------------
} // Steinberg
