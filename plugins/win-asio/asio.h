/* clang-format off */
//------------------------------------------------------------------------
// Project     : ASIO SDK
//
// Category    : Interfaces
// Filename    : common/asio.h
// Created by  : Steinberg, 05/1996
// Description : Steinberg Audio Stream I/O API v2.3
// 	ASIO functions entries which translate the ASIO interface to the
//  asiodrvr class methods
// 	2005 - Added support for DSD sample data (in cooperation with Sony)
// 	2012 - Added support for drop out detection
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

/*
	ASIO Interface Specification v 2.3

	basic concept is an i/o synchronous double-buffer scheme:

	on bufferSwitch(index == 0), host will read/write:

		after ASIOStart(), the
  read  first input buffer A (index 0)
	|   will be invalid (empty)
	*   ------------------------
	|------------------------|-----------------------|
	|                        |                       |
	|  Input Buffer A (0)    |   Input Buffer B (1)  |
	|                        |                       |
	|------------------------|-----------------------|
	|                        |                       |
	|  Output Buffer A (0)   |   Output Buffer B (1) |
	|                        |                       |
	|------------------------|-----------------------|
	*                        -------------------------
	|                        before calling ASIOStart(),
  write                      host will have filled output
                             buffer B (index 1) already

  *please* take special care of proper statement of input
  and output latencies (see ASIOGetLatencies()), these
  control sequencer sync accuracy

*/

//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------

/*
prototypes summary:

ASIOError ASIOInit(ASIODriverInfo *info);
ASIOError ASIOExit(void);
ASIOError ASIOStart(void);
ASIOError ASIOStop(void);
ASIOError ASIOGetChannels(long *numInputChannels, long *numOutputChannels);
ASIOError ASIOGetLatencies(long *inputLatency, long *outputLatency);
ASIOError ASIOGetBufferSize(long *minSize, long *maxSize, long *preferredSize, long *granularity);
ASIOError ASIOCanSampleRate(ASIOSampleRate sampleRate);
ASIOError ASIOGetSampleRate(ASIOSampleRate *currentRate);
ASIOError ASIOSetSampleRate(ASIOSampleRate sampleRate);
ASIOError ASIOGetClockSources(ASIOClockSource *clocks, long *numSources);
ASIOError ASIOSetClockSource(long reference);
ASIOError ASIOGetSamplePosition (ASIOSamples *sPos, ASIOTimeStamp *tStamp);
ASIOError ASIOGetChannelInfo(ASIOChannelInfo *info);
ASIOError ASIOCreateBuffers(ASIOBufferInfo *bufferInfos, long numChannels,
	long bufferSize, ASIOCallbacks *callbacks);
ASIOError ASIODisposeBuffers(void);
ASIOError ASIOControlPanel(void);
void *ASIOFuture(long selector, void *params);
ASIOError ASIOOutputReady(void);

*/

//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------

#ifndef __ASIO_H
#define __ASIO_H

// force 4 byte alignment
#if defined(_MSC_VER) && !defined(__MWERKS__)
#pragma pack(push, 4)
#elif PRAGMA_ALIGN_SUPPORTED
#pragma options align = native
#endif

//- - - - - - - - - - - - - - - - - - - - - - - - -
// Type definitions
//- - - - - - - - - - - - - - - - - - - - - - - - -

// number of samples data type is 64 bit integer
#if NATIVE_INT64
typedef long long int ASIOSamples;
#else
typedef struct ASIOSamples {
	unsigned long hi;
	unsigned long lo;
} ASIOSamples;
#endif

// Timestamp data type is 64 bit integer,
// Time format is Nanoseconds.
#if NATIVE_INT64
typedef long long int ASIOTimeStamp;
#else
typedef struct ASIOTimeStamp {
	unsigned long hi;
	unsigned long lo;
} ASIOTimeStamp;
#endif

// Samplerates are expressed in IEEE 754 64 bit double float,
// native format as host computer
#if IEEE754_64FLOAT
typedef double ASIOSampleRate;
#else
typedef struct ASIOSampleRate {
	char ieee[8];
} ASIOSampleRate;
#endif

// Boolean values are expressed as long
typedef long ASIOBool;
enum { ASIOFalse = 0, ASIOTrue = 1 };

// Sample Types are expressed as long
typedef long ASIOSampleType;
enum {
	ASIOSTInt16MSB = 0,
	ASIOSTInt24MSB = 1, // used for 20 bits as well
	ASIOSTInt32MSB = 2,
	ASIOSTFloat32MSB = 3, // IEEE 754 32 bit float
	ASIOSTFloat64MSB = 4, // IEEE 754 64 bit double float

	// these are used for 32 bit data buffer, with different alignment of the data inside
	// 32 bit PCI bus systems can be more easily used with these
	ASIOSTInt32MSB16 = 8,  // 32 bit data with 16 bit alignment
	ASIOSTInt32MSB18 = 9,  // 32 bit data with 18 bit alignment
	ASIOSTInt32MSB20 = 10, // 32 bit data with 20 bit alignment
	ASIOSTInt32MSB24 = 11, // 32 bit data with 24 bit alignment

	ASIOSTInt16LSB = 16,
	ASIOSTInt24LSB = 17, // used for 20 bits as well
	ASIOSTInt32LSB = 18,
	ASIOSTFloat32LSB = 19, // IEEE 754 32 bit float, as found on Intel x86 architecture
	ASIOSTFloat64LSB = 20, // IEEE 754 64 bit double float, as found on Intel x86 architecture

	// these are used for 32 bit data buffer, with different alignment of the data inside
	// 32 bit PCI bus systems can more easily used with these
	ASIOSTInt32LSB16 = 24, // 32 bit data with 18 bit alignment
	ASIOSTInt32LSB18 = 25, // 32 bit data with 18 bit alignment
	ASIOSTInt32LSB20 = 26, // 32 bit data with 20 bit alignment
	ASIOSTInt32LSB24 = 27, // 32 bit data with 24 bit alignment

	//	ASIO DSD format.
	ASIOSTDSDInt8LSB1 = 32, // DSD 1 bit data, 8 samples per byte. First sample in Least significant bit.
	ASIOSTDSDInt8MSB1 = 33, // DSD 1 bit data, 8 samples per byte. First sample in Most significant bit.
	ASIOSTDSDInt8NER8 = 40, // DSD 8 bit data, 1 sample per byte. No Endianness required.

	ASIOSTLastEntry
};

/*-----------------------------------------------------------------------------
// DSD operation and buffer layout
// Definition by Steinberg/Sony Oxford.
//
// We have tried to treat DSD as PCM and so keep a consistant structure across
// the ASIO interface.
//
// DSD's sample rate is normally referenced as a multiple of 44.1Khz, so
// the standard sample rate is refered to as 64Fs (or 2.8224Mhz). We looked
// at making a special case for DSD and adding a field to the ASIOFuture that
// would allow the user to select the Over Sampleing Rate (OSR) as a seperate
// entity but decided in the end just to treat it as a simple value of
// 2.8224Mhz and use the standard interface to set it.
//
// The second problem was the "word" size, in PCM the word size is always a
// greater than or equal to 8 bits (a byte). This makes life easy as we can
// then pack the samples into the "natural" size for the machine.
// In DSD the "word" size is 1 bit. This is not a major problem and can easily
// be dealt with if we ensure that we always deal with a multiple of 8 samples.
//
// DSD brings with it another twist to the Endianness religion. How are the
// samples packed into the byte. It would be nice to just say the most significant
// bit is always the first sample, however there would then be a performance hit
// on little endian machines. Looking at how some of the processing goes...
// Little endian machines like the first sample to be in the Least Significant Bit,
//   this is because when you write it to memory the data is in the correct format
//   to be shifted in and out of the words.
// Big endian machine prefer the first sample to be in the Most Significant Bit,
//   again for the same reasion.
//
// And just when things were looking really muddy there is a proposed extension to
// DSD that uses 8 bit word sizes. It does not care what endianness you use.
//
// Switching the driver between DSD and PCM mode
// ASIOFuture allows for extending the ASIO API quite transparently.
// See kAsioSetIoFormat, kAsioGetIoFormat, kAsioCanDoIoFormat
//
//-----------------------------------------------------------------------------*/

//- - - - - - - - - - - - - - - - - - - - - - - - -
// Error codes
//- - - - - - - - - - - - - - - - - - - - - - - - -

typedef long ASIOError;
enum {
	ASE_OK = 0,               // This value will be returned whenever the call succeeded
	ASE_SUCCESS = 0x3f4847a0, // unique success return value for ASIOFuture calls
	ASE_NotPresent = -1000,   // hardware input or output is not present or available
	ASE_HWMalfunction,        // hardware is malfunctioning (can be returned by any ASIO function)
	ASE_InvalidParameter,     // input parameter invalid
	ASE_InvalidMode,          // hardware is in a bad mode or used in a bad mode
	ASE_SPNotAdvancing,       // hardware is not running when sample position is inquired
	ASE_NoClock,              // sample clock or rate cannot be determined or is not present
	ASE_NoMemory              // not enough memory for completing the request
};

//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------

//- - - - - - - - - - - - - - - - - - - - - - - - -
// Time Info support
//- - - - - - - - - - - - - - - - - - - - - - - - -

typedef struct ASIOTimeCode {
	double speed;                // speed relation (fraction of nominal speed)
				     // optional; set to 0. or 1. if not supported
	ASIOSamples timeCodeSamples; // time in samples
	unsigned long flags;         // some information flags (see below)
	char future[64];
} ASIOTimeCode;

typedef enum ASIOTimeCodeFlags {
	kTcValid = 1,
	kTcRunning = 1 << 1,
	kTcReverse = 1 << 2,
	kTcOnspeed = 1 << 3,
	kTcStill = 1 << 4,

	kTcSpeedValid = 1 << 8
} ASIOTimeCodeFlags;

typedef struct AsioTimeInfo {
	double speed;             // absolute speed (1. = nominal)
	ASIOTimeStamp systemTime; // system time related to samplePosition, in nanoseconds
				  // on mac, must be derived from Microseconds() (not UpTime()!)
				  // on windows, must be derived from timeGetTime()
	ASIOSamples samplePosition;
	ASIOSampleRate sampleRate; // current rate
	unsigned long flags;       // (see below)
	char reserved[12];
} AsioTimeInfo;

typedef enum AsioTimeInfoFlags {
	kSystemTimeValid = 1,          // must always be valid
	kSamplePositionValid = 1 << 1, // must always be valid
	kSampleRateValid = 1 << 2,
	kSpeedValid = 1 << 3,

	kSampleRateChanged = 1 << 4,
	kClockSourceChanged = 1 << 5
} AsioTimeInfoFlags;

typedef struct ASIOTime // both input/output
{
	long reserved[4];             // must be 0
	struct AsioTimeInfo timeInfo; // required
	struct ASIOTimeCode timeCode; // optional, evaluated if (timeCode.flags & kTcValid)
} ASIOTime;

/*

using time info:
it is recommended to use the new method with time info even if the asio
device does not support timecode; continuous calls to ASIOGetSamplePosition
and ASIOGetSampleRate are avoided, and there is a more defined relationship
between callback time and the time info.

see the example below.
to initiate time info mode, after you have received the callbacks pointer in
ASIOCreateBuffers, you will call the asioMessage callback with kAsioSupportsTimeInfo
as the argument. if this returns 1, host has accepted time info mode.
now host expects the new callback bufferSwitchTimeInfo to be used instead
of the old bufferSwitch method. the ASIOTime structure is assumed to be valid
and accessible until the callback returns.

using time code:
if the device supports reading time code, it will call host's asioMessage callback
with kAsioSupportsTimeCode as the selector. it may then fill the according
fields and set the kTcValid flag.
host will call the future method with the kAsioEnableTimeCodeRead selector when
it wants to enable or disable tc reading by the device. you should also support
the kAsioCanTimeInfo and kAsioCanTimeCode selectors in ASIOFuture (see example).

note:
the AsioTimeInfo/ASIOTimeCode pair is supposed to work in both directions.
as a matter of convention, the relationship between the sample
position counter and the time code at buffer switch time is
(ignoring offset between tc and sample pos when tc is running):

on input:	sample 0 -> input  buffer sample 0 -> time code 0
on output:	sample 0 -> output buffer sample 0 -> time code 0

this means that for 'real' calculations, one has to take into account
the according latencies.

example:

ASIOTime asioTime;

in createBuffers()
{
	memset(&asioTime, 0, sizeof(ASIOTime));
	AsioTimeInfo* ti = &asioTime.timeInfo;
	ti->sampleRate = theSampleRate;
	ASIOTimeCode* tc = &asioTime.timeCode;
	tc->speed = 1.;
	timeInfoMode = false;
	canTimeCode = false;
	if(callbacks->asioMessage(kAsioSupportsTimeInfo, 0, 0, 0) == 1)
	{
		timeInfoMode = true;
#if kCanTimeCode
		if(callbacks->asioMessage(kAsioSupportsTimeCode, 0, 0, 0) == 1)
			canTimeCode = true;
#endif
	}
}

void switchBuffers(long doubleBufferIndex, bool processNow)
{
	if(timeInfoMode)
	{
		AsioTimeInfo* ti = &asioTime.timeInfo;
		ti->flags =	kSystemTimeValid | kSamplePositionValid | kSampleRateValid;
		ti->systemTime = theNanoSeconds;
		ti->samplePosition = theSamplePosition;
		if(ti->sampleRate != theSampleRate)
			ti->flags |= kSampleRateChanged;
		ti->sampleRate = theSampleRate;

#if kCanTimeCode
		if(canTimeCode && timeCodeEnabled)
		{
			ASIOTimeCode* tc = &asioTime.timeCode;
			tc->timeCodeSamples = tcSamples;						// tc in samples
			tc->flags = kTcValid | kTcRunning | kTcOnspeed;			// if so...
		}
		ASIOTime* bb = callbacks->bufferSwitchTimeInfo(&asioTime, doubleBufferIndex, processNow ? ASIOTrue : ASIOFalse);
#else
		callbacks->bufferSwitchTimeInfo(&asioTime, doubleBufferIndex, processNow ? ASIOTrue : ASIOFalse);
#endif
	}
	else
		callbacks->bufferSwitch(doubleBufferIndex, ASIOFalse);
}

ASIOError ASIOFuture(long selector, void *params)
{
	switch(selector)
	{
		case kAsioEnableTimeCodeRead:
			timeCodeEnabled = true;
			return ASE_SUCCESS;
		case kAsioDisableTimeCodeRead:
			timeCodeEnabled = false;
			return ASE_SUCCESS;
		case kAsioCanTimeInfo:
			return ASE_SUCCESS;
		#if kCanTimeCode
		case kAsioCanTimeCode:
			return ASE_SUCCESS;
		#endif
	}
	return ASE_NotPresent;
};

*/

//- - - - - - - - - - - - - - - - - - - - - - - - -
// application's audio stream handler callbacks
//- - - - - - - - - - - - - - - - - - - - - - - - -

typedef struct ASIOCallbacks {
	void (*bufferSwitch)(long doubleBufferIndex, ASIOBool directProcess);
	// bufferSwitch indicates that both input and output are to be processed.
	// the current buffer half index (0 for A, 1 for B) determines
	// - the output buffer that the host should start to fill. the other buffer
	//   will be passed to output hardware regardless of whether it got filled
	//   in time or not.
	// - the input buffer that is now filled with incoming data. Note that
	//   because of the synchronicity of i/o, the input always has at
	//   least one buffer latency in relation to the output.
	// directProcess suggests to the host whether it should immedeately
	// start processing (directProcess == ASIOTrue), or whether its process
	// should be deferred because the call comes from a very low level
	// (for instance, a high level priority interrupt), and direct processing
	// would cause timing instabilities for the rest of the system. If in doubt,
	// directProcess should be set to ASIOFalse.
	// Note: bufferSwitch may be called at interrupt time for highest efficiency.

	void (*sampleRateDidChange)(ASIOSampleRate sRate);
	// gets called when the AudioStreamIO detects a sample rate change
	// If sample rate is unknown, 0 is passed (for instance, clock loss
	// when externally synchronized).

	long (*asioMessage)(long selector, long value, void *message, double *opt);
	// generic callback for various purposes, see selectors below.
	// note this is only present if the asio version is 2 or higher

	ASIOTime *(*bufferSwitchTimeInfo)(ASIOTime *params, long doubleBufferIndex, ASIOBool directProcess);
	// new callback with time info. makes ASIOGetSamplePosition() and various
	// calls to ASIOGetSampleRate obsolete,
	// and allows for timecode sync etc. to be preferred; will be used if
	// the driver calls asioMessage with selector kAsioSupportsTimeInfo.
} ASIOCallbacks;

// asioMessage selectors
enum {
	kAsioSelectorSupported = 1, // selector in <value>, returns 1L if supported,
				    // 0 otherwise
	kAsioEngineVersion,         // returns engine (host) asio implementation version,
				    // 2 or higher
	kAsioResetRequest,          // request driver reset. if accepted, this
				    // will close the driver (ASIO_Exit() ) and
				    // re-open it again (ASIO_Init() etc). some
				    // drivers need to reconfigure for instance
				    // when the sample rate changes, or some basic
				    // changes have been made in ASIO_ControlPanel().
				    // returns 1L; note the request is merely passed
				    // to the application, there is no way to determine
				    // if it gets accepted at this time (but it usually
				    // will be).
	kAsioBufferSizeChange,      // not yet supported, will currently always return 0L.
				    // for now, use kAsioResetRequest instead.
				    // once implemented, the new buffer size is expected
				    // in <value>, and on success returns 1L
	kAsioResyncRequest,         // the driver went out of sync, such that
				    // the timestamp is no longer valid. this
				    // is a request to re-start the engine and
				    // slave devices (sequencer). returns 1 for ok,
				    // 0 if not supported.
	kAsioLatenciesChanged,      // the drivers latencies have changed. The engine
				    // will refetch the latencies.
	kAsioSupportsTimeInfo,      // if host returns true here, it will expect the
				    // callback bufferSwitchTimeInfo to be called instead
				    // of bufferSwitch
	kAsioSupportsTimeCode,      //
	kAsioMMCCommand,            // unused - value: number of commands, message points to mmc commands
	kAsioSupportsInputMonitor,  // kAsioSupportsXXX return 1 if host supports this
	kAsioSupportsInputGain,     // unused and undefined
	kAsioSupportsInputMeter,    // unused and undefined
	kAsioSupportsOutputGain,    // unused and undefined
	kAsioSupportsOutputMeter,   // unused and undefined
	kAsioOverload,              // driver detected an overload

	kAsioNumMessageSelectors
};

//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------

//- - - - - - - - - - - - - - - - - - - - - - - - -
// (De-)Construction
//- - - - - - - - - - - - - - - - - - - - - - - - -

typedef struct ASIODriverInfo {
	long asioVersion;   // currently, 2
	long driverVersion; // driver specific
	char name[32];
	char errorMessage[124];
	void *sysRef; // on input: system reference
		      // (Windows: application main window handle, Mac & SGI: 0)
} ASIODriverInfo;

ASIOError ASIOInit(ASIODriverInfo *info);
/* Purpose:
	  Initialize the AudioStreamIO.
	Parameter:
	  info: pointer to an ASIODriver structure:
	    - asioVersion:
			- on input, the host version. *** Note *** this is 0 for earlier asio
			implementations, and the asioMessage callback is implemeted
			only if asioVersion is 2 or greater. sorry but due to a design fault
			the driver doesn't have access to the host version in ASIOInit :-(
			added selector for host (engine) version in the asioMessage callback
			so we're ok from now on.
			- on return, asio implementation version.
			  older versions are 1
			  if you support this version (namely, ASIO_outputReady() )
			  this should be 2 or higher. also see the note in
			  ASIO_getTimeStamp() !
	    - version: on return, the driver version (format is driver specific)
	    - name: on return, a null-terminated string containing the driver's name
		- error message: on return, should contain a user message describing
		  the type of error that occured during ASIOInit(), if any.
		- sysRef: platform specific
	Returns:
	  If neither input nor output is present ASE_NotPresent
	  will be returned.
	  ASE_NoMemory, ASE_HWMalfunction are other possible error conditions
*/

ASIOError ASIOExit(void);
/* Purpose:
	  Terminates the AudioStreamIO.
	Parameter:
	  None.
	Returns:
	  If neither input nor output is present ASE_NotPresent
	  will be returned.
	Notes: this implies ASIOStop() and ASIODisposeBuffers(),
	  meaning that no host callbacks must be accessed after ASIOExit().
*/

//- - - - - - - - - - - - - - - - - - - - - - - - -
// Start/Stop
//- - - - - - - - - - - - - - - - - - - - - - - - -

ASIOError ASIOStart(void);
/* Purpose:
	  Start input and output processing synchronously.
	  This will
	  - reset the sample counter to zero
	  - start the hardware (both input and output)
	    The first call to the hosts' bufferSwitch(index == 0) then tells
	    the host to read from input buffer A (index 0), and start
	    processing to output buffer A while output buffer B (which
	    has been filled by the host prior to calling ASIOStart())
	    is possibly sounding (see also ASIOGetLatencies()) 
	Parameter:
	  None.
	Returns:
	  If neither input nor output is present, ASE_NotPresent
	  will be returned.
	  If the hardware fails to start, ASE_HWMalfunction will be returned.
	Notes:
	  There is no restriction on the time that ASIOStart() takes
	  to perform (that is, it is not considered a realtime trigger).
*/

ASIOError ASIOStop(void);
/* Purpose:
	  Stops input and output processing altogether.
	Parameter:
	  None.
	Returns:
	  If neither input nor output is present ASE_NotPresent
	  will be returned.
	Notes:
	  On return from ASIOStop(), the driver must in no
	  case call the hosts' bufferSwitch() routine.
*/

//- - - - - - - - - - - - - - - - - - - - - - - - -
// Inquiry methods and sample rate
//- - - - - - - - - - - - - - - - - - - - - - - - -

ASIOError ASIOGetChannels(long *numInputChannels, long *numOutputChannels);
/* Purpose:
	  Returns number of individual input/output channels.
	Parameter:
	  numInputChannels will hold the number of available input channels
	  numOutputChannels will hold the number of available output channels
	Returns:
	  If no input/output is present ASE_NotPresent will be returned.
	  If only inputs, or only outputs are available, the according
	  other parameter will be zero, and ASE_OK is returned.
*/

ASIOError ASIOGetLatencies(long *inputLatency, long *outputLatency);
/* Purpose:
	  Returns the input and output latencies. This includes
	  device specific delays, like FIFOs etc.
	Parameter:
	  inputLatency will hold the 'age' of the first sample frame
	  in the input buffer when the hosts reads it in bufferSwitch()
	  (this is theoretical, meaning it does not include the overhead
	  and delay between the actual physical switch, and the time
	  when bufferSitch() enters).
	  This will usually be the size of one block in sample frames, plus
	  device specific latencies.

	  outputLatency will specify the time between the buffer switch,
	  and the time when the next play buffer will start to sound.
	  The next play buffer is defined as the one the host starts
	  processing after (or at) bufferSwitch(), indicated by the
	  index parameter (0 for buffer A, 1 for buffer B).
	  It will usually be either one block, if the host writes directly
	  to a dma buffer, or two or more blocks if the buffer is 'latched' by
	  the driver. As an example, on ASIOStart(), the host will have filled
	  the play buffer at index 1 already; when it gets the callback (with
	  the parameter index == 0), this tells it to read from the input
	  buffer 0, and start to fill the play buffer 0 (assuming that now
	  play buffer 1 is already sounding). In this case, the output
	  latency is one block. If the driver decides to copy buffer 1
	  at that time, and pass it to the hardware at the next slot (which
	  is most commonly done, but should be avoided), the output latency
	  becomes two blocks instead, resulting in a total i/o latency of at least
	  3 blocks. As memory access is the main bottleneck in native dsp processing,
	  and to acheive less latency, it is highly recommended to try to avoid
	  copying (this is also why the driver is the owner of the buffers). To
	  summarize, the minimum i/o latency can be acheived if the input buffer
	  is processed by the host into the output buffer which will physically
	  start to sound on the next time slice. Also note that the host expects
	  the bufferSwitch() callback to be accessed for each time slice in order
	  to retain sync, possibly recursively; if it fails to process a block in
	  time, it will suspend its operation for some time in order to recover.
	Returns:
	  If no input/output is present ASE_NotPresent will be returned.
*/

ASIOError ASIOGetBufferSize(long *minSize, long *maxSize, long *preferredSize, long *granularity);
/* Purpose:
	  Returns min, max, and preferred buffer sizes for input/output
	Parameter:
	  minSize will hold the minimum buffer size
	  maxSize will hold the maxium possible buffer size
	  preferredSize will hold the preferred buffer size (a size which
	  best fits performance and hardware requirements)
	  granularity will hold the granularity at which buffer sizes
	  may differ. Usually, the buffer size will be a power of 2;
	  in this case, granularity will hold -1 on return, signalling
	  possible buffer sizes starting from minSize, increased in
	  powers of 2 up to maxSize.
	Returns:
	  If no input/output is present ASE_NotPresent will be returned.
	Notes:
	  When minimum and maximum buffer size are equal,
	  the preferred buffer size has to be the same value as well; granularity
	  should be 0 in this case.
*/

ASIOError ASIOCanSampleRate(ASIOSampleRate sampleRate);
/* Purpose:
	  Inquires the hardware for the available sample rates.
	Parameter:
	  sampleRate is the rate in question.
	Returns:
	  If the inquired sample rate is not supported, ASE_NoClock will be returned.
	  If no input/output is present ASE_NotPresent will be returned.
*/
ASIOError ASIOGetSampleRate(ASIOSampleRate *currentRate);
/* Purpose:
	  Get the current sample Rate.
	Parameter:
	  currentRate will hold the current sample rate on return.
	Returns:
	  If sample rate is unknown, sampleRate will be 0 and ASE_NoClock will be returned.
	  If no input/output is present ASE_NotPresent will be returned.
	Notes:
*/

ASIOError ASIOSetSampleRate(ASIOSampleRate sampleRate);
/* Purpose:
	  Set the hardware to the requested sample Rate. If sampleRate == 0,
	  enable external sync.
	Parameter:
	  sampleRate: on input, the requested rate
	Returns:
	  If sampleRate is unknown ASE_NoClock will be returned.
	  If the current clock is external, and sampleRate is != 0,
	  ASE_InvalidMode will be returned
	  If no input/output is present ASE_NotPresent will be returned.
	Notes:
*/

typedef struct ASIOClockSource {
	long index;               // as used for ASIOSetClockSource()
	long associatedChannel;   // for instance, S/PDIF or AES/EBU
	long associatedGroup;     // see channel groups (ASIOGetChannelInfo())
	ASIOBool isCurrentSource; // ASIOTrue if this is the current clock source
	char name[32];            // for user selection
} ASIOClockSource;

ASIOError ASIOGetClockSources(ASIOClockSource *clocks, long *numSources);
/* Purpose:
	  Get the available external audio clock sources
	Parameter:
	  clocks points to an array of ASIOClockSource structures:
	  	- index: this is used to identify the clock source
	  	  when ASIOSetClockSource() is accessed, should be
	  	  an index counting from zero
	  	- associatedInputChannel: the first channel of an associated
	  	  input group, if any.
	  	- associatedGroup: the group index of that channel.
	  	  groups of channels are defined to seperate for
	  	  instance analog, S/PDIF, AES/EBU, ADAT connectors etc,
	  	  when present simultaniously. Note that associated channel
	  	  is enumerated according to numInputs/numOutputs, means it
	  	  is independant from a group (see also ASIOGetChannelInfo())
	  	  inputs are associated to a clock if the physical connection
	  	  transfers both data and clock (like S/PDIF, AES/EBU, or
	  	  ADAT inputs). if there is no input channel associated with
	  	  the clock source (like Word Clock, or internal oscillator), both
	  	  associatedChannel and associatedGroup should be set to -1.
	  	- isCurrentSource: on exit, ASIOTrue if this is the current clock
	  	  source, ASIOFalse else
		- name: a null-terminated string for user selection of the available sources.
	  numSources:
	      on input: the number of allocated array members
	      on output: the number of available clock sources, at least
	      1 (internal clock generator).
	Returns:
	  If no input/output is present ASE_NotPresent will be returned.
	Notes:
*/

ASIOError ASIOSetClockSource(long index);
/* Purpose:
	  Set the audio clock source
	Parameter:
	  index as obtained from an inquiry to ASIOGetClockSources()
	Returns:
	  If no input/output is present ASE_NotPresent will be returned.
	  If the clock can not be selected because an input channel which
	  carries the current clock source is active, ASE_InvalidMode
	  *may* be returned (this depends on the properties of the driver
	  and/or hardware).
	Notes:
	  Should *not* return ASE_NoClock if there is no clock signal present
	  at the selected source; this will be inquired via ASIOGetSampleRate().
	  It should call the host callback procedure sampleRateHasChanged(),
	  if the switch causes a sample rate change, or if no external clock
	  is present at the selected source.
*/

ASIOError ASIOGetSamplePosition(ASIOSamples *sPos, ASIOTimeStamp *tStamp);
/* Purpose:
	  Inquires the sample position/time stamp pair.
	Parameter:
	  sPos will hold the sample position on return. The sample
	  position is reset to zero when ASIOStart() gets called.
	  tStamp will hold the system time when the sample position
	  was latched.
	Returns:
	  If no input/output is present, ASE_NotPresent will be returned.
	  If there is no clock, ASE_SPNotAdvancing will be returned.
	Notes:

	  in order to be able to synchronise properly,
	  the sample position / time stamp pair must refer to the current block,
	  that is, the engine will call ASIOGetSamplePosition() in its bufferSwitch()
	  callback and expect the time for the current block. thus, when requested
	  in the very first bufferSwitch after ASIO_Start(), the sample position
	  should be zero, and the time stamp should refer to the very time where
	  the stream was started. it also means that the sample position must be
	  block aligned. the driver must ensure proper interpolation if the system
	  time can not be determined for the block position. the driver is responsible
	  for precise time stamps as it usually has most direct access to lower
	  level resources. proper behaviour of ASIO_GetSamplePosition() and ASIO_GetLatencies()
	  are essential for precise media synchronization!
*/

typedef struct ASIOChannelInfo {
	long channel;        // on input, channel index
	ASIOBool isInput;    // on input
	ASIOBool isActive;   // on exit
	long channelGroup;   // dto
	ASIOSampleType type; // dto
	char name[32];       // dto
} ASIOChannelInfo;

ASIOError ASIOGetChannelInfo(ASIOChannelInfo *info);
/* Purpose:
	  retreive information about the nature of a channel
	Parameter:
	  info: pointer to a ASIOChannelInfo structure with
	  	- channel: on input, the channel index of the channel in question.
	  	- isInput: on input, ASIOTrue if info for an input channel is
	  	  requested, else output
		- channelGroup: on return, the channel group that the channel
		  belongs to. For drivers which support different types of
		  channels, like analog, S/PDIF, AES/EBU, ADAT etc interfaces,
		  there should be a reasonable grouping of these types. Groups
		  are always independant form a channel index, that is, a channel
		  index always counts from 0 to numInputs/numOutputs regardless
		  of the group it may belong to.
		  There will always be at least one group (group 0). Please
		  also note that by default, the host may decide to activate
		  channels 0 and 1; thus, these should belong to the most
		  useful type (analog i/o, if present).
	  	- type: on return, contains the sample type of the channel
	  	- isActive: on return, ASIOTrue if channel is active as it was
	  	  installed by ASIOCreateBuffers(), ASIOFalse else
	  	- name:  describing the type of channel in question. Used to allow
	  	  for user selection, and enabling of specific channels. examples:
	      "Analog In", "SPDIF Out" etc
	Returns:
	  If no input/output is present ASE_NotPresent will be returned.
	Notes:
	  If possible, the string should be organised such that the first
	  characters are most significantly describing the nature of the
	  port, to allow for identification even if the view showing the
	  port name is too small to display more than 8 characters, for
	  instance.
*/

//- - - - - - - - - - - - - - - - - - - - - - - - -
// Buffer preparation
//- - - - - - - - - - - - - - - - - - - - - - - - -

typedef struct ASIOBufferInfo {
	ASIOBool isInput; // on input:  ASIOTrue: input, else output
	long channelNum;  // on input:  channel index
	void *buffers[2]; // on output: double buffer addresses
} ASIOBufferInfo;

ASIOError ASIOCreateBuffers(ASIOBufferInfo *bufferInfos, long numChannels, long bufferSize, ASIOCallbacks *callbacks);

/* Purpose:
	  Allocates input/output buffers for all input and output channels to be activated.
	Parameter:
	  bufferInfos is a pointer to an array of ASIOBufferInfo structures:
	    - isInput: on input, ASIOTrue if the buffer is to be allocated
	      for an input, output buffer else
	    - channelNum: on input, the index of the channel in question
	      (counting from 0)
	    - buffers: on exit, 2 pointers to the halves of the channels' double-buffer.
	      the size of the buffer(s) of course depend on both the ASIOSampleType
	      as obtained from ASIOGetChannelInfo(), and bufferSize
	  numChannels is the sum of all input and output channels to be created;
	  thus bufferInfos is a pointer to an array of numChannels ASIOBufferInfo
	  structures.
	  bufferSize selects one of the possible buffer sizes as obtained from
	  ASIOGetBufferSizes().
	  callbacks is a pointer to an ASIOCallbacks structure.
	Returns:
	  If not enough memory is available ASE_NoMemory will be returned.
	  If no input/output is present ASE_NotPresent will be returned.
	  If bufferSize is not supported, or one or more of the bufferInfos elements
	  contain invalid settings, ASE_InvalidMode will be returned.
	Notes:
	  If individual channel selection is not possible but requested,
	  the driver has to handle this. namely, bufferSwitch() will only
	  have filled buffers of enabled outputs. If possible, processing
	  and buss activities overhead should be avoided for channels which
	  were not enabled here.
*/

ASIOError ASIODisposeBuffers(void);
/* Purpose:
	  Releases all buffers for the device.
	Parameter:
	  None.
	Returns:
	  If no buffer were ever prepared, ASE_InvalidMode will be returned.
	  If no input/output is present ASE_NotPresent will be returned.
	Notes:
	  This implies ASIOStop().
*/

ASIOError ASIOControlPanel(void);
/* Purpose:
	  request the driver to start a control panel component
	  for device specific user settings. This will not be
	  accessed on some platforms (where the component is accessed
	  instead).
	Parameter:
	  None.
	Returns:
	  If no panel is available ASE_NotPresent will be returned.
	  Actually, the return code is ignored.
	Notes:
	  if the user applied settings which require a re-configuration
	  of parts or all of the enigine and/or driver (such as a change of
	  the block size), the asioMessage callback can be used (see
	  ASIO_Callbacks).
*/

ASIOError ASIOFuture(long selector, void *params);
/* Purpose:
	  various
	Parameter:
	  selector: operation Code as to be defined. zero is reserved for
	  testing purposes.
	  params: depends on the selector; usually pointer to a structure
	  for passing and retreiving any type and amount of parameters.
	Returns:
	  the return value is also selector dependant. if the selector
	  is unknown, ASE_InvalidParameter should be returned to prevent
	  further calls with this selector. on success, ASE_SUCCESS
	  must be returned (note: ASE_OK is *not* sufficient!)
	Notes:
	  see selectors defined below.	  
*/

enum {
	kAsioEnableTimeCodeRead = 1, // no arguments
	kAsioDisableTimeCodeRead,    // no arguments
	kAsioSetInputMonitor,        // ASIOInputMonitor* in params
	kAsioTransport,              // ASIOTransportParameters* in params
	kAsioSetInputGain,           // ASIOChannelControls* in params, apply gain
	kAsioGetInputMeter,          // ASIOChannelControls* in params, fill meter
	kAsioSetOutputGain,          // ASIOChannelControls* in params, apply gain
	kAsioGetOutputMeter,         // ASIOChannelControls* in params, fill meter
	kAsioCanInputMonitor,        // no arguments for kAsioCanXXX selectors
	kAsioCanTimeInfo,
	kAsioCanTimeCode,
	kAsioCanTransport,
	kAsioCanInputGain,
	kAsioCanInputMeter,
	kAsioCanOutputGain,
	kAsioCanOutputMeter,
	kAsioOptionalOne,

	//	DSD support
	//	The following extensions are required to allow switching
	//	and control of the DSD subsystem.
	kAsioSetIoFormat = 0x23111961,   /* ASIOIoFormat * in params.			*/
	kAsioGetIoFormat = 0x23111983,   /* ASIOIoFormat * in params.			*/
	kAsioCanDoIoFormat = 0x23112004, /* ASIOIoFormat * in params.			*/

	// Extension for drop out detection
	kAsioCanReportOverload = 0x24042012, /* return ASE_SUCCESS if driver can detect and report overloads */

	kAsioGetInternalBufferSamples =
		0x25042012 /* ASIOInternalBufferInfo * in params. Deliver size of driver internal buffering, return ASE_SUCCESS if supported */
};

typedef struct ASIOInputMonitor {
	long input;     // this input was set to monitor (or off), -1: all
	long output;    // suggested output for monitoring the input (if so)
	long gain;      // suggested gain, ranging 0 - 0x7fffffffL (-inf to +12 dB)
	ASIOBool state; // ASIOTrue => on, ASIOFalse => off
	long pan;       // suggested pan, 0 => all left, 0x7fffffff => right
} ASIOInputMonitor;

typedef struct ASIOChannelControls {
	long channel;     // on input, channel index
	ASIOBool isInput; // on input
	long gain;        // on input,  ranges 0 thru 0x7fffffff
	long meter;       // on return, ranges 0 thru 0x7fffffff
	char future[32];
} ASIOChannelControls;

typedef struct ASIOTransportParameters {
	long command; // see enum below
	ASIOSamples samplePosition;
	long track;
	long trackSwitches[16]; // 512 tracks on/off
	char future[64];
} ASIOTransportParameters;

enum {
	kTransStart = 1,
	kTransStop,
	kTransLocate, // to samplePosition
	kTransPunchIn,
	kTransPunchOut,
	kTransArmOn,      // track
	kTransArmOff,     // track
	kTransMonitorOn,  // track
	kTransMonitorOff, // track
	kTransArm,        // trackSwitches
	kTransMonitor     // trackSwitches
};

/*
// DSD support
//	Some notes on how to use ASIOIoFormatType.
//
//	The caller will fill the format with the request types.
//	If the board can do the request then it will leave the
//	values unchanged. If the board does not support the
//	request then it will change that entry to Invalid (-1)
//
//	So to request DSD then
//
//	ASIOIoFormat NeedThis={kASIODSDFormat};
//
//	if(ASE_SUCCESS != ASIOFuture(kAsioSetIoFormat,&NeedThis) ){
//		// If the board did not accept one of the parameters then the
//		// whole call will fail and the failing parameter will
//		// have had its value changes to -1.
//	}
//
// Note: Switching between the formats need to be done before the "prepared"
// state (see ASIO 2 documentation) is entered.
*/
typedef long int ASIOIoFormatType;
enum ASIOIoFormatType_e {
	kASIOFormatInvalid = -1,
	kASIOPCMFormat = 0,
	kASIODSDFormat = 1,
};

typedef struct ASIOIoFormat_s {
	ASIOIoFormatType FormatType;
	char future[512 - sizeof(ASIOIoFormatType)];
} ASIOIoFormat;

// Extension for drop detection
// Note: Refers to buffering that goes beyond the double buffer e.g. used by USB driver designs
typedef struct ASIOInternalBufferInfo {
	long inputSamples;  // size of driver's internal input buffering which is included in getLatencies
	long outputSamples; // size of driver's internal output buffering which is included in getLatencies
} ASIOInternalBufferInfo;

ASIOError ASIOOutputReady(void);
/* Purpose:
	  this tells the driver that the host has completed processing
	  the output buffers. if the data format required by the hardware
	  differs from the supported asio formats, but the hardware
	  buffers are DMA buffers, the driver will have to convert
	  the audio stream data; as the bufferSwitch callback is
	  usually issued at dma block switch time, the driver will
	  have to convert the *previous* host buffer, which increases
	  the output latency by one block.
	  when the host finds out that ASIOOutputReady() returns
	  true, it will issue this call whenever it completed
	  output processing. then the driver can convert the
	  host data directly to the dma buffer to be played next,
	  reducing output latency by one block.
	  another way to look at it is, that the buffer switch is called
	  in order to pass the *input* stream to the host, so that it can
	  process the input into the output, and the output stream is passed
	  to the driver when the host has completed its process.
	Parameter:
		None
	Returns:
	  only if the above mentioned scenario is given, and a reduction
	  of output latency can be acheived by this mechanism, should
	  ASE_OK be returned. otherwise (and usually), ASE_NotPresent
	  should be returned in order to prevent further calls to this
	  function. note that the host may want to determine if it is
	  to use this when the system is not yet fully initialized, so
	  ASE_OK should always be returned if the mechanism makes sense.	  
	Notes:
	  please remeber to adjust ASIOGetLatencies() according to
	  whether ASIOOutputReady() was ever called or not, if your
	  driver supports this scenario.
	  also note that the engine may fail to call ASIO_OutputReady()
	  in time in overload cases. as already mentioned, bufferSwitch
      should be called for every block regardless of whether a block
      could be processed in time.
*/

// restore old alignment
#if defined(_MSC_VER) && !defined(__MWERKS__)
#pragma pack(pop)
#elif PRAGMA_ALIGN_SUPPORTED
#pragma options align = reset
#endif

#endif
