//-----------------------------------------------------------------------------
// Project     : VST SDK
// Flags       : clang-format SMTGSequencer
// Category    : Helpers
// Filename    : public.sdk/source/vst/utility/ump.h
// Created by  : Steinberg, 12/2023
// Description : a single c++17 header UMP parser implementation without other dependencies than
// 				 to std::array
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#ifndef SMTG_ALWAYS_INLINE
#if _MSVC
#define SMTG_ALWAYS_INLINE __forceinline
#else
#define SMTG_ALWAYS_INLINE __inline__ __attribute__ ((__always_inline__))
#endif
#endif

#include <array>
#include <cassert>
#include <cstdint>

//------------------------------------------------------------------------
namespace Steinberg::Vst::UMP {

struct IUniversalMidiPacketHandler;
enum class ParsingAction : uint32_t;

//------------------------------------------------------------------------
/** the parse sections control which messages to process of UMP
 *
 *	the generated code is smaller and runtime is faster if only the needed sections are enabled.
 */
enum ParseSections : uint8_t
{
	Utility = 1 << 0,
	System = 1 << 1,
	ChannelVoice1 = 1 << 2,
	SysEx = 1 << 3,
	ChannelVoice2 = 1 << 3,
	Data128 = 1 << 4,
	All = 0xff
};

//------------------------------------------------------------------------
/** stateless parsing universal MIDI packets
 *
 *	@tparam sections	which sections to parse
 *	@param numWords		number of 32-bit words
 *	@param words		pointer to a words array
 *	@param handler		callback handler
 *	@return				number of successfully processed words
 */
template <ParseSections sections = ParseSections::All>
size_t parsePackets (const size_t numWords, const uint32_t* words,
                     const IUniversalMidiPacketHandler& handler);

//------------------------------------------------------------------------
struct IUniversalMidiPacketHandler
{
	using Group = uint8_t;
	using Channel = uint8_t;
	using Index = uint8_t;
	using NoteNumber = uint8_t;
	using BankNumber = uint8_t;
	using ControllerNumber = uint8_t;
	using Velocity8 = uint8_t;
	using Velocity16 = uint16_t;
	using AttributeType = uint8_t;
	using AttributeValue = uint16_t;
	using OptionFlags = uint8_t;
	using Data8 = uint8_t;
	using Data32 = uint32_t;
	using Program = uint8_t;
	using BankMSB = uint8_t;
	using BankLSB = uint8_t;
	using Timestamp = uint16_t;
	using Timecode = uint8_t;
	using StreamID = uint8_t;
	using SysEx6ByteData = const std::array<uint8_t, 6>&;
	using SysEx13ByteData = const std::array<uint8_t, 13>&;
	using MixedData = const std::array<uint8_t, 14>&;

	// UTILITY
	virtual void onNoop (Group group) const = 0;
	virtual void onJitterClock (Group group, Timestamp time) const = 0;
	virtual void onJitterTimestamp (Group group, Timestamp time) const = 0;

	// SYSTEM COMMON
	virtual void onMIDITimeCode (Group group, Timecode timecode) const = 0;
	virtual void onSongPositionPointer (Group group, uint8_t posLSB, uint8_t posMSB) const = 0;
	virtual void onSongSelect (Group group, uint8_t songIndex) const = 0;
	virtual void onTuneRequest (Group group) const = 0;

	// SYSTEM REALTIME
	enum class SystemRealtime
	{
		TimingClock,
		Start,
		Continue,
		Stop,
		ActiveSensing,
		Reset
	};
	virtual void onSystemRealtime (Group group, SystemRealtime which) const = 0;

	// MIDI 1.0 CHANNEL VOICE Messages
	virtual void onMidi1NoteOff (Group group, Channel channel, NoteNumber note,
	                             Velocity8 velocity) const = 0;
	virtual void onMidi1NoteOn (Group group, Channel channel, NoteNumber note,
	                            Velocity8 velocity) const = 0;
	virtual void onMidi1PolyPressure (Group group, Channel channel, NoteNumber note,
	                                  Data8 data) const = 0;
	virtual void onMidi1ControlChange (Group group, Channel channel, ControllerNumber controller,
	                                   Data8 value) const = 0;
	virtual void onMidi1ProgramChange (Group group, Channel channel, Program program) const = 0;
	virtual void onMidi1ChannelPressure (Group group, Channel channel, Data8 pressure) const = 0;
	virtual void onMidi1PitchBend (Group group, Channel channel, Data8 valueLSB,
	                               Data8 valueMSB) const = 0;

	// DATA 64 BIT
	virtual void onSysExPacket (Group group, SysEx6ByteData data) const = 0;
	virtual void onSysExStart (Group group, SysEx6ByteData data) const = 0;
	virtual void onSysExContinue (Group group, SysEx6ByteData data) const = 0;
	virtual void onSysExEnd (Group group, SysEx6ByteData data) const = 0;

	// MIDI 2.0 CHANNEL VOICE Messages
	virtual void onRegisteredPerNoteController (Group group, Channel channel, NoteNumber note,
	                                            ControllerNumber controller, Data32 data) const = 0;
	virtual void onAssignablePerNoteController (Group group, Channel channel, NoteNumber note,
	                                            ControllerNumber controller, Data32 data) const = 0;
	virtual void onRegisteredController (Group group, Channel channel, BankNumber bank, Index index,
	                                     Data32 data) const = 0;
	virtual void onAssignableController (Group group, Channel channel, BankNumber bank, Index index,
	                                     Data32 data) const = 0;
	virtual void onRelativeRegisteredController (Group group, Channel channel, BankNumber bank,
	                                             Index index, Data32 data) const = 0;
	virtual void onRelativeAssignableController (Group group, Channel channel, BankNumber bank,
	                                             Index index, Data32 data) const = 0;
	virtual void onPerNotePitchBend (Group group, Channel channel, NoteNumber note,
	                                 Data32 data) const = 0;
	virtual void onNoteOff (Group group, Channel channel, NoteNumber note, Velocity16 velocity,
	                        AttributeType attr, AttributeValue attrValue) const = 0;
	virtual void onNoteOn (Group group, Channel channel, NoteNumber note, Velocity16 velocity,
	                       AttributeType attr, AttributeValue attrValue) const = 0;
	virtual void onPolyPressure (Group group, Channel channel, NoteNumber note,
	                             Data32 data) const = 0;
	virtual void onControlChange (Group group, Channel channel, ControllerNumber controller,
	                              Data32 data) const = 0;
	virtual void onProgramChange (Group group, Channel channel, OptionFlags options,
	                              Program program, BankMSB bankMSB, BankLSB bankLSB) const = 0;
	virtual void onChannelPressure (Group group, Channel channel, Data32 data) const = 0;
	virtual void onPitchBend (Group group, Channel channel, Data32 data) const = 0;
	virtual void onPerNoteManagement (Group group, Channel channel, NoteNumber note,
	                                  OptionFlags options) const = 0;

	// DATA 128 BIT
	virtual void onSysEx8Packet (Group group, Data8 numBytes, Index streamID,
	                             SysEx13ByteData data) const = 0;
	virtual void onSysEx8Start (Group group, Data8 numBytes, Index streamID,
	                            SysEx13ByteData data) const = 0;
	virtual void onSysEx8Continue (Group group, Data8 numBytes, Index streamID,
	                               SysEx13ByteData data) const = 0;
	virtual void onSysEx8End (Group group, Data8 numBytes, Index streamID,
	                          SysEx13ByteData data) const = 0;
	virtual void onMixedDataSetHeader (Group group, Index mdsID, MixedData data) const = 0;
	virtual void onMixedDataSetPayload (Group group, Index mdsID, MixedData data) const = 0;

	// invalid input data
	virtual ParsingAction onInvalidInputData (size_t index) const = 0;
	// insufficient input data
	virtual void onInsufficentInputData (size_t index, size_t numMissingWords) const = 0;
};

//------------------------------------------------------------------------
enum class ParsingAction : uint32_t
{
	Break,
	Continue
};

namespace Detail {

//------------------------------------------------------------------------
enum class MessageType : uint8_t
{
	Utility = 0x0, // 1 word
	System = 0x1, // 1 word
	ChannelVoice1 = 0x2, // 1 word
	SysEx = 0x3, // 2 words
	ChannelVoice2 = 0x4, // 2 words
	Data128 = 0x5, // 4 words
	Unknown6 = 0x6, // 1 word
	Unknown7 = 0x7, // 1 word
	Unknown8 = 0x8, // 2 words
	Unknown9 = 0x9, // 2 words
	UnknownA = 0xa, // 2 words
	UnknownB = 0xb, // 3 words
	UnknownC = 0xc, // 3 words
	UnknownD = 0xd, // 4 words
	UnknownE = 0xe, // 4 words
	UnknownF = 0xf, // 4 words
};

//------------------------------------------------------------------------
struct UMPMessage
{
	uint32_t data;

	SMTG_ALWAYS_INLINE constexpr MessageType type () const
	{
		auto val = (data & 0xf0000000ul) >> 28;
		return static_cast<MessageType> (val);
	}

	SMTG_ALWAYS_INLINE constexpr size_t messageWordCount () const
	{
		auto index = static_cast<size_t> (type ());
		return wordCounts[index];
	}

	SMTG_ALWAYS_INLINE constexpr uint8_t group () const
	{
		return static_cast<uint8_t> (value<uint32_t, 4, 4> (data));
	}

	SMTG_ALWAYS_INLINE constexpr uint8_t byte3_7bits () const
	{
		return static_cast<uint8_t> (value<uint32_t, 17, 7> (data));
	}
	SMTG_ALWAYS_INLINE constexpr uint8_t byte4_7bits () const
	{
		return static_cast<uint8_t> (value<uint32_t, 25, 7> (data));
	}

	SMTG_ALWAYS_INLINE constexpr uint8_t byte1 () const { return value<uint32_t, 0, 8> (data); }
	SMTG_ALWAYS_INLINE constexpr uint8_t byte2 () const { return value<uint32_t, 8, 8> (data); }
	SMTG_ALWAYS_INLINE constexpr uint8_t byte3 () const { return value<uint32_t, 16, 8> (data); }
	SMTG_ALWAYS_INLINE constexpr uint8_t byte4 () const { return value<uint32_t, 24, 8> (data); }

protected:
	static constexpr std::array wordCounts = {1u, 1u, 1u, 2u, 2u, 4u, 1u, 1u,
	                                          2u, 2u, 2u, 3u, 3u, 4u, 4u, 4u};

	template <typename T, size_t pos, size_t bits>
	SMTG_ALWAYS_INLINE constexpr T bitMask () const
	{
		return ((static_cast<T> (1) << bits) - 1) << (((sizeof (T) * 8) - bits) - pos);
	}

	template <typename T, size_t pos, size_t bits>
	SMTG_ALWAYS_INLINE constexpr T value (T data) const
	{
		return (data & bitMask<T, pos, bits> ()) >> (((sizeof (T) * 8) - bits) - pos);
	}
};

//------------------------------------------------------------------------
struct UMPMessage2 : UMPMessage
{
	uint32_t data2;

	SMTG_ALWAYS_INLINE constexpr uint8_t byte5_7bits () const
	{
		return static_cast<uint8_t> (value<uint32_t, 1, 7> (data2));
	}
	SMTG_ALWAYS_INLINE constexpr uint8_t byte6_7bits () const
	{
		return static_cast<uint8_t> (value<uint32_t, 9, 7> (data2));
	}
	SMTG_ALWAYS_INLINE constexpr uint8_t byte7_7bits () const
	{
		return static_cast<uint8_t> (value<uint32_t, 17, 7> (data2));
	}
	SMTG_ALWAYS_INLINE constexpr uint8_t byte8_7bits () const
	{
		return static_cast<uint8_t> (value<uint32_t, 25, 7> (data2));
	}
	SMTG_ALWAYS_INLINE constexpr uint16_t byte5_16bits () const
	{
		return static_cast<uint16_t> (value<uint32_t, 0, 16> (data2));
	}
	SMTG_ALWAYS_INLINE constexpr uint16_t byte7_16bits () const
	{
		return static_cast<uint16_t> (value<uint32_t, 16, 16> (data2));
	}

	SMTG_ALWAYS_INLINE constexpr uint8_t byte5 () const { return value<uint32_t, 0, 8> (data2); }
	SMTG_ALWAYS_INLINE constexpr uint8_t byte6 () const { return value<uint32_t, 8, 8> (data2); }
	SMTG_ALWAYS_INLINE constexpr uint8_t byte7 () const { return value<uint32_t, 16, 8> (data2); }
	SMTG_ALWAYS_INLINE constexpr uint8_t byte8 () const { return value<uint32_t, 24, 8> (data2); }

#if SMTG_OS_MACOS
	// TO REMOVE: We had to add a duplicate this function from UMPMessage
	// for older XCode than 15 to fix a linker issue
	SMTG_ALWAYS_INLINE constexpr uint8_t group () const
	{
		return static_cast<uint8_t> (value<uint32_t, 4, 4> (data));
	}
#endif
};

//------------------------------------------------------------------------
struct UMPMessage4 : UMPMessage2
{
	uint32_t data3;
	uint32_t data4;

	SMTG_ALWAYS_INLINE constexpr uint8_t byte9 () const { return value<uint32_t, 0, 8> (data3); }
	SMTG_ALWAYS_INLINE constexpr uint8_t byte10 () const { return value<uint32_t, 8, 8> (data3); }
	SMTG_ALWAYS_INLINE constexpr uint8_t byte11 () const { return value<uint32_t, 16, 8> (data3); }
	SMTG_ALWAYS_INLINE constexpr uint8_t byte12 () const { return value<uint32_t, 24, 8> (data3); }
	SMTG_ALWAYS_INLINE constexpr uint8_t byte13 () const { return value<uint32_t, 0, 8> (data4); }
	SMTG_ALWAYS_INLINE constexpr uint8_t byte14 () const { return value<uint32_t, 8, 8> (data4); }
	SMTG_ALWAYS_INLINE constexpr uint8_t byte15 () const { return value<uint32_t, 16, 8> (data4); }
	SMTG_ALWAYS_INLINE constexpr uint8_t byte16 () const { return value<uint32_t, 24, 8> (data4); }
};

static_assert (sizeof (UMPMessage) == sizeof (uint32_t), "");
static_assert (sizeof (UMPMessage2) == sizeof (uint32_t) * 2, "");
static_assert (sizeof (UMPMessage4) == sizeof (uint32_t) * 4, "");

//------------------------------------------------------------------------
struct UMPMessageUtility : UMPMessage
{
	enum class Status
	{
		Noop = 0x0,
		JrClock = 0x1,
		JrTimestamp = 0x2,
	};
	SMTG_ALWAYS_INLINE constexpr Status status () const
	{
		return static_cast<Status> (value<uint32_t, 8, 4> (data));
	}
	SMTG_ALWAYS_INLINE constexpr IUniversalMidiPacketHandler::Timestamp timestamp () const
	{
		return static_cast<IUniversalMidiPacketHandler::Timestamp> (value<uint32_t, 16, 16> (data));
	}
};

//------------------------------------------------------------------------
struct UMPMessageSystem : UMPMessage
{
	enum class Status
	{
		Reserved = 0xf0,
		MIDITimeCode = 0xf1,
		SongPositionPointer = 0xf2,
		SongSelect = 0xf3,
		Reserved2 = 0xf4,
		Reserved3 = 0xf5,
		TuneRequest = 0xf6,
		Reserved4 = 0xf7,
		TimingClock = 0xf8,
		Reserved5 = 0xf9,
		Start = 0xfa,
		Continue = 0xfb,
		Stop = 0xfc,
		Reserved6 = 0xfd,
		ActiveSensing = 0xfe,
		Reset = 0xff,
	};
	SMTG_ALWAYS_INLINE constexpr Status status () const
	{
		return static_cast<Status> (value<uint32_t, 8, 4> (data));
	}
};

//------------------------------------------------------------------------
struct UMPMessageChannelVoice1 : UMPMessage
{
	enum class Status
	{
		NoteOff = 0x8,
		NoteOn = 0x9,
		PolyPressure = 0xa,
		ControlChange = 0xb,
		ProgramChange = 0xc,
		ChannelPressure = 0xd,
		PitchBend = 0xe,
	};
	SMTG_ALWAYS_INLINE constexpr Status status () const
	{
		return static_cast<Status> (value<uint32_t, 8, 4> (data));
	}
	SMTG_ALWAYS_INLINE constexpr uint32_t channel () const { return value<uint32_t, 12, 4> (data); }
};

//------------------------------------------------------------------------
struct UMPMessageSysEx : UMPMessage2
{
	enum class Status
	{
		Packet = 0x0,
		Start = 0x1,
		Continue = 0x2,
		End = 0x3,
	};
	SMTG_ALWAYS_INLINE constexpr Status status () const
	{
		return static_cast<Status> (value<uint32_t, 8, 4> (data));
	}
};

//------------------------------------------------------------------------
struct UMPMessageChannelVoice2 : UMPMessage2
{
	enum class Status
	{
		RegisteredPerNoteController = 0x0,
		AssignablePerNoteController = 0x1,
		RegisteredController = 0x2,
		AssignableController = 0x3,
		RelativeRegisteredController = 0x4,
		RelativeAssignableController = 0x5,
		PerNotePitchBend = 0x6,
		NoteOff = 0x8,
		NoteOn = 0x9,
		PolyPressure = 0xa,
		ControlChange = 0xb,
		ProgramChange = 0xc,
		ChannelPressure = 0xd,
		PitchBend = 0xe,
		PerNoteManagement = 0xf
	};

	SMTG_ALWAYS_INLINE constexpr Status status () const
	{
		return static_cast<Status> (value<uint32_t, 8, 4> (data));
	}
	SMTG_ALWAYS_INLINE constexpr uint32_t channel () const { return value<uint32_t, 12, 4> (data); }
};

//------------------------------------------------------------------------
struct UMPMessageData128 : UMPMessage4
{
	enum class Status
	{
		Packet = 0x0,
		Start = 0x1,
		Continue = 0x2,
		End = 0x3,
		MixedHeader = 0x8,
		MixedPayload = 0x9
	};
	SMTG_ALWAYS_INLINE constexpr Status status () const
	{
		return static_cast<Status> (value<uint32_t, 8, 4> (data));
	}
	SMTG_ALWAYS_INLINE constexpr uint8_t numBytes () const
	{
		return static_cast<uint8_t> (value<uint32_t, 12, 4> (data));
	}
	SMTG_ALWAYS_INLINE constexpr uint8_t mdsId () const { return numBytes (); }
};

//------------------------------------------------------------------------
SMTG_ALWAYS_INLINE bool onUtilityMessage (const UMPMessageUtility& msg,
                                          const IUniversalMidiPacketHandler& handler)
{
	auto status = msg.status ();
	using Status = UMPMessageUtility::Status;
	switch (status)
	{
		case Status::Noop:
		{
			handler.onNoop (msg.group ());
			break;
		}
		case Status::JrClock:
		{
			handler.onJitterClock (msg.group (), msg.timestamp ());
			break;
		}
		case Status::JrTimestamp:
		{
			handler.onJitterTimestamp (msg.group (), msg.timestamp ());
			break;
		}
		default: return false;
	}
	return true;
}

//------------------------------------------------------------------------
SMTG_ALWAYS_INLINE bool onSystemMessage (const UMPMessageSystem& msg,
                                         const IUniversalMidiPacketHandler& handler)
{
	auto status = msg.status ();
	using Status = UMPMessageSystem::Status;
	using SystemRealtime = IUniversalMidiPacketHandler::SystemRealtime;
	switch (status)
	{
		case Status::MIDITimeCode:
		{
			handler.onMIDITimeCode (msg.group (), msg.byte3_7bits ());
			break;
		}
		case Status::SongPositionPointer:
		{
			handler.onSongPositionPointer (msg.group (), msg.byte3_7bits (), msg.byte4_7bits ());
			break;
		}
		case Status::SongSelect:
		{
			handler.onSongSelect (msg.group (), msg.byte3_7bits ());
			break;
		}
		case Status::TuneRequest:
		{
			handler.onTuneRequest (msg.group ());
			break;
		}
		case Status::TimingClock:
		{
			handler.onSystemRealtime (msg.group (), SystemRealtime::TimingClock);
			break;
		}
		case Status::Start:
		{
			handler.onSystemRealtime (msg.group (), SystemRealtime::Start);
			break;
		}
		case Status::Continue:
		{
			handler.onSystemRealtime (msg.group (), SystemRealtime::Continue);
			break;
		}
		case Status::Stop:
		{
			handler.onSystemRealtime (msg.group (), SystemRealtime::Stop);
			break;
		}
		case Status::ActiveSensing:
		{
			handler.onSystemRealtime (msg.group (), SystemRealtime::ActiveSensing);
			break;
		}
		case Status::Reset:
		{
			handler.onSystemRealtime (msg.group (), SystemRealtime::Reset);
			break;
		}
		default: return false;
	}
	return true;
}

//------------------------------------------------------------------------
SMTG_ALWAYS_INLINE bool onChannelVoice1Message (const UMPMessageChannelVoice1& msg,
                                                const IUniversalMidiPacketHandler& handler)
{
	using Status = UMPMessageChannelVoice1::Status;
	switch (msg.status ())
	{
		case Status::NoteOff:
		{
			handler.onMidi1NoteOff (msg.group (), msg.channel (), msg.byte3_7bits (),
			                        msg.byte4_7bits ());
			break;
		}
		case Status::NoteOn:
		{
			handler.onMidi1NoteOn (msg.group (), msg.channel (), msg.byte3_7bits (),
			                       msg.byte4_7bits ());
			break;
		}
		case Status::PolyPressure:
		{
			handler.onMidi1PolyPressure (msg.group (), msg.channel (), msg.byte3_7bits (),
			                             msg.byte4_7bits ());
			break;
		}
		case Status::ControlChange:
		{
			handler.onMidi1ControlChange (msg.group (), msg.channel (), msg.byte3_7bits (),
			                              msg.byte4_7bits ());
			break;
		}
		case Status::ProgramChange:
		{
			handler.onMidi1ProgramChange (msg.group (), msg.channel (), msg.byte3_7bits ());
			break;
		}
		case Status::ChannelPressure:
		{
			handler.onMidi1ChannelPressure (msg.group (), msg.channel (), msg.byte3_7bits ());
			break;
		}
		case Status::PitchBend:
		{
			handler.onMidi1PitchBend (msg.group (), msg.channel (), msg.byte3_7bits (),
			                          msg.byte4_7bits ());
			break;
		}
		default: return false;
	}
	return true;
}

//------------------------------------------------------------------------
SMTG_ALWAYS_INLINE bool onSysExMessage (const UMPMessageSysEx& msg,
                                        const IUniversalMidiPacketHandler& handler)
{
	using Status = UMPMessageSysEx::Status;
	switch (msg.status ())
	{
		case Status::Packet:
		{
			handler.onSysExPacket (msg.group (),
			                       {msg.byte3_7bits (), msg.byte4_7bits (), msg.byte5_7bits (),
			                        msg.byte6_7bits (), msg.byte7_7bits (), msg.byte8_7bits ()});
			break;
		}
		case Status::Start:
		{
			handler.onSysExStart (msg.group (),
			                      {msg.byte3_7bits (), msg.byte4_7bits (), msg.byte5_7bits (),
			                       msg.byte6_7bits (), msg.byte7_7bits (), msg.byte8_7bits ()});
			break;
		}
		case Status::Continue:
		{
			handler.onSysExContinue (msg.group (),
			                         {msg.byte3_7bits (), msg.byte4_7bits (), msg.byte5_7bits (),
			                          msg.byte6_7bits (), msg.byte7_7bits (), msg.byte8_7bits ()});
			break;
		}
		case Status::End:
		{
			handler.onSysExEnd (msg.group (),
			                    {msg.byte3_7bits (), msg.byte4_7bits (), msg.byte5_7bits (),
			                     msg.byte6_7bits (), msg.byte7_7bits (), msg.byte8_7bits ()});
			break;
		}
		default: return false;
	}
	return true;
}

//------------------------------------------------------------------------
SMTG_ALWAYS_INLINE bool onChannelVoice2Message (const UMPMessageChannelVoice2& msg,
                                                const IUniversalMidiPacketHandler& handler)
{
	auto group = msg.group ();
	auto status = msg.status ();
	auto channel = msg.channel ();

	using Status = UMPMessageChannelVoice2::Status;
	switch (status)
	{
		case Status::RegisteredPerNoteController:
		{
			handler.onRegisteredPerNoteController (group, channel, msg.byte3_7bits (),
			                                       msg.byte4_7bits (), msg.data2);
			break;
		}
		case Status::AssignablePerNoteController:
		{
			handler.onAssignablePerNoteController (group, channel, msg.byte3_7bits (),
			                                       msg.byte4_7bits (), msg.data2);
			break;
		}
		case Status::RegisteredController:
		{
			handler.onRegisteredController (group, channel, msg.byte3_7bits (), msg.byte4_7bits (),
			                                msg.data2);
			break;
		}
		case Status::AssignableController:
		{
			handler.onAssignableController (group, channel, msg.byte3_7bits (), msg.byte4_7bits (),
			                                msg.data2);
			break;
		}
		case Status::RelativeRegisteredController:
		{
			handler.onRelativeRegisteredController (group, channel, msg.byte3_7bits (),
			                                        msg.byte4_7bits (), msg.data2);
			break;
		}
		case Status::RelativeAssignableController:
		{
			handler.onRelativeAssignableController (group, channel, msg.byte3_7bits (),
			                                        msg.byte4_7bits (), msg.data2);
			break;
		}
		case Status::PerNotePitchBend:
		{
			handler.onPerNotePitchBend (group, channel, msg.byte3_7bits (), msg.data2);
			break;
		}
		case Status::NoteOn:
		{
			handler.onNoteOn (group, channel, msg.byte3_7bits (), msg.byte5_16bits (),
			                  msg.byte4_7bits (), msg.byte7_16bits ());
			break;
		}
		case Status::NoteOff:
		{
			handler.onNoteOff (group, channel, msg.byte3_7bits (), msg.byte5_16bits (),
			                   msg.byte4_7bits (), msg.byte7_16bits ());
			break;
		}
		case Status::PolyPressure:
		{
			handler.onPolyPressure (group, channel, msg.byte3_7bits (), msg.data2);
			break;
		}
		case Status::ControlChange:
		{
			handler.onControlChange (group, channel, msg.byte3_7bits (), msg.data2);
			break;
		}
		case Status::ProgramChange:
		{
			handler.onProgramChange (group, channel, msg.byte4_7bits (), msg.byte5_7bits (),
			                         msg.byte7_7bits (), msg.byte8_7bits ());
			break;
		}
		case Status::ChannelPressure:
		{
			handler.onChannelPressure (group, channel, msg.data2);
			break;
		}
		case Status::PitchBend:
		{
			handler.onPitchBend (group, channel, msg.data2);
			break;
		}
		case Status::PerNoteManagement:
		{
			handler.onPerNoteManagement (group, channel, msg.byte3_7bits (), msg.byte4_7bits ());
			break;
		}
		default: return false;
	}
	return true;
}

//------------------------------------------------------------------------
SMTG_ALWAYS_INLINE bool onData128Message (const UMPMessageData128& msg,
                                          const IUniversalMidiPacketHandler& handler)
{
	using Status = UMPMessageData128::Status;
	switch (msg.status ())
	{
		case Status::Packet:
		{
			handler.onSysEx8Packet (msg.group (), msg.numBytes (), msg.byte3 (),
			                        {msg.byte4 (), msg.byte5 (), msg.byte6 (), msg.byte7 (),
			                         msg.byte8 (), msg.byte9 (), msg.byte10 (), msg.byte12 (),
			                         msg.byte13 (), msg.byte14 (), msg.byte15 (), msg.byte16 ()});
			break;
		}
		case Status::Start:
		{
			handler.onSysEx8Start (msg.group (), msg.numBytes (), msg.byte3 (),
			                       {msg.byte4 (), msg.byte5 (), msg.byte6 (), msg.byte7 (),
			                        msg.byte8 (), msg.byte9 (), msg.byte10 (), msg.byte12 (),
			                        msg.byte13 (), msg.byte14 (), msg.byte15 (), msg.byte16 ()});
			break;
		}
		case Status::Continue:
		{
			handler.onSysEx8Continue (msg.group (), msg.numBytes (), msg.byte3 (),
			                          {msg.byte4 (), msg.byte5 (), msg.byte6 (), msg.byte7 (),
			                           msg.byte8 (), msg.byte9 (), msg.byte10 (), msg.byte12 (),
			                           msg.byte13 (), msg.byte14 (), msg.byte15 (), msg.byte16 ()});
			break;
		}
		case Status::End:
		{
			handler.onSysEx8End (msg.group (), msg.numBytes (), msg.byte3 (),
			                     {msg.byte4 (), msg.byte5 (), msg.byte6 (), msg.byte7 (),
			                      msg.byte8 (), msg.byte9 (), msg.byte10 (), msg.byte12 (),
			                      msg.byte13 (), msg.byte14 (), msg.byte15 (), msg.byte16 ()});
			break;
		}
		case Status::MixedHeader:
		{
			handler.onMixedDataSetHeader (msg.group (), msg.mdsId (),
			                              {msg.byte3 (), msg.byte4 (), msg.byte5 (), msg.byte6 (),
			                               msg.byte7 (), msg.byte8 (), msg.byte9 (), msg.byte10 (),
			                               msg.byte12 (), msg.byte13 (), msg.byte14 (),
			                               msg.byte15 (), msg.byte16 ()});
			break;
		}
		case Status::MixedPayload:
		{
			handler.onMixedDataSetPayload (msg.group (), msg.mdsId (),
			                               {msg.byte3 (), msg.byte4 (), msg.byte5 (), msg.byte6 (),
			                                msg.byte7 (), msg.byte8 (), msg.byte9 (), msg.byte10 (),
			                                msg.byte12 (), msg.byte13 (), msg.byte14 (),
			                                msg.byte15 (), msg.byte16 ()});
			break;
		}
		default: return false;
	}
	return true;
}

//------------------------------------------------------------------------
template <uint32_t Sections>
SMTG_ALWAYS_INLINE size_t parse (const size_t numWords, const uint32_t* words,
                                 const IUniversalMidiPacketHandler& handler)
{
	auto msg = reinterpret_cast<const UMPMessage*> (words);
	for (size_t index = 0; index < numWords;)
	{
		auto numMsgWords = msg->messageWordCount ();
		if (index + numMsgWords > numWords)
		{
			handler.onInsufficentInputData (index, (index + numMsgWords) - numWords);
			return index;
		}
		switch (msg->type ())
		{
			case MessageType::Utility:
			{
				if
					constexpr ((Sections & ParseSections::Utility) != 0)
					{
						auto utilityMsg = reinterpret_cast<const UMPMessageUtility*> (msg);
						if (!onUtilityMessage (*utilityMsg, handler))
						{
							if (handler.onInvalidInputData (index) == ParsingAction::Break)
								return index;
						}
					}
				break;
			}
			case MessageType::System:
			{
				if
					constexpr ((Sections & ParseSections::System) != 0)
					{
						auto systemMsg = reinterpret_cast<const UMPMessageSystem*> (msg);
						if (!onSystemMessage (*systemMsg, handler))
						{
							if (handler.onInvalidInputData (index) == ParsingAction::Break)
								return index;
						}
					}
				break;
			}
			case MessageType::ChannelVoice1:
			{
				if
					constexpr ((Sections & ParseSections::ChannelVoice1) != 0)
					{
						auto channelMsg = reinterpret_cast<const UMPMessageChannelVoice1*> (msg);
						if (!onChannelVoice1Message (*channelMsg, handler))
						{
							if (handler.onInvalidInputData (index) == ParsingAction::Break)
								return index;
						}
					}
				break;
			}
			case MessageType::SysEx:
			{
				if
					constexpr ((Sections & ParseSections::SysEx) != 0)
					{
						assert (index < numWords - 1);
						auto sysExMsg = reinterpret_cast<const UMPMessageSysEx*> (msg);
						if (!onSysExMessage (*sysExMsg, handler))
						{
							if (handler.onInvalidInputData (index) == ParsingAction::Break)
								return index;
						}
					}
				break;
			}
			case MessageType::ChannelVoice2:
			{
				if
					constexpr ((Sections & ParseSections::ChannelVoice2) != 0)
					{
						assert (index < numWords - 1);
						auto channelMsg = reinterpret_cast<const UMPMessageChannelVoice2*> (msg);
						if (!onChannelVoice2Message (*channelMsg, handler))
						{
							if (handler.onInvalidInputData (index) == ParsingAction::Break)
								return index;
						}
					}
				break;
			}
			case MessageType::Data128:
			{
				if
					constexpr ((Sections & ParseSections::Data128) != 0)
					{
						assert (index < numWords - 3);
						auto dataMsg = reinterpret_cast<const UMPMessageData128*> (msg);
						if (!onData128Message (*dataMsg, handler))
						{
							if (handler.onInvalidInputData (index) == ParsingAction::Break)
								return index;
						}
					}
				break;
			}
		}
		index += numMsgWords;
		msg += numMsgWords;
	}
	return numWords;
}

//------------------------------------------------------------------------
} // Detail

//------------------------------------------------------------------------
template <ParseSections Sections>
SMTG_ALWAYS_INLINE size_t parsePackets (const size_t numWords, const uint32_t* words,
                                        const IUniversalMidiPacketHandler& handler)
{
	return Detail::parse<Sections> (numWords, words, handler);
}

//------------------------------------------------------------------------
struct UniversalMidiPacketHandlerAdapter : IUniversalMidiPacketHandler
{
	void onNoop (Group group) const override {}
	void onJitterClock (Group group, Timestamp time) const override {}
	void onJitterTimestamp (Group group, Timestamp time) const override {}
	void onMIDITimeCode (Group group, Timecode timecode) const override {}
	void onSongPositionPointer (Group group, uint8_t posLSB, uint8_t posMSB) const override {}
	void onSongSelect (Group group, uint8_t songIndex) const override {}
	void onTuneRequest (Group group) const override {}
	void onSystemRealtime (Group group, SystemRealtime which) const override {}
	void onMidi1NoteOff (Group group, Channel channel, NoteNumber note,
	                     Velocity8 velocity) const override
	{
	}
	void onMidi1NoteOn (Group group, Channel channel, NoteNumber note,
	                    Velocity8 velocity) const override
	{
	}
	void onMidi1PolyPressure (Group group, Channel channel, NoteNumber note,
	                          Data8 data) const override
	{
	}
	void onMidi1ControlChange (Group group, Channel channel, ControllerNumber controller,
	                           Data8 value) const override
	{
	}
	void onMidi1ProgramChange (Group group, Channel channel, Program program) const override {}
	void onMidi1ChannelPressure (Group group, Channel channel, Data8 pressure) const override {}
	void onMidi1PitchBend (Group group, Channel channel, Data8 valueLSB,
	                       Data8 valueMSB) const override
	{
	}
	void onSysExPacket (Group group, SysEx6ByteData data) const override {}
	void onSysExStart (Group group, SysEx6ByteData data) const override {}
	void onSysExContinue (Group group, SysEx6ByteData data) const override {}
	void onSysExEnd (Group group, SysEx6ByteData data) const override {}
	void onRegisteredPerNoteController (Group group, Channel channel, NoteNumber note,
	                                    ControllerNumber controller, Data32 data) const override
	{
	}
	void onAssignablePerNoteController (Group group, Channel channel, NoteNumber note,
	                                    ControllerNumber controller, Data32 data) const override
	{
	}
	void onRegisteredController (Group group, Channel channel, BankNumber bank, Index index,
	                             Data32 data) const override
	{
	}
	void onAssignableController (Group group, Channel channel, BankNumber bank, Index index,
	                             Data32 data) const override
	{
	}
	void onRelativeRegisteredController (Group group, Channel channel, BankNumber bank, Index index,
	                                     Data32 data) const override
	{
	}
	void onRelativeAssignableController (Group group, Channel channel, BankNumber bank, Index index,
	                                     Data32 data) const override
	{
	}
	void onPerNotePitchBend (Group group, Channel channel, NoteNumber note,
	                         Data32 data) const override
	{
	}
	void onNoteOff (Group group, Channel channel, NoteNumber note, Velocity16 velocity,
	                AttributeType attr, AttributeValue attrValue) const override
	{
	}
	void onNoteOn (Group group, Channel channel, NoteNumber note, Velocity16 velocity,
	               AttributeType attr, AttributeValue attrValue) const override
	{
	}
	void onPolyPressure (Group group, Channel channel, NoteNumber note, Data32 data) const override
	{
	}
	void onControlChange (Group group, Channel channel, ControllerNumber controller,
	                      Data32 data) const override
	{
	}
	void onProgramChange (Group group, Channel channel, OptionFlags options, Program program,
	                      BankMSB bankMSB, BankLSB bankLSB) const override
	{
	}
	void onChannelPressure (Group group, Channel channel, Data32 data) const override {}
	void onPitchBend (Group group, Channel channel, Data32 data) const override {}
	void onPerNoteManagement (Group group, Channel channel, NoteNumber note,
	                          OptionFlags options) const override
	{
	}
	void onSysEx8Packet (Group group, Data8 numBytes, Index streamID,
	                     SysEx13ByteData data) const override
	{
	}
	void onSysEx8Start (Group group, Data8 numBytes, Index streamID,
	                    SysEx13ByteData data) const override
	{
	}
	void onSysEx8Continue (Group group, Data8 numBytes, Index streamID,
	                       SysEx13ByteData data) const override
	{
	}
	void onSysEx8End (Group group, Data8 numBytes, Index streamID,
	                  SysEx13ByteData data) const override
	{
	}
	void onMixedDataSetHeader (Group group, Index mdsID, MixedData data) const override {}
	void onMixedDataSetPayload (Group group, Index mdsID, MixedData data) const override {}
	ParsingAction onInvalidInputData (size_t index) const override
	{
		return ParsingAction::Continue;
	}
	void onInsufficentInputData (size_t index, size_t numMissingWords) const override {}
};

//------------------------------------------------------------------------
} // Steinberg::Vst::EditorHost
