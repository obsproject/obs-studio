/*
 * aeffectx.h - simple header to allow VeSTige compilation and eventually work
 *
 * Copyright (c) 2006 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
 *
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#include <stdint.h>

#ifndef _AEFFECTX_H
#define _AEFFECTX_H

#define CCONST(a, b, c, d)( ( ( (int) a ) << 24 ) |      \
		( ( (int) b ) << 16 ) |    \
		( ( (int) c ) << 8 ) |     \
		( ( (int) d ) << 0 ) )


const int audioMasterAutomate = 0;
const int audioMasterVersion = 1;
const int audioMasterCurrentId = 2;
const int audioMasterIdle = 3;
const int audioMasterPinConnected = 4;
// unsupported? 5
const int audioMasterWantMidi = 6;
const int audioMasterGetTime = 7;
const int audioMasterProcessEvents = 8;
const int audioMasterSetTime = 9;
const int audioMasterTempoAt = 10;
const int audioMasterGetNumAutomatableParameters = 11;
const int audioMasterGetParameterQuantization = 12;
const int audioMasterIOChanged = 13;
const int audioMasterNeedIdle = 14;
const int audioMasterSizeWindow = 15;
const int audioMasterGetSampleRate = 16;
const int audioMasterGetBlockSize = 17;
const int audioMasterGetInputLatency = 18;
const int audioMasterGetOutputLatency = 19;
const int audioMasterGetPreviousPlug = 20;
const int audioMasterGetNextPlug = 21;
const int audioMasterWillReplaceOrAccumulate = 22;
const int audioMasterGetCurrentProcessLevel = 23;
const int audioMasterGetAutomationState = 24;
const int audioMasterOfflineStart = 25;
const int audioMasterOfflineRead = 26;
const int audioMasterOfflineWrite = 27;
const int audioMasterOfflineGetCurrentPass = 28;
const int audioMasterOfflineGetCurrentMetaPass = 29;
const int audioMasterSetOutputSampleRate = 30;
// unsupported? 31
const int audioMasterGetSpeakerArrangement = 31; // deprecated in 2.4?
const int audioMasterGetVendorString = 32;
const int audioMasterGetProductString = 33;
const int audioMasterGetVendorVersion = 34;
const int audioMasterVendorSpecific = 35;
const int audioMasterSetIcon = 36;
const int audioMasterCanDo = 37;
const int audioMasterGetLanguage = 38;
const int audioMasterOpenWindow = 39;
const int audioMasterCloseWindow = 40;
const int audioMasterGetDirectory = 41;
const int audioMasterUpdateDisplay = 42;
const int audioMasterBeginEdit = 43;
const int audioMasterEndEdit = 44;
const int audioMasterOpenFileSelector = 45;
const int audioMasterCloseFileSelector = 46; // currently unused
const int audioMasterEditFile = 47; // currently unused
const int audioMasterGetChunkFile = 48; // currently unused
const int audioMasterGetInputSpeakerArrangement = 49; // currently unused

const int effFlagsHasEditor = 1;
const int effFlagsCanReplacing = 1 << 4; // very likely
const int effFlagsProgramChunks = 1 << 5; // from Ardour
const int effFlagsIsSynth = 1 << 8; // currently unused

const int effOpen = 0;
const int effClose = 1; // currently unused
const int effSetProgram = 2; // currently unused
const int effGetProgram = 3; // currently unused
// The next one was gleaned from http://www.kvraudio.com/forum/viewtopic.php?p=1905347
const int effSetProgramName = 4;
const int effGetProgramName = 5; // currently unused
// The next two were gleaned from http://www.kvraudio.com/forum/viewtopic.php?p=1905347
const int effGetParamLabel = 6;
const int effGetParamDisplay = 7;
const int effGetParamName = 8; // currently unused
const int effSetSampleRate = 10;
const int effSetBlockSize = 11;
const int effMainsChanged = 12;
const int effEditGetRect = 13;
const int effEditOpen = 14;
const int effEditClose = 15;
const int effEditIdle = 19;
const int effEditTop = 20;
const int effIdentify = 22; // from http://www.asseca.org/vst-24-specs/efIdentify.html
const int effGetChunk = 23; // from Ardour
const int effSetChunk = 24; // from Ardour
const int effProcessEvents = 25;
// The next one was gleaned from http://www.asseca.org/vst-24-specs/efCanBeAutomated.html
const int effCanBeAutomated = 26;
// The next one was gleaned from http://www.kvraudio.com/forum/viewtopic.php?p=1905347
const int effGetProgramNameIndexed = 29;
// The next one was gleaned from http://www.asseca.org/vst-24-specs/efGetPlugCategory.html
const int effGetPlugCategory = 35;
const int effGetEffectName = 45;
const int effGetParameterProperties = 56; // missing
const int effGetVendorString = 47;
const int effGetProductString = 48;
const int effGetVendorVersion = 49;
const int effCanDo = 51; // currently unused
// The next one was gleaned from http://www.asseca.org/vst-24-specs/efIdle.html
const int effIdle = 53;
const int effGetVstVersion = 58; // currently unused
// The next one was gleaned from http://www.asseca.org/vst-24-specs/efBeginSetProgram.html
const int effBeginSetProgram = 67;
// The next one was gleaned from http://www.asseca.org/vst-24-specs/efEndSetProgram.html
const int effEndSetProgram = 68;
// The next one was gleaned from http://www.asseca.org/vst-24-specs/efShellGetNextPlugin.html
const int effShellGetNextPlugin = 70;
// The next one was gleaned from http://www.asseca.org/vst-24-specs/efBeginLoadBank.html
const int effBeginLoadBank = 75;
// The next one was gleaned from http://www.asseca.org/vst-24-specs/efBeginLoadProgram.html
const int  effBeginLoadProgram = 76;

// The next two were gleaned from http://www.kvraudio.com/forum/printview.php?t=143587&start=0
const int effStartProcess = 71;
const int effStopProcess = 72;

const int kEffectMagic = CCONST( 'V', 's', 't', 'P' );
const int kVstLangEnglish = 1;
const int kVstMidiType = 1;

const int kVstNanosValid = 1 << 8;
const int kVstPpqPosValid = 1 << 9;
const int kVstTempoValid = 1 << 10;
const int kVstBarsValid = 1 << 11;
const int kVstCyclePosValid = 1 << 12;
const int kVstTimeSigValid = 1 << 13;
// from Ardour
const int kVstSmpteValid = 1 << 14;
// from Ardour
const int kVstClockValid = 1 << 15;

const int kVstTransportPlaying = 1 << 1;
const int kVstTransportCycleActive = 1 << 2;
const int kVstTransportChanged = 1;


class RemoteVstPlugin;


class VstMidiEvent
{
	public:
		// 00
		int type;
		// 04
		int byteSize;
		// 08
		int deltaFrames;
		// 0c?
		int flags;
		// 10?
		int noteLength;
		// 14?
		int noteOffset;
		// 18
		char midiData[4];
		// 1c?
		char detune;
		// 1d?
		char noteOffVelocity;
		// 1e?
		char reserved1;
		// 1f?
		char reserved2;
};


class VstEvent
{
	char dump[sizeof( VstMidiEvent )];
};


class VstEvents
{
	public:
		// 00
		int numEvents;
		// 04
		void *reserved;
		// 08
		VstEvent* events[1];
};


// Not finished, neither really used
class VstParameterProperties
{
	public:
	/*
		float stepFloat;
		char label[64];
		int flags;
		int minInteger;
		int maxInteger;
		int stepInteger;
		char shortLabel[8];
		int category;
		char categoryLabel[24];
		char empty[128];
	*/

		float stepFloat;
		float smallStepFloat;
		float largeStepFloat;
		char label[64];
		unsigned int flags;
		unsigned int minInteger;
		unsigned int maxInteger;
		unsigned int stepInteger;
		unsigned int largeStepInteger;
		char shortLabel[8];
		unsigned short displayIndex;
		unsigned short category;
		unsigned short numParametersInCategory;
		unsigned short reserved;
		char categoryLabel[24];
		char future[16];
};


class AEffect
{
	public:
		// Never use virtual functions!!!
		// 00-03
		int magic;
		// dispatcher 04-07
		intptr_t (* dispatcher)( AEffect * , int , int , intptr_t, void * , float );
		// process, quite sure 08-0b
		void (* process)( AEffect * , float * * , float * * , int );
		// setParameter 0c-0f
		void (* setParameter)( AEffect * , int , float );
		// getParameter 10-13
		float (* getParameter)( AEffect * , int );
		// programs 14-17
		int numPrograms;
		// Params 18-1b
		int numParams;
		// Input 1c-1f
		int numInputs;
		// Output 20-23
		int numOutputs;
		// flags 24-27
		int flags;
		// Fill somewhere 28-2b
		void * ptr1;
		void * ptr2;
		int initialDelay;
		// Zeroes 34-37 38-3b
		int empty3a;
		int empty3b;
		// 1.0f 3c-3f
		float unkown_float;
		// An object? pointer 40-43
		void *ptr3;
		// Zeroes 44-47
		void *user;
		// Id 48-4b
		int32_t uniqueID;
		int32_t version;
		// processReplacing 50-53
		void (* processReplacing)( AEffect * , float * * , float * * , int );
};

typedef intptr_t (* audioMasterCallback)( AEffect * , int32_t, int32_t,
		intptr_t, void * , float );

class VstTimeInfo
{
	public:
	// 00
	double samplePos;
	// 08
	double sampleRate;
	// 10
	double nanoSeconds;
	// 18
	double ppqPos;
	// 20?
	double tempo;
	// 28
	double barStartPos;
	// 30?
	double cycleStartPos;
	// 38?
	double cycleEndPos;
	// 40?
	int timeSigNumerator;
	// 44?
	int timeSigDenominator;
	// unconfirmed 48 4c 50
	char empty3[4 + 4 + 4];
	// 54
	int flags;
};

// from http://www.asseca.org/vst-24-specs/efGetParameterProperties.html
enum VstParameterFlags
{
	// parameter is a switch (on/off)
	kVstParameterIsSwitch                = 1 << 0,

	// minInteger, maxInteger valid
	kVstParameterUsesIntegerMinMax       = 1 << 1,

	// stepFloat, smallStepFloat, largeStepFloat valid
	kVstParameterUsesFloatStep           = 1 << 2,

	// stepInteger, largeStepInteger valid
	kVstParameterUsesIntStep             = 1 << 3,

	// displayIndex valid
	kVstParameterSupportsDisplayIndex    = 1 << 4,

	// category, etc. valid
	kVstParameterSupportsDisplayCategory = 1 << 5,

	// set if parameter value can ramp up/down
	kVstParameterCanRamp                 = 1 << 6
};

// from http://www.asseca.org/vst-24-specs/efBeginLoadProgram.html
struct VstPatchChunkInfo
{
	int32_t version;           // Format Version (should be 1)
	int32_t pluginUniqueID;    // UniqueID of the plug-in
	int32_t pluginVersion;     // Plug-in Version
	int32_t numElements;       // Number of Programs (Bank) or
				   // Parameters (Program)
	char future[48];           // Reserved for future use
};

// from http://www.asseca.org/vst-24-specs/efGetPlugCategory.html
enum VstPlugCategory
{
	kPlugCategUnknown = 0,    // 0=Unknown, category not implemented
	kPlugCategEffect,         // 1=Simple Effect
	kPlugCategSynth,          // 2=VST Instrument (Synths, samplers,...)
	kPlugCategAnalysis,       // 3=Scope, Tuner, ...
	kPlugCategMastering,      // 4=Dynamics, ...
	kPlugCategSpacializer,    // 5=Panners, ...
	kPlugCategRoomFx,         // 6=Delays and Reverbs
	kPlugSurroundFx,          // 7=Dedicated surround processor
	kPlugCategRestoration,    // 8=Denoiser, ...
	kPlugCategOfflineProcess, // 9=Offline Process
	kPlugCategShell,          // 10=Plug-in is container of other
				  // plug-ins @see effShellGetNextPlugin()
	kPlugCategGenerator,      // 11=ToneGenerator, ...
	kPlugCategMaxCount        // 12=Marker to count the categories
};

#endif