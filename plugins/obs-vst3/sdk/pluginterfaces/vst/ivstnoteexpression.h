//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Interfaces
// Filename    : pluginterfaces/vst/ivstnoteexpression.h
// Created by  : Steinberg, 10/2010
// Description : VST Note Expression Interfaces
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
/** \ingroup vst3typedef */
/**@{*/
/** Note Expression Types */
typedef uint32 NoteExpressionTypeID;
/** Note Expression Value */
typedef double NoteExpressionValue;
/**@}*/

//------------------------------------------------------------------------
/** NoteExpressionTypeIDs describes the type of the note expression.
VST predefines some types like volume, pan, tuning by defining their ranges and curves.
Used by NoteExpressionEvent::typeId and NoteExpressionTypeID::typeId
\see NoteExpressionTypeInfo
*/
enum NoteExpressionTypeIDs : uint32
{
	kVolumeTypeID = 0,		///< Volume, plain range [0 = -oo , 0.25 = 0dB, 0.5 = +6dB, 1 = +12dB]: plain = 20 * log (4 * norm)
	kPanTypeID,				///< Panning (L-R), plain range [0 = left, 0.5 = center, 1 = right]
	kTuningTypeID,			///< Tuning, plain range [0 = -120.0 (ten octaves down), 0.5 none, 1 = +120.0 (ten octaves up)]
							///< plain = 240 * (norm - 0.5) and norm = plain / 240 + 0.5
							///< oneOctave is 12.0 / 240.0; oneHalfTune = 1.0 / 240.0;
	kVibratoTypeID,			///< Vibrato
	kExpressionTypeID,		///< Expression
	kBrightnessTypeID,		///< Brightness
	kTextTypeID,			///< See NoteExpressionTextEvent
	kPhonemeTypeID,			///< TODO:

	kCustomStart = 100000,	///< start of custom note expression type ids
	kCustomEnd   = 200000,  ///< end of custom note expression type ids
	
	kInvalidTypeID = 0xFFFFFFFF	///< indicates an invalid note expression type
};

//------------------------------------------------------------------------
/** Description of a Note Expression Type
This structure is part of the NoteExpressionTypeInfo structure, it describes for given
NoteExpressionTypeID its default value (for example 0.5 for a kTuningTypeID (kIsBipolar: centered)),
its minimum and maximum (for predefined NoteExpressionTypeID the full range is predefined too) and a
stepCount when the given NoteExpressionTypeID is limited to discrete values (like on/off state).
\see NoteExpressionTypeInfo
*/
struct NoteExpressionValueDescription
{
	NoteExpressionValue defaultValue;	///< default normalized value [0,1]
	NoteExpressionValue minimum;		///< minimum normalized value [0,1]
	NoteExpressionValue maximum;		///< maximum normalized value [0,1]
	int32 stepCount;					///< number of discrete steps (0: continuous, 1: toggle, discrete value otherwise - see \ref vst3ParameterIntro)
};

#if SMTG_OS_WINDOWS && !SMTG_PLATFORM_64
#include "pluginterfaces/vst/vstpshpack4.h"
#endif
//------------------------------------------------------------------------
/** Note Expression Value event. Used in \ref Event (union)
A note expression event affects one single playing note (referring its noteId).
This kind of event is send from host to the plug-in like other events (NoteOnEvent,
NoteOffEvent,...) in \ref ProcessData during the process call. Note expression events for a specific
noteId can only occur after a NoteOnEvent. The host must take care that the event list (\ref
IEventList) is properly sorted. Expression events are always absolute normalized values [0.0, 1.0].
The predefined types have a predefined mapping of the normalized values (see \ref
NoteExpressionTypeIDs) \sa INoteExpressionController
*/
struct NoteExpressionValueEvent
{
	NoteExpressionTypeID typeId;	///< see \ref NoteExpressionTypeID
	int32 noteId;					///< associated note identifier to apply the change

	NoteExpressionValue value;		///< normalized value [0.0, 1.0].
};

//------------------------------------------------------------------------
/** Note Expression Int event. Used in \ref Event (union)
Same as NoteExpressionValueEvent but use a uint64 instead of a NoteExpressionValue (double)
\ingroup vstIPlug vst380
- [released: 3.8.0]
*/
struct NoteExpressionIntValueEvent
{
	NoteExpressionTypeID typeId;	///< see \ref NoteExpressionTypeID
	int32 noteId;					///< associated note identifier to apply the change

	uint64 value;
};

//------------------------------------------------------------------------
/** Note Expression Text event. Used in Event (union)
A Expression event affects one single playing note. \sa INoteExpressionController

\see NoteExpressionTypeInfo
*/
struct NoteExpressionTextEvent
{
	NoteExpressionTypeID typeId;	///< see \ref NoteExpressionTypeID (kTextTypeID or kPhoneticTypeID)
	int32 noteId;					///< associated note identifier to apply the change

	uint32 textLen;					///< the number of characters (TChar) between the beginning of text and the terminating
									///< null character (without including the terminating null character itself)

	const TChar* text;				///< UTF-16, null terminated
};

#if SMTG_OS_WINDOWS && !SMTG_PLATFORM_64
#include "pluginterfaces/base/falignpop.h"
#endif

//------------------------------------------------------------------------
/** NoteExpressionTypeInfo is the structure describing a note expression supported by the plug-in.
This structure is used by the method \ref INoteExpressionController::getNoteExpressionInfo.
\see INoteExpressionController
*/
struct NoteExpressionTypeInfo
{
	NoteExpressionTypeID typeId;			///< unique identifier of this note Expression type
	String128 title;						///< note Expression type title (e.g. "Volume")
	String128 shortTitle;					///< note Expression type short title (e.g. "Vol")
	String128 units;						///< note Expression type unit (e.g. "dB")
	int32 unitId;							///< id of unit this NoteExpression belongs to (see \ref vst3Units), in order to sort the note expression, it is possible to use unitId like for parameters. -1 means no unit used.
	NoteExpressionValueDescription valueDesc;	///< value description see \ref NoteExpressionValueDescription
	ParamID associatedParameterId;			///< optional associated parameter ID (for mapping from note expression to global (using the parameter automation for example) and back). Only used when kAssociatedParameterIDValid is set in flags.

	int32 flags;							///< NoteExpressionTypeFlags (see below)
	enum NoteExpressionTypeFlags
	{
		kIsBipolar		= 1 << 0,			///< event is bipolar (centered), otherwise unipolar
		kIsOneShot		= 1 << 1,			///< event occurs only one time for its associated note (at begin of the noteOn)
		kIsAbsolute		= 1 << 2,			///< This note expression will apply an absolute change to the sound (not relative (offset))
		kAssociatedParameterIDValid	= 1 << 3,///< indicates that the associatedParameterID is valid and could be used
	};
};

//------------------------------------------------------------------------
/** Extended plug-in interface IEditController for note expression event support:
Vst::INoteExpressionController
\ingroup vstIPlug vst350
- [plug imp]
- [extends IEditController]
- [released: 3.5.0]
- [optional]

With this plug-in interface, the host can retrieve all necessary note expression information
supported by the plug-in. Note expression information (\ref NoteExpressionTypeInfo) are specific for
given channel and event bus.

Note that there is only one NoteExpressionTypeID per given channel of an event bus.

The method getNoteExpressionStringByValue allows conversion from a normalized value to a string
representation and the getNoteExpressionValueByString method from a string to a normalized value.

When the note expression state changes (for example when switching presets) the plug-in needs
to inform the host about it via \ref IComponentHandler::restartComponent (kNoteExpressionChanged).
*/
class INoteExpressionController : public FUnknown
{
public:
	/** Returns number of supported note change types for event bus index and channel.
	 * \note [UI-thread & Connected] */
	virtual int32 PLUGIN_API getNoteExpressionCount (int32 busIndex /*in*/,
	                                                 int16 channel /*in*/) = 0;

	/** Returns note change type info.
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API getNoteExpressionInfo (int32 busIndex /*in*/, int16 channel /*in*/,
	                                                  int32 noteExpressionIndex /*in*/,
	                                                  NoteExpressionTypeInfo& info /*out*/) = 0;

	/** Gets a user readable representation of the normalized note change value.
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API getNoteExpressionStringByValue (
	    int32 busIndex /*in*/, int16 channel /*in*/, NoteExpressionTypeID id /*in*/,
	    NoteExpressionValue valueNormalized /*in*/, String128 string /*out*/) = 0;

	/** Converts the user readable representation to the normalized note change value.
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API getNoteExpressionValueByString (
	    int32 busIndex /*in*/, int16 channel /*in*/, NoteExpressionTypeID id /*in*/,
	    const TChar* string /*in*/, NoteExpressionValue& valueNormalized /*out*/) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (INoteExpressionController, 0xB7F8F859, 0x41234872, 0x91169581, 0x4F3721A3)

//------------------------------------------------------------------------
//------------------------------------------------------------------------
/** KeyswitchTypeIDs describes the type of a key switch
\see KeyswitchInfo
*/
enum KeyswitchTypeIDs : uint32
{
	kNoteOnKeyswitchTypeID = 0,	///< press before noteOn is played
	kOnTheFlyKeyswitchTypeID,	///< press while noteOn is played
	kOnReleaseKeyswitchTypeID,	///< press before entering release
	kKeyRangeTypeID				///< key should be maintained pressed for playing
};

/** \ingroup vst3typedef */
typedef uint32 KeyswitchTypeID;

//------------------------------------------------------------------------
/** KeyswitchInfo is the structure describing a key switch
This structure is used by the method \ref IKeyswitchController::getKeyswitchInfo.
\see IKeyswitchController
*/
struct KeyswitchInfo
{
	KeyswitchTypeID typeId;		///< see KeyswitchTypeID
	String128 title;			///< name of key switch (e.g. "Accentuation")
	String128 shortTitle;		///< short title (e.g. "Acc")

	int32 keyswitchMin;			///< associated main key switch min (value between [0, 127])
	int32 keyswitchMax;			///< associated main key switch max (value between [0, 127])
	int32 keyRemapped;			/** optional remapped key switch (default -1), the plug-in could provide one remapped
								key for a key switch (allowing better location on the keyboard of the key switches) */

	int32 unitId;				///< id of unit this key switch belongs to (see \ref vst3Units), -1 means no unit used.

	int32 flags;				///< not yet used (set to 0)
};

//------------------------------------------------------------------------
/** Extended plug-in interface IEditController for key switches support: Vst::IKeyswitchController
\ingroup vstIPlug vst350
- [plug imp]
- [extends IEditController]
- [released: 3.5.0]
- [optional]

When a (instrument) plug-in supports such interface, the host could get from the plug-in the current
set of used key switches (megatrig/articulation) for a given channel of a event bus and then
automatically use them (like in Cubase 6) to create VST Expression Map (allowing to associated
symbol to a given articulation / key switch).
*/
class IKeyswitchController : public FUnknown
{
public:
	/** Returns number of supported key switches for event bus index and channel.
	 * \note [UI-thread & Connected] */
	virtual int32 PLUGIN_API getKeyswitchCount (int32 busIndex /*in*/, int16 channel /*in*/) = 0;

	/** Returns key switch info.
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API getKeyswitchInfo (int32 busIndex /*in*/, int16 channel /*in*/,
	                                             int32 keySwitchIndex /*in*/,
	                                             KeyswitchInfo& info /*out*/) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IKeyswitchController, 0x1F2F76D3, 0xBFFB4B96, 0xB99527A5, 0x5EBCCEF4)

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpop.h"
//------------------------------------------------------------------------
