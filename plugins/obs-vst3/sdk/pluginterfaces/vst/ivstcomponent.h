//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Interfaces
// Filename    : pluginterfaces/vst/ivstcomponent.h
// Created by  : Steinberg, 04/2005
// Description : Basic VST Interfaces
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/vst/vsttypes.h"

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpush.h"
//------------------------------------------------------------------------

//------------------------------------------------------------------------
namespace Steinberg {
class IBStream;

//------------------------------------------------------------------------
/** All VST specific interfaces are located in Vst namespace */
namespace Vst {

/** Standard value for PFactoryInfo::flags */
const int32 kDefaultFactoryFlags = PFactoryInfo::kUnicode; 

#define BEGIN_FACTORY_DEF(vendor,url,email) using namespace Steinberg; \
	SMTG_EXPORT_SYMBOL IPluginFactory* PLUGIN_API GetPluginFactory () {	\
	if (!gPluginFactory) \
	{ \
		static PFactoryInfo factoryInfo (vendor, url, email, Vst::kDefaultFactoryFlags); \
		gPluginFactory = new CPluginFactory (factoryInfo);

//------------------------------------------------------------------------
/** \defgroup vstBus VST busses
Bus Description

A bus can be understood as a "collection of data channels" belonging together.
It describes a data input or a data output of the plug-in.
A VST component can define any desired number of busses.
Dynamic usage of busses is handled in the host by activating and deactivating busses.
All busses are initially inactive.
The component has to define the maximum number of supported busses and it has to
define which of them have to be activated by default after instantiation of the plug-in (This is
only a wish, the host is allow to not follow it, and only activate the first bus for example).
A host that can handle multiple busses, allows the user to activate busses which are initially all inactive.
The kMain busses have to place before any others kAux busses.

See also: IComponent::getBusInfo, IComponent::activateBus
*/
/**@{*/

//------------------------------------------------------------------------
/** Bus media types */
enum MediaTypes
{
	kAudio = 0,		///< audio
	kEvent,			///< events
	kNumMediaTypes
};

//------------------------------------------------------------------------
/** Bus directions */
enum BusDirections
{
	kInput = 0,		///< input bus
	kOutput			///< output bus
};

//------------------------------------------------------------------------
/** Bus types */
enum BusTypes
{
	kMain = 0,		///< main bus
	kAux			///< auxiliary bus (sidechain)
};

//------------------------------------------------------------------------
/** BusInfo:
This is the structure used with getBusInfo, informing the host about what is a specific given bus.
\n See also: Steinberg::Vst::IComponent::getBusInfo
*/
struct BusInfo
{
	MediaType mediaType;	///< Media type - has to be a value of \ref MediaTypes
	BusDirection direction; ///< input or output \ref BusDirections
	int32 channelCount;		///< number of channels (if used then need to be recheck after \ref
							/// IAudioProcessor::setBusArrangements is called).
							/// For a bus of type MediaTypes::kEvent the channelCount corresponds
							/// to the number of supported MIDI channels by this bus
	String128 name;			///< name of the bus
	BusType busType;		///< main or aux - has to be a value of \ref BusTypes
	uint32 flags;			///< flags - a combination of \ref BusFlags
	enum BusFlags
	{
		/** The bus should be activated by the host per default on instantiation (activateBus call is requested).
 		 	By default a bus is inactive. */
		kDefaultActive = 1 << 0,
		/** The bus does not contain ordinary audio data, but data used for control changes at sample rate.
 			The data is in the same format as the audio data [-1..1].
 			A host has to prevent unintended routing to speakers to prevent damage.
		 	Only valid for audio media type busses.
		 	[released: 3.7.0] */
		kIsControlVoltage = 1 << 1
	};
};

/**@}*/

//------------------------------------------------------------------------
/** I/O modes */
enum IoModes
{
	kSimple = 0,		///< 1:1 Input / Output. Only used for Instruments. See \ref vst3IoMode
	kAdvanced,			///< n:m Input / Output. Only used for Instruments.
	kOfflineProcessing	///< plug-in used in an offline processing context
};

//------------------------------------------------------------------------
/** Routing Information:
When the plug-in supports multiple I/O busses, a host may want to know how the busses are related. The
relation of an event-input-channel to an audio-output-bus in particular is of interest to the host
(in order to relate MIDI-tracks to audio-channels)
\n See also: IComponent::getRoutingInfo, \ref vst3Routing
*/
struct RoutingInfo
{
	MediaType mediaType;	///< media type see \ref MediaTypes
	int32 busIndex;			///< bus index
	int32 channel;			///< channel (-1 for all channels)
};

//------------------------------------------------------------------------
// IComponent Interface
//------------------------------------------------------------------------
/** Component base interface: Vst::IComponent
\ingroup vstIPlug vst300
- [plug imp]
- [released: 3.0.0]
- [mandatory]

This is the basic interface for a VST component and must always be supported.
It contains the common parts of any kind of processing class. The parts that
are specific to a media type are defined in a separate interface. An implementation
component must provide both the specific interface and IComponent.
\see IPluginBase
*/
class IComponent : public IPluginBase
{
public:
//------------------------------------------------------------------------
	/** Called before initializing the component to get information about the controller class.
	 * \note [UI-thread & Created] */
	virtual tresult PLUGIN_API getControllerClassId (TUID classId /*out*/) = 0;

	/** Called before 'initialize' to set the component usage (optional). See \ref IoModes.
	 * \note [UI-thread & Created] */
	virtual tresult PLUGIN_API setIoMode (IoMode mode /*in*/) = 0;

	/** Called after the plug-in is initialized. See \ref MediaTypes, BusDirections.
	 * \note [UI-thread & Initialized] */
	virtual int32 PLUGIN_API getBusCount (MediaType type /*in*/, BusDirection dir /*in*/) = 0;

	/** Called after the plug-in is initialized. See \ref MediaTypes, BusDirections.
	 * \note [UI-thread & Initialized] */
	virtual tresult PLUGIN_API getBusInfo (MediaType type /*in*/, BusDirection dir /*in*/,
	                                       int32 index /*in*/, BusInfo& bus /*out*/) = 0;

	/** Retrieves routing information (to be implemented when more than one regular input or output
	 * bus exists). The inInfo always refers to an input bus while the returned outInfo must refer
	 * to an output bus!
	 * \note [UI-thread & Initialized] */
	virtual tresult PLUGIN_API getRoutingInfo (RoutingInfo& inInfo /*in*/,
	                                           RoutingInfo& outInfo /*out*/) = 0;

	/** Called upon (de-)activating a bus in the host application. The plug-in should only processed
	 * an activated bus, the host could provide less see \ref AudioBusBuffers in the process call
	 * (see \ref IAudioProcessor::process) if last busses are not activated. An already activated
	 * bus does not need to be reactivated after a IAudioProcessor::setBusArrangements call.
	 * \note [UI-thread & Setup Done] */
	virtual tresult PLUGIN_API activateBus (MediaType type /*in*/, BusDirection dir /*in*/,
	                                        int32 index /*in*/, TBool state /*in*/) = 0;

	/** Activates / deactivates the component.
	 * \note [UI-thread & Setup Done] */
	virtual tresult PLUGIN_API setActive (TBool state /*in*/) = 0;

	/** Sets complete state of component.
	 * \note [UI-thread & (Initialized | Connected | Setup Done | Activated | Processing)] */
	virtual tresult PLUGIN_API setState (IBStream* state /*in*/) = 0;

	/** Retrieves complete state of component.
	 * \note [UI-thread & (Initialized | Connected | Setup Done | Activated | Processing)] */
	virtual tresult PLUGIN_API getState (IBStream* state /*inout*/) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IComponent, 0xE831FF31, 0xF2D54301, 0x928EBBEE, 0x25697802)

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpop.h"
//------------------------------------------------------------------------
