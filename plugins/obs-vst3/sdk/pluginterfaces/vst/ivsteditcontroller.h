//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Interfaces
// Filename    : pluginterfaces/vst/ivsteditcontroller.h
// Created by  : Steinberg, 09/2005
// Description : VST Edit Controller Interfaces
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
/** Class Category Name for Controller Component */
//------------------------------------------------------------------------
#ifndef kVstComponentControllerClass
#define kVstComponentControllerClass "Component Controller Class"
#endif

//------------------------------------------------------------------------
namespace Steinberg {
class IPlugView;
class IBStream;

//------------------------------------------------------------------------
namespace Vst {

//------------------------------------------------------------------------
/** Controller Parameter Info.
 *	A parameter info describes a parameter of the controller.
 *	The id must always be the same for a parameter as this uniquely identifies the parameter.
 */
struct ParameterInfo
{
//------------------------------------------------------------------------
	ParamID id;			///< unique identifier of this parameter (named tag too)
	String128 title;	///< parameter title (e.g. "Volume")
	String128 shortTitle; ///< parameter shortTitle (e.g. "Vol")
	String128 units;	///< parameter unit (e.g. "dB")
	int32 stepCount;	///< number of discrete steps (0: continuous, 1: toggle, discrete value
						/// otherwise (corresponding to max - min, for example:
						/// 127 for min = 0 and max = 127) - see \ref vst3ParameterIntro)
	ParamValue defaultNormalizedValue; ///< default normalized value [0,1] 
	                                   /// in case of discrete value:
									   /// defaultNormalizedValue = defDiscreteValue/stepCount
	UnitID unitId;		///< id of unit this parameter belongs to (see \ref vst3Units)

	int32 flags;		///< ParameterFlags (see below)

	/** Parameter flags
	 *
	 * Note that all non defined bits are reserved for future use!
	 */
	enum ParameterFlags : int32
	{
		/** No flags wanted.
		 * [SDK 3.0.0] */
		kNoFlags = 0,

		/** Parameter can be automated.
		 * [SDK 3.0.0] */
		kCanAutomate = 1 << 0,

		/** Parameter cannot be changed from outside the plug-in
		 * (implies that kCanAutomate is NOT set).
		 * [SDK 3.0.0] */
		kIsReadOnly = 1 << 1,

		/** Attempts to set the parameter value out of the limits will result in a wrap around.
		 * [SDK 3.0.2] */
		kIsWrapAround = 1 << 2,

		/** Parameter should be displayed as list in generic editor or automation editing.
		 * [SDK 3.1.0] */
		kIsList = 1 << 3,
		/** Parameter should be NOT displayed and cannot be changed from outside the plug-in.
		 * It implies that kCanAutomate is NOT set and kIsReadOnly is set.
		 * [SDK 3.7.0] */
		kIsHidden = 1 << 4,

		/** Parameter is a program change (unitId gives info about associated unit - see \ref
		 * vst3ProgramLists).
		 * [SDK 3.0.0] */
		kIsProgramChange = 1 << 15,

		/** Special bypass parameter (only one allowed): plug-in can handle bypass.
		 * Highly recommended to export a bypass parameter for effect plug-in.
		 * [SDK 3.0.0] */
		kIsBypass = 1 << 16
	};
//------------------------------------------------------------------------
};

//------------------------------------------------------------------------
/** View Types used for IEditController::createView */
//------------------------------------------------------------------------
namespace ViewType {
const CString kEditor = "editor";
}

//------------------------------------------------------------------------
/** Flags used for IComponentHandler::restartComponent */
enum RestartFlags : int32
{
	/** The Component should be reloaded
	 * The host has to unload completely the plug-in (controller/processor) and reload it.
	 * [SDK 3.0.0] */
	kReloadComponent			= 1 << 0,

	/** Input/Output Bus configuration has changed
	 * The plug-in informs the host that either the bus configuration or the bus count has changed.
	 * The host has to deactivate the plug-in, asks the plug-in for its wanted new bus
	 * configurations, adapts its processing graph and reactivate the plug-in.
	 * [SDK 3.0.0] */
	kIoChanged = 1 << 1,

	/** Multiple parameter values have changed  (as result of a program change for example)
	 * The host invalidates all caches of parameter values and asks the edit controller for the
	 * current values.
	 * [SDK 3.0.0] */
	kParamValuesChanged = 1 << 2,

	/** Latency has changed
	 * The plug informs the host that its latency has changed, getLatencySamples should return the new latency after setActive (true) was called
	 * The host has to deactivate and reactivate the plug-in, then afterwards the host could ask for the current latency (getLatencySamples)
	 * See IAudioProcessor::getLatencySamples
	 * [SDK 3.0.0] */
	kLatencyChanged				= 1 << 3,

	/** Parameter titles (title, shortTitle and units), default values, stepCount or flags (ParameterFlags) have changed
	 * The host invalidates all caches of parameter infos and asks the edit controller for the
	 * current infos.
	 * [SDK 3.0.0] */
	kParamTitlesChanged = 1 << 4,

	/** MIDI Controllers and/or Program Changes Assignments have changed
	 * The plug-in informs the host that its MIDI-CC mapping has changed (for example after a MIDI learn or new loaded preset) 
	 * or if the stepCount or UnitID of a ProgramChange parameter has changed.
	 * The host has to rebuild the MIDI-CC => parameter mapping (getMidiControllerAssignment)
	 * and reread program changes parameters (stepCount and associated unitID)
	 * [SDK 3.0.1] */
	kMidiCCAssignmentChanged	= 1 << 5,

	/** Note Expression has changed (info, count, PhysicalUIMapping, ...)
	 * Either the note expression type info, the count of note expressions or the physical UI mapping has changed.
	 * The host invalidates all caches of note expression infos and asks the edit controller for the current ones.
	 * See INoteExpressionController, NoteExpressionTypeInfo and INoteExpressionPhysicalUIMapping
	 * [SDK 3.5.0] */
	kNoteExpressionChanged		= 1 << 6,

	/** Input / Output bus titles have changed
	 * The host invalidates all caches of bus titles and asks the edit controller for the current titles.
	 * [SDK 3.5.0] */
	kIoTitlesChanged			= 1 << 7,

	/** Prefetch support has changed
	 * The plug-in informs the host that its PrefetchSupport has changed
	 * The host has to deactivate the plug-in, calls IPrefetchableSupport::getPrefetchableSupport 
	 * and reactivate the plug-in.
	 * See IPrefetchableSupport
	 * [SDK 3.6.1] */
	kPrefetchableSupportChanged	= 1 << 8,

	/** RoutingInfo has changed
	 * The plug-in informs the host that its internal routing (relation of an event-input-channel to
	 * an audio-output-bus) has changed. The host asks the plug-in for the new routing with 
	 * IComponent::getRoutingInfo, \ref vst3Routing
	 * See IComponent
	 * [SDK 3.6.6] */
	kRoutingInfoChanged			= 1 << 9,

	/** Key switches has changed (info, count)
	 * Either the Key switches info, the count of Key switches has changed.
	 * The host invalidates all caches of Key switches infos and asks the edit controller
	 * (IKeyswitchController) for the current ones.
	 * See IKeyswitchController
	 * [SDK 3.7.3] */
	kKeyswitchChanged			= 1 << 10,

	/** Mapping of ParamID has changed
	 * The Plug-in informs the host that its parameters ID has changed. This has to be called by the
	 * edit controller in the method setComponentState or setState (during projects loading) when the
	 * plug-in detects that the given state was associated to an older version of the plug-in, or to a
	 * plug-in to replace (for ex. migrating VST2 => VST3), with a different set of parameter IDs, then
	 * the host could remap any used parameters like automation by asking the IRemapParamID interface
	 * (which extends IEditController).
	 * See IRemapParamID
	 * [SDK 3.7.11] */
	kParamIDMappingChanged		= 1 << 11
};

//------------------------------------------------------------------------
/** Host callback interface for an edit controller: Vst::IComponentHandler
\ingroup vstIHost vst300
- [host imp]
- [released: 3.0.0]
- [mandatory]

Allow transfer of parameter editing to component (processor) via host and support automation.
Cause the host to react on configuration changes (restartComponent).

\see \ref IEditController
*/
class IComponentHandler : public FUnknown
{
public:
//------------------------------------------------------------------------
	/** To be called before calling a performEdit (e.g. on mouse-click-down event).
	 * This must be called in the UI-Thread context!
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API beginEdit (ParamID id /*in*/) = 0;

	/** Called between beginEdit and endEdit to inform the handler that a given parameter has a new
	 * value. This must be called in the UI-Thread context!
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API performEdit (ParamID id /*in*/,
	                                        ParamValue valueNormalized /*in*/) = 0;

	/** To be called after calling a performEdit (e.g. on mouse-click-up event).
	 * This must be called in the UI-Thread context!
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API endEdit (ParamID id /*in*/) = 0;

	/** Instructs host to restart the component. This must be called in the UI-Thread context!
	 * @param[in] flags is a combination of RestartFlags
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API restartComponent (int32 flags /*in*/) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IComponentHandler, 0x93A0BEA3, 0x0BD045DB, 0x8E890B0C, 0xC1E46AC6)

//------------------------------------------------------------------------
/** Extended host callback interface for an edit controller: Vst::IComponentHandler2
\ingroup vstIHost vst310
- [host imp]
- [extends IComponentHandler]
- [released: 3.1.0]
- [optional]

One part handles:
- Setting dirty state of the plug-in
- Requesting the host to open the editor

The other part handles parameter group editing from the plug-in UI. It wraps a set of \ref
IComponentHandler::beginEdit / \ref Steinberg::Vst::IComponentHandler::performEdit / \ref
Steinberg::Vst::IComponentHandler::endEdit functions (see \ref IComponentHandler) which should use
the same timestamp in the host when writing automation. This allows for better synchronizing of
multiple parameter changes at once.

\section IComponentHandler2Example Examples of different use cases

\code{.cpp}
//--------------------------------------
// we are in the editcontroller...
// in case of multiple switch buttons (with associated ParamID 1 and 3)
// on mouse down :
hostHandler2->startGroupEdit ();
hostHandler->beginEdit (1);
hostHandler->beginEdit (3);
hostHandler->performEdit (1, 1.0);
hostHandler->performEdit (3, 0.0); // the opposite of paramID 1 for example
....
// on mouse up :
hostHandler->endEdit (1);
hostHandler->endEdit (3);
hostHandler2->finishGroupEdit ();
....
....
//--------------------------------------
// in case of multiple faders (with associated ParamID 1 and 3)
// on mouse down :
hostHandler2->startGroupEdit ();
hostHandler->beginEdit (1);
hostHandler->beginEdit (3);
hostHandler2->finishGroupEdit ();
....
// on mouse move :
hostHandler2->startGroupEdit ();
hostHandler->performEdit (1, x); // x the wanted value
hostHandler->performEdit (3, x);
hostHandler2->finishGroupEdit ();
....
// on mouse up :
hostHandler2->startGroupEdit ();
hostHandler->endEdit (1);
hostHandler->endEdit (3);
hostHandler2->finishGroupEdit ();
\endcode
\see \ref IComponentHandler, \ref IEditController
*/
class IComponentHandler2 : public FUnknown
{
public:
//------------------------------------------------------------------------
	/** Tells host that the plug-in is dirty (something besides parameters has changed since last
	 * save), if true the host should apply a save before quitting.
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API setDirty (TBool state /*in*/) = 0;

	/** Tells host that it should open the plug-in editor the next time it's possible. You should
	 * use this instead of showing an alert and blocking the program flow (especially on loading
	 * projects).
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API requestOpenEditor (FIDString name = ViewType::kEditor /*in*/) = 0;

//------------------------------------------------------------------------
	/** Starts the group editing (call before a \ref IComponentHandler::beginEdit),
	 * the host will keep the current timestamp at this call and will use it for all
	 * \ref IComponentHandler::beginEdit, \ref IComponentHandler::performEdit,
	 * \ref IComponentHandler::endEdit calls until a \ref finishGroupEdit ().
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API startGroupEdit () = 0;

	/** Finishes the group editing started by a \ref startGroupEdit (call after a \ref
	 * IComponentHandler::endEdit).
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API finishGroupEdit () = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IComponentHandler2, 0xF040B4B3, 0xA36045EC, 0xABCDC045, 0xB4D5A2CC)

//------------------------------------------------------------------------
/** Extended host callback interface for an edit controller: Vst::IComponentHandlerBusActivation
\ingroup vstIHost vst368
- [host imp]
- [extends IComponentHandler]
- [released: 3.6.8]
- [optional]

Allows the plug-in to request the host to activate or deactivate a specific bus. 
If the host accepts this request, it will call later on \ref IComponent::activateBus. 
This is particularly useful for instruments with more than 1 outputs, where the user could request
from the plug-in UI a given output bus activation.

\code{.cpp}
	// somewhere in your code when you need to inform the host to enable a specific Bus.
	FUnknownPtr<IComponentHandlerBusActivation> busActivation (componentHandler);
	if (busActivation)
	{
		// here we want to activate our audio input sidechain (the 2cd input bus: index 1)
		busActivation->requestBusActivation (kAudio, kInput, 1, true);
	}
\endcode
\see \ref IComponentHandler
*/
class IComponentHandlerBusActivation : public FUnknown
{
public:
//------------------------------------------------------------------------
	/** request the host to activate or deactivate a specific bus.
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API requestBusActivation (MediaType type /*in*/, BusDirection dir /*in*/,
	                                                 int32 index /*in*/, TBool state /*in*/) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IComponentHandlerBusActivation, 0x067D02C1, 0x5B4E274D, 0xA92D90FD, 0x6EAF7240)

//------------------------------------------------------------------------
/** Extended host callback interface for an edit controller: Vst::IProgress
\ingroup vstIHost vst370
- [host imp]
- [extends IComponentHandler]
- [released: 3.7.0]
- [optional]

Allows the plug-in to request the host to create a progress for some specific tasks which take some
time. The host can visualize the progress as read-only UI elements.
For example, after loading a project where a plug-in needs to load extra data (e.g. samples) in a
background thread, this enables the host to get and visualize the current status of the loading
progress and to inform the user when the loading is finished.
Note: During the progress, the host can unload the plug-in at any time. Make sure that the plug-in
supports this use case.

\section IProgressExample Example

\code{.cpp}
//--------------------------------------
// we are in the editcontroller:
// as member: IProgress::ID mProgressID;

FUnknownPtr<IProgress> progress (componentHandler);
if (progress)
    progress->start (IProgress::ProgressType::UIBackgroundTask, STR ("Load Samples..."),
                     mProgressID);

// ...
myProgressValue += incProgressStep;
FUnknownPtr<IProgress> progress (componentHandler);
if (progress)
    progress->update (mProgressID, myProgressValue);

// ...
FUnknownPtr<IProgress> progress (componentHandler);
if (progress)
    progress->finish (mProgressID);
\endcode

\see \ref IComponentHandler
*/
class IProgress : public FUnknown
{
public:
	//------------------------------------------------------------------------
	enum ProgressType : uint32
	{
		/** plug-in state is restored async (in a background Thread) */
		AsyncStateRestoration = 0,

		/** a plug-in task triggered by a UI action */
		UIBackgroundTask
	};

	using ID = uint64;

	/** Start a new progress of a given type and optional Description. outID is as ID created by the
	 * host to identify this newly created progress (for update and finish method).
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API start (ProgressType type /*in*/,
	                                  const tchar* optionalDescription /*in*/,
	                                  ID& outID /*out*/) = 0;

	/** Update the progress value (normValue between [0, 1]) associated to the given id.
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API update (ID id /*in*/, ParamValue normValue /*in*/) = 0;

	/** Finish the progress associated to the given id.
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API finish (ID id /*in*/) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IProgress, 0x00C9DC5B, 0x9D904254, 0x91A388C8, 0xB4E91B69)

//------------------------------------------------------------------------
/** Edit controller component interface: Vst::IEditController
\ingroup vstIPlug vst300
- [plug imp]
- [released: 3.0.0]
- [mandatory]

The controller part of an effect or instrument with parameter handling (export, definition,
conversion, ...). \see \ref IComponent::getControllerClassId, \ref IMidiMapping
*/
class IEditController : public IPluginBase
{
public:
//------------------------------------------------------------------------
	/** Receives the component state.
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API setComponentState (IBStream* state /*in*/) = 0;

	/** Sets the controller state.
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API setState (IBStream* state /*in*/) = 0;

	/** Gets the controller state.
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API getState (IBStream* state /*inout*/) = 0;

	// parameters -------------------------
	/** Returns the number of parameters exported.
	 * \note [UI-thread & Connected] */
	virtual int32 PLUGIN_API getParameterCount () = 0;

	/** Gets for a given index the parameter information.
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API getParameterInfo (int32 paramIndex /*in*/,
	                                             ParameterInfo& info /*out*/) = 0;

	/** Gets for a given paramID and normalized value its associated string representation.
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API getParamStringByValue (ParamID id /*in*/,
	                                                  ParamValue valueNormalized /*in*/,
	                                                  String128 string /*out*/) = 0;

	/** Gets for a given paramID and string its normalized value.
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API getParamValueByString (ParamID id /*in*/, TChar* string /*in*/,
	                                                  ParamValue& valueNormalized /*out*/) = 0;

	/** Returns for a given paramID and a normalized value its plain representation
	 * (for example -6 for -6dB - see \ref vst3AutomationIntro).
	 * \note [UI-thread & Connected] */
	virtual ParamValue PLUGIN_API normalizedParamToPlain (ParamID id /*in*/,
	                                                      ParamValue valueNormalized /*in*/) = 0;

	/** Returns for a given paramID and a plain value its normalized value. (see \ref
	 * vst3AutomationIntro).
	 * \note [UI-thread & Connected] */
	virtual ParamValue PLUGIN_API plainParamToNormalized (ParamID id /*in*/,
	                                                      ParamValue plainValue /*in*/) = 0;

	/** Returns the normalized value of the parameter associated to the paramID.
	 * \note [UI-thread & Connected] */
	virtual ParamValue PLUGIN_API getParamNormalized (ParamID id /*in*/) = 0;

	/** Sets the normalized value to the parameter associated to the paramID. The controller must
	 * never pass this value-change back to the host via the IComponentHandler.
	 * It should update the according GUI element(s) only!
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API setParamNormalized (ParamID id /*in*/, ParamValue value /*in*/) = 0;

	// handler ----------------------------
	/** Gets from host a handler which allows the Plugin-in to communicate with the host.
	 * \note This is mandatory if the host is using the IEditController!
	 * \note [UI-thread & Initialized] */
	virtual tresult PLUGIN_API setComponentHandler (IComponentHandler* handler /*in*/) = 0;

	// view -------------------------------
	/** Creates the editor view of the plug-in, currently only "editor" is supported, see \ref
	 * ViewType. The life time of the editor view will never exceed the life time of this controller
	 * instance.
	 * \note [UI-thread & Connected] */
	virtual IPlugView* PLUGIN_API createView (FIDString name /*in*/) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IEditController, 0xDCD7BBE3, 0x7742448D, 0xA874AACC, 0x979C759E)

//------------------------------------------------------------------------
/** \ingroup vst3typedef */
/**@{*/
/** Knob Mode Type */
using KnobMode = int32;
/**@}*/

//------------------------------------------------------------------------
/** Knob Mode */
enum KnobModes : KnobMode
{
	kCircularMode = 0,		///< Circular with jump to clicked position
	kRelativCircularMode,	///< Circular without jump to clicked position
	kLinearMode				///< Linear: depending on vertical movement
};

//------------------------------------------------------------------------
/** Edit controller component interface extension: Vst::IEditController2
\ingroup vstIPlug vst310
- [plug imp]
- [extends IEditController]
- [released: 3.1.0]
- [optional]

Extension to allow the host to inform the plug-in about the host Knob Mode,
and to open the plug-in about box or help documentation.

\see \ref IEditController, \ref EditController
*/
class IEditController2 : public FUnknown
{
public:
	/** Host could set the Knob Mode for the plug-in. Return kResultFalse means not supported mode.
	 * \see KnobModes.
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API setKnobMode (KnobMode mode /*in*/) = 0;

	/** Host could ask to open the plug-in help (could be: opening a PDF document or link to a web
	 * page). The host could call it with onlyCheck set to true for testing support of open Help.
	 * Return kResultFalse means not supported function.
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API openHelp (TBool onlyCheck /*in*/) = 0;

	/** Host could ask to open the plug-in about box.
	 * The host could call it with onlyCheck set to true for testing support of open AboutBox.
	 * Return kResultFalse means not supported function.
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API openAboutBox (TBool onlyCheck /*in*/) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IEditController2, 0x7F4EFE59, 0xF3204967, 0xAC27A3AE, 0xAFB63038)

//------------------------------------------------------------------------
/** MIDI Mapping interface: Vst::IMidiMapping
\ingroup vstIPlug vst301
- [plug imp]
- [extends IEditController]
- [released: 3.0.1]
- [optional]

MIDI controllers are not transmitted directly to a VST component. MIDI as hardware protocol has
restrictions that can be avoided in software. Controller data in particular come along with unclear
and often ignored semantics. On top of this they can interfere with regular parameter automation and
the host is unaware of what happens in the plug-in when passing MIDI controllers directly.

So any functionality that is to be controlled by MIDI controllers must be exported as regular
parameter. The host will transform incoming MIDI controller data using this interface and transmit
them as regular parameter change. This allows the host to automate them in the same way as other
parameters. CtrlNumber can be a typical MIDI controller value extended to some others values like
pitchbend or aftertouch (see \ref ControllerNumbers). If the mapping has changed, the plug-in must
call IComponentHandler::restartComponent (kMidiCCAssignmentChanged) to inform the host about this
change.

\section IMidiMappingExample Example

\code{.cpp}
//--------------------------------------
// in myeditcontroller.h
class MyEditController: public EditControllerEx1, public IMidiMapping
{
    //...
    //---IMidiMapping---------------------------
    tresult PLUGIN_API getMidiControllerAssignment (int32 busIndex, int16 channel,
                                                    CtrlNumber midiControllerNumber,
                                                    ParamID& id) override;
    //---Interface---------
    OBJ_METHODS (MyEditController, EditControllerEx1)
    DEFINE_INTERFACES
        DEF_INTERFACE (IMidiMapping)
    END_DEFINE_INTERFACES (MyEditController)
    REFCOUNT_METHODS (MyEditController)
};

//--------------------------------------
// in myeditcontroller.cpp
tresult PLUGIN_API MyEditController::getMidiControllerAssignment (int32 busIndex,
                                                                  int16 midiChannel,
                                                                  CtrlNumber midiControllerNumber,
                                                                  ParamID& tag)
{
    // for my first Event bus and for MIDI channel 0 and for MIDI CC Volume only
    if (busIndex == 0 && midiChannel == 0 && midiControllerNumber == kCtrlVolume)
    {
        tag = kGainId;
        return kResultTrue;
    }
    return kResultFalse;
}
\endcode
*/
class IMidiMapping : public FUnknown
{
public:
	/** Gets an (preferred) associated ParamID for a given Input Event Bus index, channel and MIDI
	 * Controller.
	 * @param[in] busIndex - index of Input Event Bus
	 * @param[in] channel - channel of the bus
	 * @param[in] midiControllerNumber - see \ref ControllerNumbers for expected values (could be
	 * bigger than 127)
	 * @param[out] id - return the associated ParamID to the given midiControllerNumber
	 * 
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API getMidiControllerAssignment (int32 busIndex /*in*/,
	                                                        int16 channel /*in*/,
	                                                        CtrlNumber midiControllerNumber /*in*/,
	                                                        ParamID& id /*out*/) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IMidiMapping, 0xDF0FF9F7, 0x49B74669, 0xB63AB732, 0x7ADBF5E5)

//------------------------------------------------------------------------
/** Parameter Editing from host: Vst::IEditControllerHostEditing
\ingroup vstIPlug vst350
- [plug imp]
- [extends IEditController]
- [released: 3.5.0]
- [optional]

If this interface is implemented by the edit controller, and when performing edits from outside
the plug-in (host / remote) of a not automatable and not read-only, and not hidden flagged parameter
(kind of helper parameter), the host will start with a beginEditFromHost before calling
setParamNormalized and end with an endEditFromHost. Here the sequence that the host will call:

\section IEditControllerExample Example

\code{.cpp}
//------------------------------------------------------------------------
plugEditController->beginEditFromHost (id);
plugEditController->setParamNormalized (id, value);
plugEditController->setParamNormalized (id, value + 0.1);
// ...
plugEditController->endEditFromHost (id);
\endcode

\see \ref IEditController
*/
class IEditControllerHostEditing : public FUnknown
{
public:
	/** Called before a setParamNormalized sequence, a endEditFromHost will be call at the end of
	 * the editing action.
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API beginEditFromHost (ParamID paramID /*in*/) = 0;

	/** Called after a beginEditFromHost and a sequence of setParamNormalized.
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API endEditFromHost (ParamID paramID /*in*/) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IEditControllerHostEditing, 0xC1271208, 0x70594098, 0xB9DD34B3, 0x6BB0195E)

//------------------------------------------------------------------------
/** Extended plug-in interface IComponentHandler for an edit controller
\ingroup vstIHost vst379
- [host imp]
- [extends IComponentHandler]
- [released: 3.7.9]
- [optional]
*/
//------------------------------------------------------------------------
class IComponentHandlerSystemTime : public FUnknown
{
public:
//------------------------------------------------------------------------
	/** get the current systemTime (the same as the one used in ProcessContext::systemTime).
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API getSystemTime (int64& systemTime /*out*/) = 0;
//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IComponentHandlerSystemTime, 0xF9E53056, 0xD1554CD5, 0xB7695E1B, 0x7B0F7745)

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpop.h"
//------------------------------------------------------------------------
