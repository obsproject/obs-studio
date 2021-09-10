/* SPDX-License-Identifier: MIT */
/**
    @file		ntv2supportlogger.cpp
    @brief		Implementation of CNTV2SupportLogger class.
    @copyright	(C) 2017-2021 AJA Video Systems, Inc.    All rights reserved.
**/
#include "ntv2supportlogger.h"
#include "ntv2devicescanner.h"
#include "ntv2devicefeatures.h"
#include "ntv2konaflashprogram.h"
#include "ntv2registerexpert.h"
#include "ntv2registersmb.h"
#include "ntv2rp188.h"
#include "ajabase/system/info.h"
#include <algorithm>
#include <sstream>
#include <vector>
#include <iterator>

#if defined(MSWindows)
	#define	PATH_DELIMITER	"\\"
#else
	#define	PATH_DELIMITER	"/"
#endif

using namespace std;

typedef map <NTV2Channel, AUTOCIRCULATE_STATUS>     ChannelToACStatus;
typedef ChannelToACStatus::const_iterator			ChannelToACStatusConstIter;
typedef pair <NTV2Channel, AUTOCIRCULATE_STATUS>	ChannelToACStatusPair;
typedef map <uint16_t, NTV2TimeCodeList>			FrameToTCList;
typedef FrameToTCList::const_iterator				FrameToTCListConstIter;
typedef pair <uint16_t, NTV2TimeCodeList>			FrameToTCListPair;
typedef map <NTV2Channel, FrameToTCList>			ChannelToPerFrameTCList;
typedef ChannelToPerFrameTCList::const_iterator		ChannelToPerFrameTCListConstIter;
typedef pair <NTV2Channel, FrameToTCList>			ChannelToPerFrameTCListPair;

static std::string makeHeader(std::ostringstream& oss, const std::string& name)
{
    oss << setfill('=') << setw(96) << " " << name << ":" << setfill(' ') << endl << endl;
    return oss.str();
}

static std::string timecodeToString(const NTV2_RP188 & inRP188)
{
    ostringstream oss;
    if (inRP188.IsValid())
    {
        const CRP188 foo(inRP188);
        oss << foo;
    }
    else
    {
        oss << "---";
    }

    return oss.str();
}

static std::string appSignatureToString(const ULWord inAppSignature)
{
    ostringstream oss;
    const string sigStr(NTV2_4CC_AS_STRING(inAppSignature));
    if (isprint(sigStr.at(0))  &&  isprint(sigStr.at(1))  &&  isprint(sigStr.at(2))  &&  isprint(sigStr.at(3)))
        oss << "'" << sigStr << "'";
    else if (inAppSignature)
        oss << "0x" << hex << setw (8) << setfill ('0') << inAppSignature << dec << " (" << inAppSignature << ")";
    else
        oss << "'----' (0)";
    return oss.str();
}

static std::string pidToString(const uint32_t inPID)
{
    ostringstream	oss;
    #if defined (MSWindows)
        oss << inPID;
        //TCHAR	filename	[MAX_PATH];
        //HANDLE	processHandle	(OpenProcess (PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, inPID));
        //if (processHandle)
        //{
        //	if (GetModuleFileNameEx (processHandle, NULL, filename, MAX_PATH))
        //		oss << " (" << filename << ")";
        //	CloseHandle (processHandle);
        //}
    #elif defined (AJALinux)
        oss << inPID;
    #elif defined (AJAMac)
        oss << inPID;
       // char		pathbuf [PROC_PIDPATHINFO_MAXSIZE];
       // const int	rc		(::proc_pidpath (pid_t (inPID), pathbuf, sizeof (pathbuf)));
       // if (rc == 0 && ::strlen (pathbuf))
       //     oss << " (" << string (pathbuf) << ")";
    #else
        oss << inPID;
    #endif
    return oss.str();
}

//	NTV2_AUDIO_BUFFER_SIZE_8MB	8MB buffer:																							16-ch:	130,560 samples		8-ch:	261,120 samples		6-ch:	348,160 samples
//	NTV2_AUDIO_BUFFER_SIZE_4MB	4MB buffer:	0x00400000(4,194,304 bytes) - 0x00004000(16,384 bytes) = 0x003FC000(4,177,920 bytes)	16-ch:	65,280 samples		8-ch:	130,560 samples		6-ch:	174,080 samples
//	NTV2_AUDIO_BUFFER_SIZE_2MB	2MB buffer:																							16-ch:	32,640 samples		8-ch:	65,280 samples		6-ch:	87,040 samples
//	NTV2_AUDIO_BUFFER_SIZE_1MB	1MB buffer:																							16-ch:	16,320 samples		8-ch:	32,640 samples		6-ch:	43,520 samples
//	Returns the maximum number of samples for a given NTV2AudioBufferSize and maximum audio channel count...
static uint32_t maxSampleCountForNTV2AudioBufferSize (const NTV2AudioBufferSize inBufferSize, const uint16_t inChannelCount)
{												//	NTV2_AUDIO_BUFFER_SIZE_1MB	NTV2_AUDIO_BUFFER_SIZE_4MB	NTV2_AUDIO_BUFFER_SIZE_2MB	NTV2_AUDIO_BUFFER_SIZE_8MB	NTV2_AUDIO_BUFFER_INVALID
    static uint32_t	gMaxSampleCount16 []	=	{	16320,						65280,						32640,						130560,						0	};
    static uint32_t	gMaxSampleCount8 []		=	{	32640,						130560,						65280,						261120,						0	};
    static uint32_t	gMaxSampleCount6 []		=	{	87040,						174080,						87040,						348160,						0	};
    if (NTV2_IS_VALID_AUDIO_BUFFER_SIZE (inBufferSize))
        switch (inChannelCount)
        {
            case 16:	return gMaxSampleCount16 [inBufferSize];
            case 8:		return gMaxSampleCount8 [inBufferSize];
            case 6:		return gMaxSampleCount6 [inBufferSize];
            default:	break;
        }
    return 0;
}

static NTV2VideoFormat getVideoFormat (CNTV2Card & device, const NTV2Channel inChannel)
{
    NTV2VideoFormat	result(NTV2_FORMAT_UNKNOWN);
    device.GetVideoFormat(result, inChannel);
    return result;
}

static NTV2PixelFormat getPixelFormat (CNTV2Card & device, const NTV2Channel inChannel)
{
    NTV2PixelFormat	result(NTV2_FBF_INVALID);
    device.GetFrameBufferFormat(inChannel, result);
    return result;
}

static NTV2Mode getMode (CNTV2Card & device, const NTV2Channel inChannel)
{
    NTV2Mode	result(NTV2_MODE_INVALID);
    device.GetMode(inChannel, result);
    return result;
}

static bool	isEnabled (CNTV2Card & device, const NTV2Channel inChannel)
{
	bool result(false);
	device.IsChannelEnabled(inChannel, result);
	return result;
}

static string getActiveFrameStr (CNTV2Card & device, const NTV2Channel inChannel)
{
	if (!isEnabled(device, inChannel))
		return "---";
	ULWord frameNum(0);
	if (NTV2_IS_INPUT_MODE(::getMode(device, inChannel)))
		device.GetInputFrame(inChannel, frameNum);
	else
		device.GetOutputFrame(inChannel, frameNum);
	ostringstream oss;
	oss << DEC(frameNum);
    return oss.str();
}

static ULWord readCurrentAudioPosition (CNTV2Card & device, NTV2AudioSystem audioSystem, NTV2Mode mode)
{
    ULWord result(0);
    if (NTV2_IS_OUTPUT_MODE (mode))
        device.ReadAudioLastOut (result, audioSystem);	//	read head
    else
        device.ReadAudioLastIn (result, audioSystem);	//	write head
    return result;
}

static ULWord getNumAudioChannels (CNTV2Card & device, NTV2AudioSystem audioSystem)
{
    ULWord numChannels = 1;
    device.GetNumberAudioChannels(numChannels, audioSystem);
    return numChannels;
}

static ULWord bytesToSamples (CNTV2Card & device, NTV2AudioSystem audioSystem, const ULWord inBytes)
{
    return inBytes / sizeof (uint32_t) / getNumAudioChannels(device, audioSystem);
}

static ULWord getCurrentPositionSamples(CNTV2Card & device, NTV2AudioSystem audioSystem, NTV2Mode mode)
{
    ULWord bytes = readCurrentAudioPosition(device, audioSystem, mode);
    return bytesToSamples(device, audioSystem, bytes);
}

static ULWord getMaxNumSamples (CNTV2Card & device, NTV2AudioSystem audioSystem)
{
    NTV2AudioBufferSize bufferSize;
    device.GetAudioBufferSize(bufferSize, audioSystem);

    return maxSampleCountForNTV2AudioBufferSize (bufferSize, uint16_t(getNumAudioChannels(device, audioSystem)));
}

static NTV2Channel findActiveACChannel (CNTV2Card & device, NTV2AudioSystem audioSystem, AUTOCIRCULATE_STATUS & outStatus)
{
    for (UWord chan (0);  chan < ::NTV2DeviceGetNumVideoChannels (device.GetDeviceID());  chan++)
        if (device.AutoCirculateGetStatus (NTV2Channel(chan), outStatus))
            if (!outStatus.IsStopped())
                if (outStatus.GetAudioSystem() == audioSystem)
                {
                    NTV2Mode mode = NTV2_MODE_DISPLAY;
                    device.GetMode(NTV2Channel(chan), mode);
                    if ((outStatus.IsInput() && NTV2_IS_INPUT_MODE (mode))
                        ||  (outStatus.IsOutput() && NTV2_IS_OUTPUT_MODE (mode)))
                            return NTV2Channel(chan);
                }
    return NTV2_CHANNEL_INVALID;
}

static bool detectInputChannelPairs (CNTV2Card & device, const NTV2AudioSource inAudioSource,
									 const NTV2EmbeddedAudioInput inEmbeddedSource,
									 NTV2AudioChannelPairs & outChannelPairsPresent)
{
    outChannelPairsPresent.clear();
    switch (inAudioSource)
    {
        default:					return	false;

        case NTV2_AUDIO_EMBEDDED:	return	NTV2_IS_VALID_EMBEDDED_AUDIO_INPUT (inEmbeddedSource)
                                                //	Input detection is based on the audio de-embedder (as opposed to the SDI spigot)...
                                                ?	device.GetDetectedAudioChannelPairs (NTV2AudioSystem(inEmbeddedSource), outChannelPairsPresent)
                                                :	false;

        case NTV2_AUDIO_AES:		return	device.GetDetectedAESChannelPairs (outChannelPairsPresent);

        case NTV2_AUDIO_ANALOG:		if (NTV2_IS_VALID_VIDEO_FORMAT(device.GetAnalogInputVideoFormat())
                                        || NTV2_IS_VALID_VIDEO_FORMAT(device.GetAnalogCompositeInputVideoFormat()))
                                            {outChannelPairsPresent.insert(NTV2_AudioChannel1_2);	return true;}	//	Assume chls 1&2 if an analog signal present
                                    break;

        case NTV2_AUDIO_HDMI:		if (NTV2_IS_VALID_VIDEO_FORMAT(device.GetHDMIInputVideoFormat()))
                                    {
                                        NTV2HDMIAudioChannels	hdmiChls	(NTV2_INVALID_HDMI_AUDIO_CHANNELS);
                                        if (!device.GetHDMIInputAudioChannels (hdmiChls))
                                            return false;
                                        for (NTV2AudioChannelPair chPair (NTV2_AudioChannel1_2);
                                             chPair < ((hdmiChls == NTV2_HDMIAudio8Channels)  ?  NTV2_AudioChannel9_10  :  NTV2_AudioChannel3_4);
                                             chPair = NTV2AudioChannelPair(chPair + 1))
                                                    outChannelPairsPresent.insert (chPair);
                                        return true;
                                    }
                                    break;
    }
    return false;
}

static bool getBitfileDate (CNTV2Card & device, std::string &bitFileDateString, NTV2XilinxFPGA whichFPGA)
{
    BITFILE_INFO_STRUCT bitFileInfo;
    memset(&bitFileInfo, 0, sizeof(BITFILE_INFO_STRUCT));
    bitFileInfo.whichFPGA = whichFPGA;
    bool bBitFileInfoAvailable = false;     // BitFileInfo is implemented only on 5.2 and later drivers.
    if (true)	//	boardID != BOARD_ID_XENAX &&  boardID != BOARD_ID_XENAX2 )
        bBitFileInfoAvailable = device.DriverGetBitFileInformation(bitFileInfo);
    if( bBitFileInfoAvailable )
    {
        bitFileDateString = bitFileInfo.designNameStr;
        if (bitFileDateString.find(".ncd") > 0)
        {
            bitFileDateString = bitFileDateString.substr(0, bitFileDateString.find(".ncd"));
            bitFileDateString += ".bit ";
            bitFileDateString += bitFileInfo.dateStr;
            bitFileDateString += " ";
            bitFileDateString += bitFileInfo.timeStr;
        }
        else if (bitFileDateString.find(";") != string::npos)
        {
            bitFileDateString = bitFileDateString.substr(0, bitFileDateString.find(";"));
            bitFileDateString += ".bit ";
            bitFileDateString += bitFileInfo.dateStr;
            bitFileDateString += " ";
            bitFileDateString += bitFileInfo.timeStr;
        }
        else if (bitFileDateString.find(".bit") != string::npos &&
                 bitFileDateString != ".bit")
        {
            bitFileDateString = bitFileInfo.designNameStr;
            bitFileDateString += " ";
            bitFileDateString += bitFileInfo.dateStr;
            bitFileDateString += " ";
            bitFileDateString += bitFileInfo.timeStr;
        }
        else
        {
            bitFileDateString = "bad bitfile date string";
            return false;
        }
    }
    else
        return false;
    return true;
}

AJAExport std::ostream & operator << (std::ostream & outStream, const CNTV2SupportLogger & inData)
{
    outStream << inData.ToString();
    return outStream;
}

CNTV2SupportLogger::CNTV2SupportLogger (CNTV2Card & card, NTV2SupportLoggerSections sections)
    :	mDevice		(card),
		mDispose	(false),
		mSections	(sections)
{
}

CNTV2SupportLogger::CNTV2SupportLogger (UWord cardIndex, NTV2SupportLoggerSections sections)
    :	mDevice		(*(new CNTV2Card(cardIndex))),
		mDispose	(true),
		mSections	(sections)
{
}

CNTV2SupportLogger::~CNTV2SupportLogger()
{
	if (mDispose)
		delete &mDevice;
}

int CNTV2SupportLogger::Version (void)
{
    // Bump this whenever the formatting of the support log changes drastically
    return 2;
}

void CNTV2SupportLogger::PrependToSection (uint32_t section, const string & sectionData)
{
    if (mPrependMap.find(section) != mPrependMap.end())
    {
        mPrependMap.at(section).insert(0, "\n");
        mPrependMap.at(section).insert(0, sectionData);
    }
    else
    {
        mPrependMap[section] = sectionData;
        mPrependMap.at(section).append("\n");
    }
}

void CNTV2SupportLogger::AppendToSection (uint32_t section, const string & sectionData)
{
    if (mAppendMap.find(section) != mAppendMap.end())
    {
        mAppendMap.at(section).append("\n");
        mAppendMap.at(section).append(sectionData);
    }
    else
    {
        mAppendMap[section] = "\n";
        mAppendMap.at(section).append(sectionData);
    }
}

void CNTV2SupportLogger::AddHeader (const string & sectionName, const string & sectionData)
{
    ostringstream oss;
    makeHeader(oss, sectionName);
    oss << sectionData << "\n";
    mHeaderStr.append(oss.str());
}

void CNTV2SupportLogger::AddFooter (const string & sectionName, const string & sectionData)
{
    ostringstream oss;
    makeHeader(oss, sectionName);
    oss << sectionData << "\n";
    mFooterStr.append(oss.str());
}

// Use this macro to handle generating text for each section
// - the header
// - the prepend if any
// - the method that fills the section
// - the append if any
#define LoggerSectionToFunctionMacro(_SectionEnum_, _SectionString_, _SectionMethod_) \
    if (mSections & _SectionEnum_) \
    { \
        makeHeader(oss, _SectionString_); \
        if (mPrependMap.find(_SectionEnum_) != mPrependMap.end()) \
            oss << mPrependMap.at(_SectionEnum_); \
        \
        _SectionMethod_(oss); \
        \
        if (mAppendMap.find(_SectionEnum_) != mAppendMap.end()) \
            oss << mAppendMap.at(_SectionEnum_); \
    }

std::string CNTV2SupportLogger::ToString (void) const
{
	ostringstream oss;
	vector<char> dateBufferLocal(128, 0);
	vector<char> dateBufferUTC(128, 0);

	// get the wall time and format it
	time_t now = time(AJA_NULL);
	struct tm *localTimeinfo;
	localTimeinfo = localtime(reinterpret_cast<const time_t*>(&now));
	strcpy(&dateBufferLocal[0], "");
	if (localTimeinfo)
		::strftime(&dateBufferLocal[0], dateBufferLocal.size(), "%B %d, %Y %I:%M:%S %p %Z (local)", localTimeinfo);

	struct tm *utcTimeinfo;
	utcTimeinfo = gmtime(reinterpret_cast<const time_t*>(&now));
	strcpy(&dateBufferUTC[0], "");
	if (utcTimeinfo)
		::strftime(&dateBufferUTC[0], dateBufferUTC.size(), "%Y-%m-%dT%H:%M:%SZ UTC", utcTimeinfo);

	oss << "Begin NTV2 Support Log" << "\n" <<
		   "Version: "   << CNTV2SupportLogger::Version() << "\n"
		   "Generated: " << &dateBufferLocal[0] <<
		   "           " << &dateBufferUTC[0] << "\n\n" << flush;

	if (!mHeaderStr.empty())
		oss << mHeaderStr;

	// Go ahead and show info even if the device is not open
	LoggerSectionToFunctionMacro(NTV2_SupportLoggerSectionInfo, "Info", FetchInfoLog)

	if (mDevice.IsOpen())
	{
		LoggerSectionToFunctionMacro(NTV2_SupportLoggerSectionAutoCirculate, "AutoCirculate", FetchAutoCirculateLog)
		LoggerSectionToFunctionMacro(NTV2_SupportLoggerSectionAudio, "Audio", FetchAudioLog)
		LoggerSectionToFunctionMacro(NTV2_SupportLoggerSectionRouting, "Routing", FetchRoutingLog)
		LoggerSectionToFunctionMacro(NTV2_SupportLoggerSectionRegisters, "Regs", FetchRegisterLog)
	}

	if (!mFooterStr.empty())
		oss << mFooterStr;
	oss << endl << "End NTV2 Support Log";
	return oss.str();
}

void CNTV2SupportLogger::ToString (string & outString) const
{
	outString = ToString();
}

static inline std::string HEX0NStr(const uint32_t inNum, const uint16_t inWidth)	{ostringstream	oss;  oss << HEX0N(inNum,inWidth);  return oss.str();}
static inline std::string xHEX0NStr(const uint32_t inNum, const uint16_t inWidth)	{ostringstream	oss;  oss << xHEX0N(inNum,inWidth);  return oss.str();}
template <class T> std::string DECStr (const T & inT)								{ostringstream	oss;  oss << DEC(inT);  return oss.str();}

void CNTV2SupportLogger::FetchInfoLog (ostringstream & oss) const
{
	string str;
	AJALabelValuePairs	infoTable;
	AJASystemInfo::append(infoTable, "SDK/DRIVER INFO",	"");
	AJASystemInfo::append(infoTable, "NTV2 SDK Version",	::NTV2GetVersionString(true));
	AJASystemInfo::append(infoTable, "supportlog Built",	std::string(__DATE__ " at " __TIME__));
	AJASystemInfo::append(infoTable, "Driver Version",		mDevice.GetDriverVersionString());
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	const ULWord negotiatedProtocolVersion (mDevice.GetNubProtocolVersion());
	AJASystemInfo::append(infoTable, "Watcher Nub Protocol Version",	DECStr(maxKnownProtocolVersion));
	AJASystemInfo::append(infoTable, "Negotiated Nub Protocol Version",	negotiatedProtocolVersion ? DECStr(negotiatedProtocolVersion) : "Not available");
	if (negotiatedProtocolVersion >= ntv2NubProtocolVersion3)
	{
		BUILD_INFO_STRUCT buildInfo;
		if (mDevice.DriverGetBuildInformation(buildInfo))
			AJASystemInfo::append(infoTable, "Driver Build Version",	buildInfo.buildStr);
	}
#else
	AJASystemInfo::append(infoTable, "Watcher Nub Protocol Version",	"N/A (nub client support missing)");
#endif	//	NTV2_NUB_CLIENT_SUPPORT

	AJASystemInfo::append(infoTable, "DEVICE INFO",	"");
	AJASystemInfo::append(infoTable, "Device",				mDevice.GetDisplayName());
	str = xHEX0NStr(mDevice.GetDeviceID(),8) + " (" + string(::NTV2DeviceIDString(mDevice.GetDeviceID())) + ")";
	AJASystemInfo::append(infoTable, "Device ID",			str);
	AJASystemInfo::append(infoTable, "Serial Number",		(mDevice.GetSerialNumberString(str) ? str : "Not programmed"));
	AJASystemInfo::append(infoTable, "Video Bitfile",		(getBitfileDate(mDevice, str, eFPGAVideoProc) ? str : "Not available"));
	AJASystemInfo::append(infoTable, "PCI FPGA Version",	mDevice.GetPCIFPGAVersionString());
	ULWord	numBytes(0);	string	dateStr, timeStr;
	if (mDevice.GetInstalledBitfileInfo(numBytes, dateStr, timeStr))
	{
		AJASystemInfo::append(infoTable, "Installed Bitfile ByteCount",	DECStr(numBytes));
		AJASystemInfo::append(infoTable, "Installed Bitfile Build Date",	dateStr + " " + timeStr);
	}

	if (mDevice.IsIPDevice())
	{
		PACKAGE_INFO_STRUCT pkgInfo;
		if (mDevice.GetPackageInformation(pkgInfo))
		{
			AJASystemInfo::append(infoTable, "Package",		DECStr(pkgInfo.packageNumber));
			AJASystemInfo::append(infoTable, "Build",		DECStr(pkgInfo.buildNumber));
			AJASystemInfo::append(infoTable, "Build Date",	pkgInfo.date);
			AJASystemInfo::append(infoTable, "Build Time",	pkgInfo.time);
		}

		CNTV2KonaFlashProgram ntv2Card(mDevice.GetIndexNumber());
		MacAddr mac1, mac2;
		if (ntv2Card.ReadMACAddresses(mac1, mac2))
		{
			AJASystemInfo::append(infoTable, "MAC1",	mac1.AsString());
			AJASystemInfo::append(infoTable, "MAC2",	mac2.AsString());
		}

		ULWord cfg(0);
		mDevice.ReadRegister((kRegSarekFwCfg + SAREK_REGS), cfg);
		if (cfg & SAREK_2022_2)
		{
			ULWord dnaLo(0), dnaHi(0);
			if (ntv2Card.ReadRegister(kRegSarekDNALow + SAREK_REGS, dnaLo))
				if (ntv2Card.ReadRegister(kRegSarekDNAHi + SAREK_REGS, dnaHi))
					AJASystemInfo::append(infoTable, "Device DNA",	string(HEX0NStr(dnaHi,8)+HEX0NStr(dnaLo,8)));
		}

		string licenseInfo;
		ntv2Card.ReadLicenseInfo(licenseInfo);
		AJASystemInfo::append(infoTable, "License",	licenseInfo);

		if (cfg & SAREK_2022_2)
		{
			ULWord licenseStatus(0);
			ntv2Card.ReadRegister(kRegSarekLicenseStatus + SAREK_REGS, licenseStatus);
			AJASystemInfo::append(infoTable, "License Present",	licenseStatus & SAREK_LICENSE_PRESENT ? "Yes" : "No");
			AJASystemInfo::append(infoTable, "License Status",	licenseStatus & SAREK_LICENSE_VALID ? "License is valid" : "License NOT valid");
			AJASystemInfo::append(infoTable, "License Enable Mask",	xHEX0NStr(licenseStatus & 0xff,2));
		}
	}

	//	Host info
	AJASystemInfo	hostInfo;
	oss << AJASystemInfo::ToString(infoTable) << endl
		<< hostInfo.ToString() << endl;
}

void CNTV2SupportLogger::FetchRegisterLog (ostringstream & oss) const
{
	NTV2RegisterReads	regs;
	const NTV2DeviceID	deviceID	(mDevice.GetDeviceID());
	NTV2RegNumSet		deviceRegs	(CNTV2RegisterExpert::GetRegistersForDevice (deviceID));
	const NTV2RegNumSet	virtualRegs	(CNTV2RegisterExpert::GetRegistersForClass (kRegClass_Virtual));
	static const string	sDashes		(96, '-');

	//	Dang, GetRegistersForDevice doesn't/can't read kRegCanDoRegister, so add the CanConnectROM regs here...
	if (mDevice.HasCanConnectROM())
		for (ULWord regNum(kRegFirstValidXptROMRegister);  regNum < ULWord(kRegInvalidValidXptROMRegister);  regNum++)
			deviceRegs.insert(regNum);

	oss	<< endl << deviceRegs.size() << " Device Registers " << sDashes << endl << endl;
	regs = ::FromRegNumSet (deviceRegs);
	if (!mDevice.ReadRegisters (regs))
		oss	<< "## NOTE:  One or more of these registers weren't returned by the driver -- these will have a zero value." << endl;
	for (NTV2RegisterReadsConstIter it (regs.begin());  it != regs.end();  ++it)
	{
		const NTV2RegInfo &	regInfo	(*it);
		const uint32_t		regNum	(regInfo.registerNumber);
		//const uint32_t	offset	(regInfo.registerNumber * 4);
		const uint32_t		value	(regInfo.registerValue);
		oss	<< endl
			<< "Register Name: " << CNTV2RegisterExpert::GetDisplayName(regNum) << endl
			<< "Register Number: " << regNum << endl
			<< "Register Value: " << value << " : " << xHEX0N(value,8) << endl
		//	<< "Register Classes: " << CNTV2RegisterExpert::GetRegisterClasses(regNum) << endl
			<< CNTV2RegisterExpert::GetDisplayValue	(regNum, value, deviceID) << endl;
	}

	regs = ::FromRegNumSet (virtualRegs);
	oss	<< endl << virtualRegs.size() << " Virtual Registers " << sDashes << endl << endl;
	if (!mDevice.ReadRegisters (regs))
		oss	<< "## NOTE:  One or more of these registers weren't returned by the driver -- these will have a zero value." << endl;
	for (NTV2RegisterReadsConstIter it (regs.begin());  it != regs.end();  ++it)
	{
		const NTV2RegInfo &	regInfo	(*it);
		const uint32_t		regNum	(regInfo.registerNumber);
		//const uint32_t	offset	(regInfo.registerNumber * 4);
		const uint32_t		value	(regInfo.registerValue);
		oss	<< endl
			<< "VReg Name: " << CNTV2RegisterExpert::GetDisplayName(regNum) << endl
			<< "VReg Number: " << setw(10) << regNum << endl
			<< "VReg Value: " << value << " : " << xHEX0N(value,8) << endl
			<< CNTV2RegisterExpert::GetDisplayValue	(regNum, value, deviceID)	<< endl;
	}
}

void CNTV2SupportLogger::FetchAutoCirculateLog (ostringstream & oss) const
{
	ULWord					appSignature	(0);
	int32_t					appPID			(0);
	ChannelToACStatus		perChannelStatus;	//	Per-channel AUTOCIRCULATE_STATUS
	ChannelToPerFrameTCList	perChannelTCs;		//	Per-channel collection of per-frame TCs
	NTV2EveryFrameTaskMode	taskMode	(NTV2_DISABLE_TASKS);
	static const string		dashes		(25, '-');
	const ULWord			numChannels	(::NTV2DeviceGetNumVideoChannels(mDevice.GetDeviceID()));

	//	This code block takes a snapshot of the current AutoCirculate state of the device...
	mDevice.GetEveryFrameServices(taskMode);
	mDevice.GetStreamingApplication(appSignature, appPID);

	//	Grab A/C status for each channel...
	for (NTV2Channel chan(NTV2_CHANNEL1);  chan < NTV2Channel(numChannels);  chan = NTV2Channel(chan+1))
	{
		FrameToTCList			perFrameTCs;
		AUTOCIRCULATE_STATUS	acStatus;
		if (NTV2_IS_INPUT_CROSSPOINT(acStatus.acCrosspoint))
			mDevice.WaitForInputVerticalInterrupt(chan);
		else
			mDevice.WaitForOutputVerticalInterrupt(chan);
		mDevice.AutoCirculateGetStatus (chan, acStatus);
		perChannelStatus.insert(ChannelToACStatusPair(chan, acStatus));
		if (!acStatus.IsStopped())
		{
			for (uint16_t frameNum (acStatus.GetStartFrame());  frameNum <= acStatus.GetEndFrame();  frameNum++)
			{
				FRAME_STAMP			frameStamp;
				NTV2TimeCodeList	timecodes;
				mDevice.AutoCirculateGetFrameStamp (chan, frameNum, frameStamp);
				frameStamp.GetInputTimeCodes(timecodes);
				perFrameTCs.insert(FrameToTCListPair(frameNum, timecodes));
			}	//	for each A/C frame
			perChannelTCs.insert(ChannelToPerFrameTCListPair(chan, perFrameTCs));
		}	//	if not stopped
	}	//	for each channel

	oss	<< "Task mode:  " << ::NTV2TaskModeToString(taskMode) << ", PID=" << pidToString(uint32_t(appPID)) << ", signature=" << appSignatureToString(appSignature) << endl
		<< endl
		<< "Chan/FrameStore   State  Start   End   Act   FrmProc   FrmDrop BufLvl    Audio   RP188     LTC   FBFch   FBOch   Color   VidPr     Anc   HDMIx   Field      VidFmt       PixFmt" << endl
		<< "-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------" << endl;
	for (ChannelToACStatusConstIter iter (perChannelStatus.begin());  iter != perChannelStatus.end();  ++iter)
	{
		const NTV2Channel				chan(iter->first);
		const AUTOCIRCULATE_STATUS	&	status(iter->second);
		//	The following should mirror what ntv2watcher/pages/page_autocirculate::fetchSupportLogInfo does...
			oss << ::NTV2ChannelToString(chan, true) << ": "
				<< (::isEnabled(mDevice,chan) ? NTV2_IS_INPUT_MODE(::getMode(mDevice,chan)) ? "Input " : "Output" : "Off   ")
				<< setw(12) << status[0]	//	State
				<< setw( 7) << status[1]	//	Start
				<< setw( 6) << status[2]	//	End
				<< setw( 6) << (status.IsStopped() ? ::getActiveFrameStr(mDevice,chan) : status[4])	//	Act
				<< setw(10) << status[9]	//	FrmProc
				<< setw(10) << status[10]	//	FrmDrop
				<< setw( 7) << status[11]	//	BufLvl
				<< setw( 9) << status[12]	//	Audio
				<< setw( 8) << status[13]	//	RP188
				<< setw( 8) << status[14]	//	LTC
				<< setw( 8) << status[15]	//	FBFchg
				<< setw( 8) << status[16]	//	FBOchg
				<< setw( 8) << status[17]	//	ColCor
				<< setw( 8) << status[18]	//	VidProc
				<< setw( 8) << status[19]	//	Anc
				<< setw( 8) << status[20]	//	HDMIAux
				<< setw( 8) << status[21];	//	Fld
		if (!status.IsStopped() || isEnabled(mDevice,chan))
			oss	<< setw(12) << ::NTV2VideoFormatToString(::getVideoFormat(mDevice, chan))
				<< setw(13) << ::NTV2FrameBufferFormatToString(::getPixelFormat(mDevice, chan), true)
				<< endl;
		else
			oss	<< setw(12) << "---"
				<< setw(13) << "---"
				<< endl;
	}	//	for each channel

	for (ChannelToACStatusConstIter iter (perChannelStatus.begin());  iter != perChannelStatus.end();  ++iter)
	{
		const NTV2Channel				chan(iter->first);
		const AUTOCIRCULATE_STATUS &	status(iter->second);
		if (status.IsStopped())
			continue;	//	Stopped -- skip this channel

		ChannelToPerFrameTCListConstIter it(perChannelTCs.find(chan));
		if (it == perChannelTCs.end())
			continue;	//	Channel not in perChannelTCs

		const FrameToTCList	perFrameTCs(it->second);
		oss << endl << dashes << " " << (NTV2_IS_INPUT_CROSSPOINT(status.acCrosspoint) ? "Input " : "Output ") << (chan+1) << " Per-Frame Timecodes:" << endl;
		for (FrameToTCListConstIter i(perFrameTCs.begin());  i != perFrameTCs.end();  ++i)
		{
			const uint16_t				frameNum(i->first);
			const NTV2TimeCodeList &	timecodes(i->second);
			oss << "Frame " << frameNum << ":" << endl;
			for (uint16_t tcNdx(0);  tcNdx < timecodes.size();  tcNdx++)
			{
				const NTV2TimecodeIndex	tcIndex	(static_cast<NTV2TimecodeIndex>(tcNdx));
				oss << "\t" << setw(10) << ::NTV2TCIndexToString(tcIndex, true) << setw(0) << ":\t"
					<< setw(12) << timecodeToString(timecodes[tcNdx]) << setw(0) << "\t" << timecodes[tcNdx] << endl;
			}	//	for each timecode
		}	//	for each frame
	}	//	for each channel
}	//	FetchAutoCirculateLog

void CNTV2SupportLogger::FetchAudioLog (ostringstream & oss) const
{

    const UWord		maxNumChannels		(::NTV2DeviceGetMaxAudioChannels(mDevice.GetDeviceID()));
    oss     << "             Device:\t" << mDevice.GetDisplayName ()											<< endl;

    // loop over all the audio systems
    for (int i=0; i<NTV2DeviceGetNumAudioSystems(mDevice.GetDeviceID()); i++)
    {
        //temp stubs
        // need to determin channel and audio system still
        //
		NTV2AudioSystem audioSystem = NTV2AudioSystem(i);

        AUTOCIRCULATE_STATUS acStatus;
        NTV2Channel channel = findActiveACChannel(mDevice, audioSystem, acStatus);
        if (channel != NTV2_CHANNEL_INVALID)
        {
            NTV2AudioSource audioSource = NTV2_AUDIO_EMBEDDED;
            NTV2EmbeddedAudioInput embeddedSource = NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_1;
            mDevice.GetAudioSystemInputSource(audioSystem, audioSource, embeddedSource);
            NTV2Mode mode = NTV2_MODE_DISPLAY;
            mDevice.GetMode(channel, mode);
            NTV2AudioRate audioRate = NTV2_AUDIO_48K;
            mDevice.GetAudioRate(audioRate, audioSystem);
            NTV2AudioBufferSize audioBufferSize;
            mDevice.GetAudioBufferSize(audioBufferSize, audioSystem);
            NTV2AudioLoopBack loopbackMode = NTV2_AUDIO_LOOPBACK_OFF;
            mDevice.GetAudioLoopBack(loopbackMode, audioSystem);

            NTV2AudioChannelPairs channelPairsPresent;
            if (NTV2_IS_INPUT_MODE(mode))
            {
                detectInputChannelPairs(mDevice, audioSource, embeddedSource, channelPairsPresent);
            }
            else if (NTV2_IS_OUTPUT_MODE(mode))
            {
                bool isEmbedderEnabled = false;
                mDevice.GetAudioOutputEmbedderState(NTV2Channel(audioSystem), isEmbedderEnabled);
                UWord inChannelCount = isEmbedderEnabled ? maxNumChannels : 0;

                //	Generates a NTV2AudioChannelPairs set for the given number of audio channels...
                for (UWord audioChannel (0);  audioChannel < inChannelCount;  audioChannel++)
                {
                    if (audioChannel & 1)
                        continue;
                    channelPairsPresent.insert(NTV2AudioChannelPair(audioChannel/2));
                }
            }

            if (::NTV2DeviceCanDoPCMDetection(mDevice.GetDeviceID()))
                mDevice.GetInputAudioChannelPairsWithPCM(channel, channelPairsPresent);

            NTV2AudioChannelPairs nonPCMChannelPairs;
            mDevice.GetInputAudioChannelPairsWithoutPCM(channel, nonPCMChannelPairs);
            bool isNonPCM = true;
            //end temp

            const ULWord	currentPosSampleNdx	(getCurrentPositionSamples(mDevice, audioSystem, mode));
            const ULWord	maxSamples			(getMaxNumSamples(mDevice, audioSystem));

            oss																										<< endl
            //        << "             Device:\t" << mDevice.GetDisplayName ()											<< endl
                    << "       Audio system:\t" << ::NTV2AudioSystemToString (audioSystem, true)						<< endl
                    << "        Sample Rate:\t" << ::NTV2AudioRateToString (audioRate, true)							<< endl
                    << "        Buffer Size:\t" << ::NTV2AudioBufferSizeToString (audioBufferSize, true)				<< endl
                    << "     Audio Channels:\t" << getNumAudioChannels(mDevice, audioSystem);
                    if (getNumAudioChannels(mDevice, audioSystem) == maxNumChannels)
                        oss << " (max)"										<< endl;
                    else
                        oss << " (" << maxNumChannels << " (max))"			<< endl;
            oss	<< "      Total Samples:\t[" << DEC0N(maxSamples,6) << "]"											<< endl
                    << "          Direction:\t" << ::NTV2ModeToString (mode, true)									<< endl
    //                << "       Engine State:\t" << (isAudioEngineRunning() ? (isAudioEnginePaused() ? "Paused" : "Running") : "Stopped") << endl
                    << "      AutoCirculate:\t" << ::NTV2ChannelToString (channel, true)								<< endl
                    << "      Loopback Mode:\t" << ::NTV2AudioLoopBackToString (loopbackMode, true)					<< endl;
            if (NTV2_IS_INPUT_MODE(mode))
            {
                oss	<< "Write Head Position:\t["	<< DEC0N(currentPosSampleNdx,6) << "]"				<< endl
                        << "       Audio source:\t"		<< ::NTV2AudioSourceToString(audioSource, true);
                if (NTV2_AUDIO_SOURCE_IS_EMBEDDED(audioSource))
                    oss	<< " (" << ::NTV2EmbeddedAudioInputToString(embeddedSource, true) << ")";
                oss																						<< endl
                        << "   Channels Present:\t"		<< channelPairsPresent								<< endl
                        << "   Non-PCM Channels:\t"		<< nonPCMChannelPairs								<< endl;
            }
            else if (NTV2_IS_OUTPUT_MODE(mode))
            {
                oss	<< " Read Head Position:\t[" << DEC0N(currentPosSampleNdx,6) << "]"					<< endl;
                if (::NTV2DeviceCanDoPCMControl(mDevice.GetDeviceID()))
                    oss	<< "   Non-PCM Channels:\t" << nonPCMChannelPairs								<< endl;
                else
                    oss	<< "   Non-PCM Channels:\t" << (isNonPCM ? "All Channel Pairs" : "Normal")		<< endl;
            }
        }
    }
}

void CNTV2SupportLogger::FetchRoutingLog (ostringstream & oss) const
{
    //	Dump routing info...
    CNTV2SignalRouter	router;
    mDevice.GetRouting (router);
    oss << "(NTV2InputCrosspointID <== NTV2OutputCrosspointID)" << endl;
    router.Print (oss, false);
/**
    //	Dump routing registers...
    NTV2RegNumSet		deviceRoutingRegs;
    const NTV2RegNumSet	routingRegs	(CNTV2RegisterExpert::GetRegistersForClass (kRegClass_Routing));
    const NTV2RegNumSet	deviceRegs	(CNTV2RegisterExpert::GetRegistersForDevice (mDevice.GetDeviceID()));
    //	Get the intersection of the deviceRegs|routingRegs sets...
    set_intersection (routingRegs.begin(), routingRegs.end(),  deviceRegs.begin(), deviceRegs.end(),  std::inserter(deviceRoutingRegs, deviceRoutingRegs.begin()));
    NTV2RegisterReads	regsToRead	(::FromRegNumSet (deviceRoutingRegs));
    mDevice.ReadRegisters (regsToRead);	//	Read the routing regs
    oss << endl
        << deviceRoutingRegs.size() << " Routing Registers:" << endl << regsToRead << endl;
**/
}

struct registerToLoadString
{
	NTV2RegisterNumber registerNum;
	std::string registerStr;
};
const registerToLoadString registerToLoadStrings[] =
{
	Enum2Str(kRegGlobalControl)
	Enum2Str(kRegCh1Control)
	Enum2Str(kRegCh2Control)
	Enum2Str(kRegVidProcXptControl)
	Enum2Str(kRegMixer1Coefficient)
	Enum2Str(kRegSplitControl)
	Enum2Str(kRegFlatMatteValue)
	Enum2Str(kRegOutputTimingControl)
	Enum2Str(kRegAud1Delay)
	Enum2Str(kRegAud1Control)
	Enum2Str(kRegAud1SourceSelect)
	Enum2Str(kRegAud2Delay)
	Enum2Str(kRegGlobalControl3)
	Enum2Str(kRegXptSelectGroup1)
	Enum2Str(kRegXptSelectGroup2)
	Enum2Str(kRegXptSelectGroup3)
	Enum2Str(kRegXptSelectGroup4)
	Enum2Str(kRegXptSelectGroup5)
	Enum2Str(kRegXptSelectGroup6)
	Enum2Str(kRegXptSelectGroup7)
	Enum2Str(kRegXptSelectGroup8)
	Enum2Str(kRegCh1ControlExtended)
	Enum2Str(kRegCh2ControlExtended)
	Enum2Str(kRegXptSelectGroup11)
	Enum2Str(kRegXptSelectGroup12)
	Enum2Str(kRegAud2Control)
	Enum2Str(kRegAud2SourceSelect)
	Enum2Str(kRegXptSelectGroup9)
	Enum2Str(kRegXptSelectGroup10)
	Enum2Str(kRegSDITransmitControl)
	Enum2Str(kRegCh3Control)
	Enum2Str(kRegCh4Control)
	Enum2Str(kRegXptSelectGroup13)
	Enum2Str(kRegXptSelectGroup14)
	Enum2Str(kRegGlobalControl2)
	Enum2Str(kRegAud3Control)
	Enum2Str(kRegAud4Control)
	Enum2Str(kRegAud3SourceSelect)
	Enum2Str(kRegAud4SourceSelect)
	Enum2Str(kRegXptSelectGroup17)
	Enum2Str(kRegXptSelectGroup15)
	Enum2Str(kRegXptSelectGroup16)
	Enum2Str(kRegAud3Delay)
	Enum2Str(kRegAud4Delay)
	Enum2Str(kRegXptSelectGroup18)
	Enum2Str(kRegXptSelectGroup19)
	Enum2Str(kRegXptSelectGroup20)
	Enum2Str(kRegGlobalControlCh2)
	Enum2Str(kRegGlobalControlCh3)
	Enum2Str(kRegGlobalControlCh4)
	Enum2Str(kRegGlobalControlCh5)
	Enum2Str(kRegGlobalControlCh6)
	Enum2Str(kRegGlobalControlCh7)
	Enum2Str(kRegGlobalControlCh8)
	Enum2Str(kRegCh5Control)
	Enum2Str(kRegCh6Control)
	Enum2Str(kRegCh7Control)
	Enum2Str(kRegCh8Control)
	Enum2Str(kRegXptSelectGroup21)
	Enum2Str(kRegXptSelectGroup22)
	Enum2Str(kRegXptSelectGroup30)
	Enum2Str(kRegXptSelectGroup23)
	Enum2Str(kRegXptSelectGroup24)
	Enum2Str(kRegXptSelectGroup25)
	Enum2Str(kRegXptSelectGroup26)
	Enum2Str(kRegXptSelectGroup27)
	Enum2Str(kRegXptSelectGroup28)
	Enum2Str(kRegXptSelectGroup29)
	Enum2Str(kRegXptSelectGroup31)
	Enum2Str(kRegAud5Control)
	Enum2Str(kRegAud5SourceSelect)
	Enum2Str(kRegAud6Control)
	Enum2Str(kRegAud6SourceSelect)
	Enum2Str(kRegAud7Control)
	Enum2Str(kRegAud7SourceSelect)
	Enum2Str(kRegAud8Control)
	Enum2Str(kRegAud8SourceSelect)
	Enum2Str(kRegOutputTimingControlch2)
	Enum2Str(kRegOutputTimingControlch3)
	Enum2Str(kRegOutputTimingControlch4)
	Enum2Str(kRegOutputTimingControlch5)
	Enum2Str(kRegOutputTimingControlch6)
	Enum2Str(kRegOutputTimingControlch7)
	Enum2Str(kRegOutputTimingControlch8)
	Enum2Str(kRegVidProc3Control)
	Enum2Str(kRegMixer3Coefficient)
	Enum2Str(kRegFlatMatte3Value)
	Enum2Str(kRegVidProc4Control)
	Enum2Str(kRegMixer4Coefficient)
	Enum2Str(kRegFlatMatte4Value)
	Enum2Str(kRegAud5Delay)
	Enum2Str(kRegAud6Delay)
	Enum2Str(kRegAud7Delay)
	Enum2Str(kRegAud8Delay)
	Enum2Str(kRegXptSelectGroup32)
	Enum2Str(kRegXptSelectGroup33)
	Enum2Str(kRegXptSelectGroup34)
	Enum2Str(kRegXptSelectGroup35)
};

bool CNTV2SupportLogger::LoadFromLog (const std::string & inLogFilePath, const bool bForceLoad)
{
	ifstream fileInput;
	fileInput.open(inLogFilePath.c_str());
	string lineContents;
	int i = 0, numLines = 0;;
	int size = sizeof(registerToLoadStrings)/sizeof(registerToLoadString);
	string searchString;
	bool isCompatible = false;

	while(getline(fileInput, lineContents))
		numLines++;
	if(size > numLines)
		return false;
	fileInput.clear();
	fileInput.seekg(0, ios::beg);
	while(getline(fileInput, lineContents) && i < size && !bForceLoad)
	{
		searchString = "Device: ";
		searchString.append(NTV2DeviceIDToString(mDevice.GetDeviceID()));
		if (lineContents.find(searchString, 0) != string::npos)
		{
			cout << NTV2DeviceIDToString(mDevice.GetDeviceID()) << " is compatible with the log." << endl;
			isCompatible = true;
			break;
		}
		else
		{
			continue;
		}
	}

	if(!isCompatible)
		return false;

	while(i < size)
	{
		getline(fileInput, lineContents);
		if(fileInput.eof())
		{
			//Did not find the register reset stream to begin
			fileInput.clear();
			fileInput.seekg(0, ios::beg);
			i++;
			continue;
		}
		searchString = "Register Name: ";
		searchString.append(registerToLoadStrings[i].registerStr);
		if (lineContents.find(searchString, 0) != string::npos)
		{
			getline(fileInput, lineContents);
			getline(fileInput, lineContents);
			searchString = "Register Value: ";
			std::size_t start = lineContents.find(searchString);
			if(start != string::npos)
			{
				std::size_t end = lineContents.find(" : ");
				stringstream registerValueString(lineContents.substr(start + searchString.length(), end));
				uint32_t registerValue = 0;
				registerValueString >> registerValue;
				cout << "Writing register: " << registerToLoadStrings[i].registerStr << " " << registerValue << endl;
				mDevice.WriteRegister(registerToLoadStrings[i].registerNum, registerValue);
			}
			else
			{
				cout << "The format of the log file is not compatible with this option." << endl;
				return false;
			}
		}
		else
		{
			continue;
		}
		i++;
	}

	return true;
}

string CNTV2SupportLogger::InventLogFilePathAndName (CNTV2Card & inDevice, const string inPrefix, const string inExtension)
{
	string homePath;
	AJASystemInfo info;
	time_t rawtime;
	ostringstream oss;
	const string deviceName (CNTV2DeviceScanner::GetDeviceRefName(inDevice));

	info.GetValue(AJA_SystemInfoTag_Path_UserHome, homePath);
	if (!homePath.empty())
		oss << homePath << PATH_DELIMITER;
	oss << inPrefix << "_" << deviceName << "_" << ::time(&rawtime) << "." << inExtension;
	return oss.str();
}

bool CNTV2SupportLogger::DumpDeviceSDRAM (CNTV2Card & inDevice, const string & inFilePath, ostream & msgStrm)
{
	if (!inDevice.IsOpen())
		return false;
	if (inFilePath.empty())
		return false;
	NTV2Framesize frmsz(NTV2_FRAMESIZE_INVALID);
	const ULWord maxBytes(::NTV2DeviceGetActiveMemorySize(inDevice.GetDeviceID()));
	inDevice.GetFrameBufferSize(NTV2_CHANNEL1, frmsz);
	const ULWord byteCount(::NTV2FramesizeToByteCount(frmsz)), megs(byteCount/1024/1024), numFrames(maxBytes / byteCount);
	NTV2_POINTER buffer(byteCount);
	NTV2ULWordVector goodFrames, badDMAs, badWrites;
	ofstream ofs(inFilePath.c_str(), std::ofstream::out | std::ofstream::binary);
	if (!ofs)
		{msgStrm << "## ERROR: Unable to open '" << inFilePath << "' for writing" << endl;  return false;}

	for (ULWord frameNdx(0);  frameNdx < numFrames;  frameNdx++)
	{
		if (!inDevice.DMAReadFrame(frameNdx, buffer, byteCount, NTV2_CHANNEL1))
			{badDMAs.push_back(frameNdx);  continue;}
		if (!ofs.write(buffer, streamsize(buffer.GetByteCount())).good())
			{badWrites.push_back(frameNdx);  continue;}
		goodFrames.push_back(frameNdx);
	}	//	for each frame
	if (!badDMAs.empty())
	{
		msgStrm << "## ERROR: DMARead failed for " << DEC(badDMAs.size()) << " " << DEC(megs) << "MB frame(s): ";
		::NTV2PrintULWordVector(badDMAs, msgStrm); msgStrm << endl;
	}
	if (!badWrites.empty())
	{
		msgStrm << "## ERROR: Write failures for " << DEC(badWrites.size()) << " " << DEC(megs) << "MB frame(s): ";
		::NTV2PrintULWordVector(badWrites, msgStrm); msgStrm << endl;
	}
	msgStrm << "## NOTE: " << DEC(goodFrames.size()) << " x " << DEC(megs) << "MB frames from device '"
						<< CNTV2DeviceScanner::GetDeviceRefName(inDevice) << "' written to '" << inFilePath << "'" << endl;
	return true;
}
