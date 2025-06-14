//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Interfaces
// Filename    : pluginterfaces/vst/ivstrepresentation.h
// Created by  : Steinberg, 08/2010
// Description : VST Representation Interface
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
class IBStream;

namespace Vst {

//------------------------------------------------------------------------
/** RepresentationInfo is the structure describing a representation
This structure is used in the function \see IXmlRepresentationController::getXmlRepresentationStream.
\see IXmlRepresentationController 
*/
struct RepresentationInfo
{
	RepresentationInfo ()
	{
		memset (vendor, 0, kNameSize);
		memset (name, 0, kNameSize);
		memset (version, 0, kNameSize);
		memset (host, 0, kNameSize); 
	}
	
	RepresentationInfo (char8* _vendor, char8* _name = nullptr, char8* _version = nullptr, char8* _host = nullptr)
	{
		memset (vendor, 0, kNameSize);
		if (_vendor)
			strcpy (vendor,	_vendor);
		memset (name, 0, kNameSize);
		if (_name)
			strcpy (name, _name);
		memset (version, 0, kNameSize);
		if (_version)
			strcpy (version, _version);
		memset (host, 0, kNameSize);
		if (_host)
			strcpy (host, _host);
	}

	enum
	{
		kNameSize = 64
	};
	char8 vendor[kNameSize];	///< Vendor name of the associated representation (remote) (eg. "Yamaha").
	char8 name[kNameSize];		///< Representation (remote) Name (eg. "O2").
	char8 version[kNameSize];	///< Version of this "Remote" (eg. "1.0").
	char8 host[kNameSize];		///< Optional: used if the representation is for a given host only (eg. "Nuendo").
};


//------------------------------------------------------------------------
//------------------------------------------------------------------------
/** Extended plug-in interface IEditController for a component: Vst::IXmlRepresentationController
\ingroup vstIPlug vst350
- [plug imp]
- [extends IEditController]
- [released: 3.5.0]
- [optional]

A representation based on XML is a way to export, structure, and group plug-ins parameters for a specific remote (hardware or software rack (such as quick controls)).
\n
It allows to describe each parameter more precisely (what is the best matching to a knob, different title lengths matching limited remote display,...).\n See an \ref Example.
 \n\n
- A representation is composed of pages (this means that to see all exported parameters, the user has to navigate through the pages).
- A page is composed of cells (for example 8 cells per page).
- A cell is composed of layers (for example a cell could have a knob, a display, and a button, which means 3 layers).
- A layer is associated to a plug-in parameter using the ParameterID as identifier:
	- it could be a knob with a display for title and/or value, this display uses the same parameterId, but it could an another one.
	- switch
	- link which allows to jump directly to a subpage (another page) 
	- more... See Vst::LayerType
.

\n
This representation is implemented as XML text following the Document Type Definition (DTD): http://dtd.steinberg.net/VST-Remote-1.1.dtd

\section Example
Here an example of what should be passed in the stream of getXmlRepresentationStream:

\code
<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE vstXML PUBLIC "-//Steinberg//DTD VST Remote 1.1//EN" "http://dtd.steinberg.net/VST-Remote-1.1.dtd">
<vstXML version="1.0">
	<plugin classID="341FC5898AAA46A7A506BC0799E882AE" name="Chorus" vendor="Steinberg Media Technologies" />
	<originator>My name</originator>
	<date>2010-12-31</date>
	<comment>This is an example for 4 Cells per Page for the Remote named ProductRemote 
	         from company HardwareCompany.</comment>

	<!-- ===================================== -->
	<representation name="ProductRemote" vendor="HardwareCompany" version="1.0">
		<page name="Root">
			<cell>
				<layer type="knob" parameterID="0">
					<titleDisplay>
						<name>Mix dry/wet</name>
						<name>Mix</name>
					</titleDisplay>
				</layer>
			</cell>
			<cell>
				<layer type="display"></layer>
			</cell>
			<cell>
				<layer type="knob" parameterID="3">
					<titleDisplay>
						<name>Delay</name>
						<name>Dly</name>
					</titleDisplay>
				</layer>
			</cell>
			<cell>
				<layer type="knob" parameterID="15">
					<titleDisplay>
						<name>Spatial</name>
						<name>Spat</name>
					</titleDisplay>
				</layer>
			</cell>
		</page>
		<page name="Page 2">
			<cell>
				<layer type="LED" ledStyle="spread" parameterID="2">
					<titleDisplay>
						<name>Width +</name>
						<name>Widt</name>
					</titleDisplay>
				</layer>
				<!--this is the switch for shape A/B-->
				<layer type="switch" switchStyle="pushIncLooped" parameterID="4"></layer>
			</cell>
			<cell>
				<layer type="display"></layer>
			</cell>
			<cell>
				<layer type="LED" ledStyle="singleDot" parameterID="17">
					<titleDisplay>
						<name>Sync Note +</name>
						<name>Note</name>
					</titleDisplay>
				</layer>
				<!--this is the switch for sync to tempo on /off-->
				<layer type="switch" switchStyle="pushIncLooped" parameterID="16"></layer>
			</cell>
			<cell>
				<layer type="knob" parameterID="1">
					<titleDisplay>
						<name>Rate</name>
					</titleDisplay>
				</layer>
			</cell>
		</page>
	</representation>
</vstXML>
\endcode
*/
class IXmlRepresentationController : public FUnknown
{
public:
	/** Retrieves a stream containing a XmlRepresentation for a wanted representation info.
	 * \note [UI-thread & Initialized] */
	virtual tresult PLUGIN_API getXmlRepresentationStream (RepresentationInfo& info /*in*/,
	                                                       IBStream* stream /*inout*/) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IXmlRepresentationController, 0xA81A0471, 0x48C34DC4, 0xAC30C9E1, 0x3C8393D5)

//------------------------------------------------------------------------
/** Defines for XML representation Tags and Attributes */

#define ROOTXML_TAG			"vstXML"

#define COMMENT_TAG			"comment"
#define CELL_TAG			"cell"
#define CELLGROUP_TAG		"cellGroup"
#define CELLGROUPTEMPLATE_TAG	"cellGroupTemplate"
#define CURVE_TAG			"curve"
#define CURVETEMPLATE_TAG	"curveTemplate"
#define DATE_TAG			"date"
#define LAYER_TAG			"layer"
#define NAME_TAG			"name"
#define ORIGINATOR_TAG		"originator"
#define PAGE_TAG			"page"
#define PAGETEMPLATE_TAG	"pageTemplate"
#define PLUGIN_TAG			"plugin"
#define VALUE_TAG			"value"
#define VALUEDISPLAY_TAG	"valueDisplay"
#define VALUELIST_TAG		"valueList"
#define REPRESENTATION_TAG	"representation"
#define SEGMENT_TAG			"segment"
#define SEGMENTLIST_TAG		"segmentList"
#define TITLEDISPLAY_TAG	"titleDisplay"

#define ATTR_CATEGORY		"category"
#define ATTR_CLASSID		"classID"
#define ATTR_ENDPOINT		"endPoint"
#define ATTR_INDEX			"index"
#define ATTR_FLAGS			"flags"
#define ATTR_FUNCTION		"function"
#define ATTR_HOST			"host"
#define ATTR_LEDSTYLE		"ledStyle"
#define ATTR_LENGTH			"length"
#define ATTR_LINKEDTO		"linkedTo"
#define ATTR_NAME			"name"
#define ATTR_ORDER			"order"
#define ATTR_PAGE			"page"
#define ATTR_PARAMID		"parameterID"
#define ATTR_STARTPOINT		"startPoint"
#define ATTR_STYLE			"style"
#define ATTR_SWITCHSTYLE	"switchStyle"
#define ATTR_TEMPLATE		"template"
#define ATTR_TURNSPERFULLRANGE	"turnsPerFullRange"
#define ATTR_TYPE			"type"
#define ATTR_UNITID			"unitID"
#define ATTR_VARIABLES		"variables"
#define ATTR_VENDOR			"vendor"
#define ATTR_VERSION		"version"

//------------------------------------------------------------------------
/** Defines some predefined Representation Remote Names */
#define GENERIC 				"Generic"
#define GENERIC_4_CELLS			"Generic 4 Cells"
#define GENERIC_8_CELLS			"Generic 8 Cells"
#define GENERIC_12_CELLS		"Generic 12 Cells"
#define GENERIC_24_CELLS		"Generic 24 Cells"
#define GENERIC_N_CELLS			"Generic %d Cells"
#define QUICK_CONTROL_8_CELLS	"Quick Controls 8 Cells"

//------------------------------------------------------------------------
/** Layer Types used in a VST XML Representation */ 
namespace LayerType
{
	enum 
	{
		kKnob = 0, 		///< a knob (encoder or not)
		kPressedKnob,  	///< a knob which is used by pressing and turning
		kSwitchKnob,	///< knob could be pressed to simulate a switch
		kSwitch,		///< a "on/off" button
		kLED,			///< LED like VU-meter or display around a knob
		kLink,			///< indicates that this layer is a folder linked to an another INode (page)
		kDisplay,		///< only for text display (not really a control)
		kFader,			///< a fader
		kEndOfLayerType
	};

	/** FIDString variant of the LayerType */
	static const FIDString layerTypeFIDString[] = {
		"knob"
		,"pressedKnob"
		,"switchKnob"
		,"switch"
		,"LED"
		,"link"
		,"display"
		,"fader"
		,nullptr
	};
};

//------------------------------------------------------------------------
/** Curve Types used in a VST XML Representation */ 
namespace CurveType
{
	const CString kSegment		= "segment";	///<
	const CString kValueList	= "valueList";	///<
};

//------------------------------------------------------------------------
/** Attributes used to defined a Layer in a VST XML Representation */ 
namespace Attributes
{
	const CString kStyle		= ATTR_STYLE;			///< string attribute : See AttributesStyle for available string value
	const CString kLEDStyle		= ATTR_LEDSTYLE;		///< string attribute : See AttributesStyle for available string value
	const CString kSwitchStyle	= ATTR_SWITCHSTYLE;		///< string attribute : See AttributesStyle for available string value
	const CString kKnobTurnsPerFullRange = ATTR_TURNSPERFULLRANGE;	///< float attribute
	const CString kFunction		= ATTR_FUNCTION;		///< string attribute : See AttributesFunction for available string value
	const CString kFlags		= ATTR_FLAGS;			///< string attribute : See AttributesFlags for available string value
};

//------------------------------------------------------------------------
/** Attributes Function used to defined the function of a Layer in a VST XML Representation */ 
namespace AttributesFunction
{
	/// Global Style
	const CString kPanPosCenterXFunc		= "PanPosCenterX";		///< Gravity point X-axis (L-R) (for stereo: middle between left and right)
	const CString kPanPosCenterYFunc		= "PanPosCenterY";		///< Gravity point Y-axis (Front-Rear)
	const CString kPanPosFrontLeftXFunc		= "PanPosFrontLeftX";	///< Left channel Position in X-axis
	const CString kPanPosFrontLeftYFunc		= "PanPosFrontLeftY";	///< Left channel Position in Y-axis
	const CString kPanPosFrontRightXFunc	= "PanPosFrontRightX";	///< Right channel Position in X-axis
	const CString kPanPosFrontRightYFunc	= "PanPosFrontRightY";	///< Right channel Position in Y-axis
	const CString kPanRotationFunc			= "PanRotation";		///< Rotation around the Center (gravity point)
	const CString kPanLawFunc				= "PanLaw";				///< Panning Law
	const CString kPanMirrorModeFunc		= "PanMirrorMode";		///< Panning Mirror Mode
	const CString kPanLfeGainFunc			= "PanLfeGain";			///< Panning LFE Gain
	const CString kGainReductionFunc		= "GainReduction";		///< Gain Reduction for compressor
	const CString kSoloFunc					= "Solo";				///< Solo
	const CString kMuteFunc					= "Mute";				///< Mute
	const CString kVolumeFunc				= "Volume";				///< Volume
};

//------------------------------------------------------------------------
/** Attributes Style associated a specific Layer Type in a VST XML Representation */ 
namespace AttributesStyle
{
	/// Global Style
	const CString kInverseStyle			= "inverse";	///< the associated layer should use the inverse value of parameter (1 - x).

	/// LED Style
	const CString kLEDWrapLeftStyle		= "wrapLeft";	///< |======>----- (the default one if not specified)
	const CString kLEDWrapRightStyle	= "wrapRight";	///< -------<====|
	const CString kLEDSpreadStyle		= "spread";		///< ---<==|==>---
	const CString kLEDBoostCutStyle		= "boostCut";	///< ------|===>--
	const CString kLEDSingleDotStyle	= "singleDot";	///< --------|----

	/// Switch Style
	const CString kSwitchPushStyle		= "push";		///< Apply only when pressed, unpressed will reset the value to min.
	const CString kSwitchPushIncLoopedStyle	= "pushIncLooped";	///< Push will increment the value. When the max is reached it will restart with min.
																///< The default one if not specified (with 2 states values it is a OnOff switch).
	const CString kSwitchPushDecLoopedStyle	= "pushDecLooped";	///< Push will decrement the value. When the min is reached it will restart with max.
	const CString kSwitchPushIncStyle	= "pushInc";	///< Increment after each press (delta depends of the curve).
	const CString kSwitchPushDecStyle	= "pushDec";	///< Decrement after each press (delta depends of the curve).
	const CString kSwitchLatchStyle		= "latch";		///< Each push-release will change the value between min and max. 
														///< A timeout between push and release could be used to simulate a push style (if timeout is reached).
};

//------------------------------------------------------------------------
/** Attributes Flags defining a Layer in a VST XML Representation */ 
namespace AttributesFlags
{
	const CString kHideableFlag			= "hideable";	///< the associated layer marked as hideable allows a remote to hide or make it not usable a parameter when the associated value is inactive 
};

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpop.h"
//------------------------------------------------------------------------
