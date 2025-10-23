//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Interfaces
// Filename    : pluginterfaces/vst/ivstevents.h
// Created by  : Steinberg, 11/2005
// Description : VST Events Interfaces
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/vst/ivstnoteexpression.h"

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpush.h"
//------------------------------------------------------------------------

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** Reserved note identifier (noteId) range for a plug-in. Guaranteed not used by the host. */
enum NoteIDUserRange
{
	kNoteIDUserRangeLowerBound = -10000, 
	kNoteIDUserRangeUpperBound = -1000,
};

//------------------------------------------------------------------------
/** Note-on event specific data. Used in \ref Event (union)
\ingroup vstEventGrp
Pitch uses the twelve-tone equal temperament tuning (12-TET).
 */
struct NoteOnEvent
{
	int16 channel;		///< channel index in event bus
	int16 pitch;		///< range [0, 127] = [C-2, G8] with A3=440Hz (12-TET: twelve-tone equal temperament)
	float tuning;		///< 1.f = +1 cent, -1.f = -1 cent
	float velocity;		///< range [0.0, 1.0]
	int32 length;		///< in sample frames (optional, Note Off has to follow in any case!)
	int32 noteId;		///< note identifier (if not available then -1)
};

//------------------------------------------------------------------------
/** Note-off event specific data. Used in \ref Event (union)
\ingroup vstEventGrp 
*/
struct NoteOffEvent
{
	int16 channel;		///< channel index in event bus
	int16 pitch;		///< range [0, 127] = [C-2, G8] with A3=440Hz (12-TET)
	float velocity;		///< range [0.0, 1.0]
	int32 noteId;		///< associated noteOn identifier (if not available then -1)
	float tuning;		///< 1.f = +1 cent, -1.f = -1 cent
};

//------------------------------------------------------------------------
/** Data event specific data. Used in \ref Event (union)
\ingroup vstEventGrp 
*/
struct DataEvent
{
	uint32 size;		///< size in bytes of the data block bytes
	uint32 type;		///< type of this data block (see \ref DataTypes)
	const uint8* bytes;	///< pointer to the data block

	/** Value for DataEvent::type */
	enum DataTypes
	{
		kMidiSysEx = 0	///< for MIDI system exclusive message
	};
};

//------------------------------------------------------------------------
/** PolyPressure event specific data. Used in \ref Event (union)
\ingroup vstEventGrp
*/
struct PolyPressureEvent
{
	int16 channel;		///< channel index in event bus
	int16 pitch;		///< range [0, 127] = [C-2, G8] with A3=440Hz
	float pressure;		///< range [0.0, 1.0]
	int32 noteId;		///< event should be applied to the noteId (if not -1)
};

//------------------------------------------------------------------------
/** Chord event specific data. Used in \ref Event (union)
\ingroup vstEventGrp 
*/
struct ChordEvent
{
	int16 root;			///< range [0, 127] = [C-2, G8] with A3=440Hz
	int16 bassNote;		///< range [0, 127] = [C-2, G8] with A3=440Hz
	int16 mask;			///< root is bit 0
	uint16 textLen;		///< the number of characters (TChar) between the beginning of text and the terminating
						///< null character (without including the terminating null character itself)
	const TChar* text;	///< UTF-16, null terminated Hosts Chord Name
};

//------------------------------------------------------------------------
/** Scale event specific data. Used in \ref Event (union)
\ingroup vstEventGrp 
*/
struct ScaleEvent
{
	int16 root;			///< range [0, 127] = root Note/Transpose Factor
	int16 mask;			///< Bit 0 =  C,  Bit 1 = C#, ... (0x5ab5 = Major Scale)
	uint16 textLen;		///< the number of characters (TChar) between the beginning of text and the terminating
						///< null character (without including the terminating null character itself)
	const TChar* text;	///< UTF-16, null terminated, Hosts Scale Name
};

//------------------------------------------------------------------------
/** Legacy MIDI CC Out event specific data. Used in \ref Event (union)
\ingroup vstEventGrp
- [released: 3.6.12]

This kind of event is reserved for generating MIDI CC as output event for kEvent Bus during the process call.
 */
struct LegacyMIDICCOutEvent
{
	uint8 controlNumber;///< see enum ControllerNumbers [0, 255]
	int8 channel;		///< channel index in event bus [0, 15]
	int8 value;			///< value of Controller [0, 127]
	int8 value2;		///< [0, 127] used for pitch bend (kPitchBend) and polyPressure (kCtrlPolyPressure)
};

//------------------------------------------------------------------------
/** Event 
\ingroup vstEventGrp
Structure representing a single Event of different types associated to a specific event (\ref kEvent) bus.
*/
struct Event
{
	int32 busIndex;				///< event bus index
	int32 sampleOffset;			///< sample frames related to the current block start sample position
	TQuarterNotes ppqPosition;	///< position in project
	uint16 flags;				///< combination of \ref EventFlags

	/** Event Flags - used for Event::flags */
	enum EventFlags
	{
		kIsLive = 1 << 0,			///< indicates that the event is played live (directly from keyboard)

		kUserReserved1 = 1 << 14,	///< reserved for user (for internal use)
		kUserReserved2 = 1 << 15	///< reserved for user (for internal use)
	};

	/** Event Types - used for Event::type */
	enum EventTypes
	{
		kNoteOnEvent       = 0,			///< is \ref NoteOnEvent
		kNoteOffEvent      = 1,			///< is \ref NoteOffEvent
		kDataEvent         = 2,			///< is \ref DataEvent
		kPolyPressureEvent = 3,			///< is \ref PolyPressureEvent
		kNoteExpressionValueEvent = 4,	///< is \ref NoteExpressionValueEvent
		kNoteExpressionTextEvent  = 5,	///< is \ref NoteExpressionTextEvent
		kChordEvent        = 6,			///< is \ref ChordEvent
		kScaleEvent        = 7,			///< is \ref ScaleEvent
		kNoteExpressionIntValueEvent = 8,///< is \ref NoteExpressionIntValueEvent
		kLegacyMIDICCOutEvent = 65535	///< is \ref LegacyMIDICCOutEvent
	};

	uint16 type;				///< a value from \ref EventTypes
	union
	{
		NoteOnEvent noteOn;								///< type == kNoteOnEvent
		NoteOffEvent noteOff;							///< type == kNoteOffEvent
		DataEvent data;									///< type == kDataEvent
		PolyPressureEvent polyPressure;					///< type == kPolyPressureEvent
		NoteExpressionValueEvent noteExpressionValue;	///< type == kNoteExpressionValueEvent
		NoteExpressionTextEvent noteExpressionText;		///< type == kNoteExpressionTextEvent
		NoteExpressionIntValueEvent noteExpressionIntValue;	///< type == kNoteExpressionIntValueEvent
		ChordEvent chord;								///< type == kChordEvent
		ScaleEvent scale;								///< type == kScaleEvent
		LegacyMIDICCOutEvent midiCCOut;					///< type == kLegacyMIDICCOutEvent
	};
};

//------------------------------------------------------------------------
/** List of events to process: Vst::IEventList
\ingroup vstIHost vst300
- [host imp]
- [released: 3.0.0]
- [mandatory]

\see ProcessData, Event
*/
class IEventList : public FUnknown
{
public:
//------------------------------------------------------------------------
	/** Returns the count of events. */
	virtual int32 PLUGIN_API getEventCount () = 0;

	/** Gets parameter by index. */
	virtual tresult PLUGIN_API getEvent (int32 index /*in*/, Event& e /*out*/) = 0;

	/** Adds a new event. */
	virtual tresult PLUGIN_API addEvent (Event& e /*in*/) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IEventList, 0x3A2C4214, 0x346349FE, 0xB2C4F397, 0xB9695A44)

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpop.h"
//------------------------------------------------------------------------
