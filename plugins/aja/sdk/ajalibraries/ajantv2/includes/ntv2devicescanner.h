/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2devicescanner.h
	@brief		Declares the CNTV2DeviceScanner class.
	@copyright	(C) 2004-2021 AJA Video Systems, Inc.
**/

#ifndef NTV2DEVICESCANNER_H
#define NTV2DEVICESCANNER_H

#include "ajaexport.h"
#include "ntv2card.h"
#include "string.h"
#include <vector>
#include <algorithm>


typedef std::vector <AudioSampleRateEnum>				NTV2AudioSampleRateList;
typedef NTV2AudioSampleRateList::const_iterator			NTV2AudioSampleRateListConstIter;
typedef NTV2AudioSampleRateList::iterator				NTV2AudioSampleRateListIter;

typedef std::vector <AudioChannelsPerFrameEnum>			NTV2AudioChannelsPerFrameList;
typedef NTV2AudioChannelsPerFrameList::const_iterator	NTV2AudioChannelsPerFrameListConstIter;
typedef NTV2AudioChannelsPerFrameList::iterator			NTV2AudioChannelsPerFrameListIter;

typedef std::vector <AudioSourceEnum>					NTV2AudioSourceList;
typedef NTV2AudioSourceList::const_iterator				NTV2AudioSourceListConstIter;
typedef NTV2AudioSourceList::iterator					NTV2AudioSourceListIter;

typedef std::vector <AudioBitsPerSampleEnum>			NTV2AudioBitsPerSampleList;
typedef NTV2AudioBitsPerSampleList::const_iterator		NTV2AudioBitsPerSampleListConstIter;
typedef NTV2AudioBitsPerSampleList::iterator			NTV2AudioBitsPerSampleListIter;


/**
	@deprecated	Please use the functions provided in 'ntv2devicefeatures.h' and 'ntv2devicefeatures.hh' instead.
**/
typedef struct NTV2DeviceInfo
{
	NTV2DeviceID					deviceID;							///< @brief	Device ID/species	(e.g., DEVICE_ID_KONA3G, DEVICE_ID_IOXT, etc.)
	ULWord							deviceIndex;						///< @brief		Device index number -- this will be phased out someday
	ULWord							pciSlot;							///< @brief	PCI slot (if applicable and/or known)
	uint64_t						deviceSerialNumber;					///< @brief	Unique device serial number
	std::string						deviceIdentifier;					///< @brief	Device name as seen in Control Panel, Watcher, Cables, etc.
	UWord                           numVidInputs;						///< @brief	Total number of video inputs -- analog, digital, whatever
	UWord							numVidOutputs;						///< @brief	Total number of video outputs -- analog, digital, whatever
	UWord							numAnlgVidInputs;					///< @brief	Total number of analog video inputs
	UWord							numAnlgVidOutputs;					///< @brief	Total number of analog video outputs
	UWord							numHDMIVidInputs;					///< @brief	Total number of HDMI inputs
	UWord							numHDMIVidOutputs;					///< @brief	Total number of HDMI outputs
	UWord							numInputConverters;					///< @brief	Total number of input converters
	UWord							numOutputConverters;				///< @brief	Total number of output converters
	UWord							numUpConverters;					///< @brief	Total number of up-converters
	UWord							numDownConverters;					///< @brief	Total number of down-converters
	UWord							downConverterDelay;
	bool							isoConvertSupport;
	bool							rateConvertSupport;
	bool							dvcproHDSupport;
	bool							qrezSupport;
	bool							hdvSupport;
	bool							quarterExpandSupport;
	bool							vidProcSupport;
	bool							dualLinkSupport;					///< @brief	Supports dual-link?
	bool							colorCorrectionSupport;				///< @brief	Supports color correction?
	bool							programmableCSCSupport;				///< @brief	Programmable color space converter?
	bool							rgbAlphaOutputSupport;				///< @brief	Supports RGB alpha channel?
	bool							breakoutBoxSupport;					///< @brief	Can support a breakout box?
	bool							procAmpSupport;
	bool							has2KSupport;						///< @brief	Supports 2K formats?
	bool							has4KSupport;						///< @brief	Supports 4K formats?
	bool							has8KSupport;						///< @brief	Supports 8K formats?
    bool                            has3GLevelConversion;               ///< @brief	Supports 3G Level Conversion?
	bool							proResSupport;						///< @brief	Supports ProRes?
	bool							sdi3GSupport;						///< @brief	Supports 3G?
	bool							sdi12GSupport;						///< @brief	Supports 12G?
	bool							ipSupport;							///< @brief	Supports IP IO?
	bool							biDirectionalSDI;					///< @brief	Supports Bi-directional SDI
	bool							ltcInSupport;						///< @brief	Accepts LTC input?
	bool							ltcOutSupport;						///< @brief	Supports LTC output?
	bool							ltcInOnRefPort;						///< @brief	Supports LTC on reference input?
	bool							stereoOutSupport;					///< @brief	Supports stereo output?
	bool							stereoInSupport;					///< @brief	Supports stereo input?
	bool							multiFormat;						///< @brief	Supports multiple video formats?
	NTV2AudioSampleRateList			audioSampleRateList;				///< @brief	My supported audio sample rates
	NTV2AudioChannelsPerFrameList	audioNumChannelsList;				///< @brief	My supported number of audio channels per frame
	NTV2AudioBitsPerSampleList		audioBitsPerSampleList;				///< @brief	My supported audio bits-per-sample
	NTV2AudioSourceList				audioInSourceList;					///< @brief	My supported audio input sources (AES, ADAT, etc.)
	NTV2AudioSourceList				audioOutSourceList;					///< @brief	My supported audio output destinations (AES, etc.)
	UWord							numAudioStreams;					///< @brief	Maximum number of independent audio streams
	UWord							numAnalogAudioInputChannels;		///< @brief	Total number of analog audio input channels
	UWord							numAESAudioInputChannels;			///< @brief	Total number of AES audio input channels
	UWord							numEmbeddedAudioInputChannels;		///< @brief	Total number of embedded (SDI) audio input channels
	UWord							numHDMIAudioInputChannels;			///< @brief	Total number of HDMI audio input channels
	UWord							numAnalogAudioOutputChannels;		///< @brief	Total number of analog audio output channels
	UWord							numAESAudioOutputChannels;			///< @brief	Total number of AES audio output channels
	UWord							numEmbeddedAudioOutputChannels;		///< @brief	Total number of embedded (SDI) audio output channels
	UWord							numHDMIAudioOutputChannels;			///< @brief	Total number of HDMI audio output channels
	UWord							numDMAEngines;						///< @brief	Total number of DMA engines
	UWord							numSerialPorts;						///< @brief	Total number of serial ports
	ULWord							pingLED;
	#if !defined (NTV2_DEPRECATE)
		uint64_t		boardSerialNumber;				///< @deprecated	Use deviceSerialNumber instead
		char			boardIdentifier[32];			///< @deprecated	Use deviceIdentifier instead
		NTV2BoardType	boardType;						///< @deprecated	No longer used. Remove deviceType from the source code instead
		NTV2BoardID		boardID;						///< @deprecated	Use deviceID instead
		ULWord			boardNumber;					///< @deprecated	Use deviceIndex instead
	#endif	//	!defined (NTV2_DEPRECATE)

	AJAExport	bool operator == (const NTV2DeviceInfo & rhs) const;	///< @return	True if I'm equivalent to another ::NTV2DeviceInfo struct.
	AJAExport	inline bool operator != (const NTV2DeviceInfo & rhs) const	{ return !(*this == rhs); }	///< @return	True if I'm different from another ::NTV2DeviceInfo struct.

} NTV2DeviceInfo;



//	ostream operators

/**
	@brief	Streams the NTV2AudioSampleRateList to the given output stream in a human-readable format.
	@param		inOutStr	The output stream into which the NTV2AudioSampleRateList is to be streamed.
	@param[in]	inList		Specifies the NTV2AudioSampleRateList to be streamed.
	@return		The output stream.
**/
AJAExport	std::ostream &	operator << (std::ostream & inOutStr, const NTV2AudioSampleRateList & inList);

/**
	@brief	Streams the NTV2AudioChannelsPerFrameList to the given output stream in a human-readable format.
	@param		inOutStr	The output stream into which the NTV2AudioChannelsPerFrameList is to be streamed.
	@param[in]	inList		Specifies the NTV2AudioChannelsPerFrameList to be streamed.
	@return		The output stream.
**/
AJAExport	std::ostream &	operator << (std::ostream & inOutStr, const NTV2AudioChannelsPerFrameList & inList);

/**
	@brief	Streams the NTV2AudioSourceList to the given output stream in a human-readable format.
	@param		inOutStr	The output stream into which the NTV2AudioSourceList is to be streamed.
	@param[in]	inList		Specifies the NTV2AudioSourceList to be streamed.
	@return		The output stream.
**/
AJAExport	std::ostream &	operator << (std::ostream & inOutStr, const NTV2AudioSourceList & inList);

/**
	@brief	Streams the NTV2AudioBitsPerSampleList to the given output stream in a human-readable format.
	@param		inOutStr	The output stream into which the NTV2AudioBitsPerSampleList is to be streamed.
	@param[in]	inList		Specifies the NTV2AudioBitsPerSampleList to be streamed.
	@return		The output stream.
**/
AJAExport	std::ostream &	operator << (std::ostream & inOutStr, const NTV2AudioBitsPerSampleList & inList);

/**
	@brief	Streams the NTV2DeviceInfo to the given output stream in a human-readable format.
	@param		inOutStr	The output stream into which the NTV2DeviceInfo is to be streamed.
	@param[in]	inInfo		Specifies the NTV2DeviceInfo to be streamed.
	@return		The output stream.
**/
AJAExport	std::ostream &	operator << (std::ostream & inOutStr, const NTV2DeviceInfo & inInfo);


/**
	@brief	I am an ordered list of NTV2DeviceInfo structs.
**/
typedef std::vector <NTV2DeviceInfo>		NTV2DeviceInfoList;
typedef NTV2DeviceInfoList::const_iterator	NTV2DeviceInfoListConstIter;	//	Const iterator shorthand
typedef NTV2DeviceInfoList::iterator		NTV2DeviceInfoListIter;			//	Iterator shorthand


/**
	@brief		Streams the NTV2DeviceInfoList to an output stream in a human-readable format.
	@param		inOutStr	The output stream into which the NTV2DeviceInfoList is to be streamed.
	@param[in]	inList		Specifies the NTV2DeviceInfoList to be streamed.
	@return		The output stream.
**/
AJAExport	std::ostream &	operator << (std::ostream & inOutStr, const NTV2DeviceInfoList & inList);


typedef struct {
	ULWord							boardNumber;
	AudioSampleRateEnum				sampleRate;
	AudioChannelsPerFrameEnum		numChannels;
	AudioBitsPerSampleEnum			bitsPerSample;
	AudioSourceEnum					sourceIn;
	AudioSourceEnum					sourceOut;
} NTV2AudioPhysicalFormat;


/**
	@brief		Streams the AudioPhysicalFormat to an output stream in a human-readable format.
	@param		inOutStr	The output stream into which the AudioPhysicalFormat is to be streamed.
	@param[in]	inFormat	Specifies the AudioPhysicalFormat to be streamed.
	@return		The output stream.
**/
AJAExport	std::ostream &	operator << (std::ostream & inOutStr, const NTV2AudioPhysicalFormat & inFormat);


/**
	@brief	I am an ordered list of NTV2AudioPhysicalFormat structs.
**/
typedef std::vector <NTV2AudioPhysicalFormat>			NTV2AudioPhysicalFormatList;
typedef NTV2AudioPhysicalFormatList::const_iterator		NTV2AudioPhysicalFormatListConstIter;	//	Shorthand for const_iterator
typedef NTV2AudioPhysicalFormatList::iterator			NTV2AudioPhysicalFormatListIter;		//	Shorthand for iterator


/**
	@brief		Streams the AudioPhysicalFormatList to an output stream in a human-readable format.
	@param		inOutStr	The output stream into which the AudioPhysicalFormatList is to be streamed.
	@param[in]	inList		Specifies the AudioPhysicalFormatList to be streamed.
	@return		The output stream.
**/
AJAExport	std::ostream &	operator << (std::ostream & inOutStr, const NTV2AudioPhysicalFormatList & inList);



/**
	@brief	This class is used to enumerate AJA devices that are attached and known to the local host computer.
**/
class AJAExport CNTV2DeviceScanner
{
//	Class Methods
public:
	/**
		@brief		Rescans the host, and returns an open CNTV2Card instance for the AJA device having the given zero-based index number.
		@return		True if successful; otherwise false.
		@param[in]	inDeviceIndexNumber	Specifies the AJA device using a zero-based index number.
		@param[out]	outDevice			Receives the open, ready-to-use CNTV2Card instance.
	**/
	static bool									GetDeviceAtIndex (const ULWord inDeviceIndexNumber, CNTV2Card & outDevice);

	/**
		@brief		Rescans the host, and returns an open CNTV2Card instance for the first AJA device found on the host that has the given NTV2DeviceID.
		@return		True if successful; otherwise false.
		@param[in]	inDeviceID			Specifies the device identifier of interest.
		@param[out]	outDevice			Receives the open, ready-to-use CNTV2Card instance.
	**/
	static bool									GetFirstDeviceWithID (const NTV2DeviceID inDeviceID, CNTV2Card & outDevice);

	/**
		@brief		Rescans the host, and returns an open CNTV2Card instance for the first AJA device whose device identifier name contains the given substring.
		@note		The name is compared case-insensitively (e.g., "iO4K" == "Io4k").
		@return		True if successful; otherwise false.
		@param[in]	inNameSubString		Specifies a portion of the device name to search for.
		@param[out]	outDevice			Receives the open, ready-to-use CNTV2Card instance.
	**/
	static bool									GetFirstDeviceWithName (const std::string & inNameSubString, CNTV2Card & outDevice);

	/**
		@brief		Rescans the host, and returns an open CNTV2Card instance for the first AJA device whose serial number contains the given value.
		@note		The serial value is compared case-sensitively.
		@return		True if successful; otherwise false.
		@param[in]	inSerialStr			Specifies the device serial value to search for.
		@param[out]	outDevice			Receives the open, ready-to-use CNTV2Card instance of the first matching device.
	**/
	static bool									GetFirstDeviceWithSerial (const std::string & inSerialStr, CNTV2Card & outDevice);	//	New in SDK 16.0

	/**
		@brief		Rescans the host, and returns an open CNTV2Card instance for the first AJA device whose serial number matches the given value.
		@return		True if successful; otherwise false.
		@param[in]	inSerialNumber		Specifies the device serial value to search for.
		@param[out]	outDevice			Receives the open, ready-to-use CNTV2Card instance.
	**/
	static bool									GetDeviceWithSerial (const uint64_t inSerialNumber, CNTV2Card & outDevice);	//	New in SDK 16.0

	/**
		@brief		Rescans the host, and returns an open CNTV2Card instance for the AJA device that matches a command line argument
					according to the following evaluation sequence:
					-#	1 or 2 digit unsigned decimal integer:  a zero-based device index number;
					-#	8 or 9 character alphanumeric string:   device with a matching serial number string (case-insensitive comparison);
					-#	3-16 character hexadecimal integer, optionally preceded by '0x':  device having a matching 64-bit serial number;
					-#	All other cases:  first device (lowest index number) whose name contains the argument string (compared case-insensitively).
		@return		True if successful; otherwise false.
		@param[in]	inArgument			The argument string. If 'list' or '?', the std::cout stream is sent some
										"help text" showing a list of all available devices.
		@param[out]	outDevice			Receives the open, ready-to-use CNTV2Card instance.
	**/
	static bool									GetFirstDeviceFromArgument (const std::string & inArgument, CNTV2Card & outDevice);

	/**
		@brief	Compares two NTV2DeviceInfoLists and returns a list of additions and a list of removals.
		@param[in]	inOldList			Specifies the "old" list to be compared with a "newer" list.
		@param[in]	inNewList			Specifies the "new" list to be compared with the "older" list.
		@param[out]	outDevicesAdded		Receives a list of devices that exist in the "new" list that don't exist in the "old" list.
		@param[out]	outDevicesRemoved	Receives a list of devices that exist in the "old" list that don't exist in the "new" list.
		@return		True if the two lists differ in any way; otherwise false if they match.
	**/
	static bool									CompareDeviceInfoLists (const NTV2DeviceInfoList & inOldList,
																		const NTV2DeviceInfoList & inNewList,
																		NTV2DeviceInfoList & outDevicesAdded,
																		NTV2DeviceInfoList & outDevicesRemoved);

	/**
		@param[in]	inDevice			The CNTV2Card instance that's open for the device of interest.
		@return		A string containing the device name that will find the same given device using CNTV2DeviceScanner::GetFirstDeviceFromArgument.
	**/
	static std::string							GetDeviceRefName (CNTV2Card & inDevice);	//	New in SDK 16.0

	/**
		@return	True if the string contains a legal decimal number.
		@param[in]	inStr	The string to be tested.
	**/
	static bool			IsLegalDecimalNumber (const std::string & inStr, const size_t inMaxLength = 2);	//	New in SDK 16.0
	static uint64_t		IsLegalHexSerialNumber (const std::string & inStr);	//	New in SDK 16.0		//	e.g. "0x3236333331375458"
	static bool			IsHexDigit (const char inChr);	//	New in SDK 16.0
	static bool			IsDecimalDigit (const char inChr);	//	New in SDK 16.0
	static bool			IsAlphaNumeric (const char inStr);	//	New in SDK 16.0

	/**
		@return	True if the string contains letters and/or decimal digits.
		@param[in]	inStr	The string to be tested.
	**/
	static bool			IsAlphaNumeric (const std::string & inStr);	//	New in SDK 16.0

	/**
		@return	True if the string contains a legal serial number.
		@param[in]	inStr	The string to be tested.
	**/
	static bool			IsLegalSerialNumber (const std::string & inStr);	//	New in SDK 16.0

//	Instance Methods
public:
	//	Construction, Copying, Assigning
	/**
		@brief		Constructs me.
		@param[in]	inScanNow	Specifies if a scan should be made right away. Defaults to true.
								If false is specified, the client must explicitly call ScanHardware to enumerate NTV2 devices.
	**/
	explicit							CNTV2DeviceScanner (const bool inScanNow = true);
	explicit							CNTV2DeviceScanner (bool inScanNow, UWord inDeviceMask);

	/**
		@brief		Constructs me from an existing CNTV2DeviceScanner instance.
		@param[in]	inDeviceScanner		Specifies the CNTV2DeviceScanner instance to be copied.
	**/
	explicit							CNTV2DeviceScanner (const CNTV2DeviceScanner & inDeviceScanner);

	/**
		@brief		Assigns an existing CNTV2DeviceScanner instance to me.
		@param[in]	inDeviceScanner		Specifies the CNTV2DeviceScanner instance to be copied.
	**/
	virtual		CNTV2DeviceScanner &	operator = (const CNTV2DeviceScanner & inDeviceScanner);


	//	Scanning
	/**
		@brief	Re-scans the local host for connected AJA devices.
	**/
	virtual void	ScanHardware (void);
	virtual void	ScanHardware (UWord inDeviceMask);


	//	Inquiry
	/**
		@brief		Returns the number of AJA devices found on the local host.
		@return		Number of AJA devices found on the local host.
	**/
	virtual inline size_t						GetNumDevices (void) const			{ return GetDeviceInfoList ().size (); }

	/**
		@brief		Returns true if one or more AJA devices having the specified device identifier are attached and known to the host.
		@return		True if at least one AJA device having the specified device identifier is present on the host system; otherwise false.
		@param[in]	inDeviceID		Specifies the device identifier of interest.
		@param[in]	inRescan		Specifies if the host should be rescanned or not. Defaults to false.
	**/
	virtual bool								DeviceIDPresent (const NTV2DeviceID inDeviceID, const bool inRescan = false);

	/**
		@brief		Returns detailed information about the AJA device having the given zero-based index number.
		@return		True if successful; otherwise false.
		@param[in]	inDeviceIndexNumber	Specifies the AJA device to retrieve information about using a zero-based index number.
		@param[out]	outDeviceInfo		Specifies the NTV2DeviceInfo structure that will receive the device information.
		@param[in]	inRescan			Specifies if the host should be rescanned or not.
	**/
	virtual bool								GetDeviceInfo (const ULWord inDeviceIndexNumber, NTV2DeviceInfo & outDeviceInfo, const bool inRescan = false);

	/**
		@brief	Returns an NTV2DeviceInfoList that can be "walked" using standard C++ vector iteration techniques.
		@return	A non-constant reference to my NTV2DeviceInfoList.
	**/
	virtual inline NTV2DeviceInfoList &			GetDeviceInfoList (void)			{ return _deviceInfoList; }

	/**
		@brief	Returns an NTV2DeviceInfoList that can be "walked" using standard C++ vector iteration techniques.
		@return	A constant reference to my NTV2DeviceInfoList.
	**/
	virtual inline const NTV2DeviceInfoList &	GetDeviceInfoList (void) const		{ return _deviceInfoList; }


	//	Sorting
	/**
		@brief	Sorts my device list by ascending PCI slot number.
	**/
	virtual void								SortDeviceInfoList (void);

	#if !defined (NTV2_DEPRECATE)
		public:
		static NTV2_DEPRECATED_f(void								DumpBoardInfo (const NTV2DeviceInfo & info));					///< @deprecated	Use the ostream operators instead.
		static NTV2_DEPRECATED_f(void								DumpAudioFormatInfo (const NTV2AudioPhysicalFormat & audioPhysicalFormat));	///< @deprecated	Use the ostream operators instead.
		virtual NTV2_DEPRECATED_f(bool								BoardTypePresent (NTV2BoardType boardType, bool rescan = false));	///< @deprecated	NTV2BoardType is obsolete.
		virtual inline NTV2_DEPRECATED_f(bool						BoardIDPresent (NTV2BoardID boardID, bool rescan = false))	{return DeviceIDPresent (boardID, rescan);}	///< @deprecated	Use DeviceIDPresent instead.
		virtual inline NTV2_DEPRECATED_f(bool						GetBoardInfo (ULWord inDeviceIndexNumber, NTV2DeviceInfo & outDeviceInfo, bool inRescan))	{return GetDeviceInfo (inDeviceIndexNumber, outDeviceInfo, inRescan);}	///< @deprecated	Use GetDeviceInfo instead.
		virtual inline NTV2_DEPRECATED_f(void						SortBoardInfoList (void))		{ SortDeviceInfoList (); }		///< @deprecated	Use SortDeviceInfoList instead.
		virtual inline NTV2_DEPRECATED_f(size_t						GetNumBoards (void) const)		{ return GetNumDevices (); }	///< @deprecated	Use GetNumDevices instead.
		virtual inline NTV2_DEPRECATED_f(NTV2DeviceInfoList &		GetBoardList (void))			{ return _deviceInfoList; }		///< @deprecated	Use GetDeviceInfoList instead.
		virtual inline NTV2_DEPRECATED_f(const NTV2DeviceInfoList &	GetBoardList (void) const)		{ return _deviceInfoList; }		///< @deprecated	Use GetDeviceInfoList instead.
	#endif	//	NTV2_DEPRECATE

	virtual inline		~CNTV2DeviceScanner ()						{ }


private:
	virtual void	SetAudioAttributes(NTV2DeviceInfo & inDeviceInfo, CNTV2Card & inDevice) const;
	virtual void	SetVideoAttributes (NTV2DeviceInfo & inDevicInfo);
	virtual void	DeepCopy (const CNTV2DeviceScanner & inDeviceScanner);


//	Instance Data
private:
	NTV2DeviceInfoList					_deviceInfoList;		///	My device list

};	//	CNTV2DeviceScanner


#if !defined (NTV2_DEPRECATE)
	typedef NTV2_DEPRECATED_TYPEDEF		NTV2DeviceInfo				NTV2BoardInfo;				///	@deprecated		Use NTV2DeviceInfo instead. Type names containing "board" are being phased out.
	typedef NTV2_DEPRECATED_TYPEDEF		NTV2DeviceInfo				OurBoardInfo;				///	@deprecated		Use NTV2DeviceInfo instead. Type names containing "board" are being phased out.
	typedef NTV2_DEPRECATED_TYPEDEF		NTV2DeviceInfoList			NTV2BoardInfoList;			///	@deprecated		Use NTV2DeviceInfoList instead. Type names containing "board" are being phased out.
	typedef	NTV2_DEPRECATED_TYPEDEF		NTV2DeviceInfoList			BoardInfoList;				///	@deprecated		Use NTV2DeviceInfoList instead. Type names containing "board" are being phased out.
	typedef NTV2_DEPRECATED_TYPEDEF		NTV2AudioPhysicalFormat		AudioPhysicalFormat;		///	@deprecated		Use NTV2AudioPhysicalFormat instead. Type names without "NTV2" are being phased out.
	typedef NTV2_DEPRECATED_TYPEDEF		NTV2AudioPhysicalFormatList	AudioPhysicalFormatList;	///	@deprecated		Use NTV2AudioPhysicalFormatList instead. Type names without "NTV2" are being phased out.
	typedef	NTV2_DEPRECATED_TYPEDEF		CNTV2DeviceScanner			CNTV2BoardScan;				///	@deprecated		Use CNTV2DeviceScanner instead. Type names containing "board" are being phased out.
#endif	//	NTV2_DEPRECATE


#if !defined (AJADLL_BUILD) && !defined (AJASTATIC)
	//	This code forces link/load errors if the SDK client was built with NTV2_DEPRECATE defined,
	//	but the SDK lib/dylib/DLL was built without NTV2_DEPRECATE defined, ... or vice-versa...
	#if defined (NTV2_DEPRECATE)
		extern int gNTV2_DEPRECATE();
		static int __AJA_trigger_link_error_if_incompatible__ = gNTV2_DEPRECATE();
	#else
		extern int gNTV2_NON_DEPRECATE();
		static int __AJA_trigger_link_error_if_incompatible__  = gNTV2_NON_DEPRECATE();
	#endif
#endif	//	neither AJADLL_BUILD nor AJASTATIC

#endif	//	NTV2DEVICESCANNER_H
