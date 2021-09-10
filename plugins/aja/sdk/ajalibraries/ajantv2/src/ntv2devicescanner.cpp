/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2devicescanner.cpp
	@brief		Implementation of CNTV2DeviceScanner class.
	@copyright	(C) 2004-2021 AJA Video Systems, Inc.
**/

#include "ntv2devicescanner.h"
#include "ntv2devicefeatures.h"
#include "ntv2utils.h"
#include "ajabase/common/common.h"
#include <sstream>
#include <assert.h>

using namespace std;


static string ToLower (const string & inStr)
{
	string	result(inStr);
	return aja::lower(result);
}

static string ToUpper (const string & inStr)
{
	string	result(inStr);
	return aja::upper(result);
}

bool CNTV2DeviceScanner::IsHexDigit (const char inChr)
{	static const string sHexDigits("0123456789ABCDEFabcdef");
	return sHexDigits.find(inChr) != string::npos;
}

bool CNTV2DeviceScanner::IsDecimalDigit (const char inChr)
{	static const string sDecDigits("0123456789");
	return sDecDigits.find(inChr) != string::npos;
}

bool CNTV2DeviceScanner::IsAlphaNumeric (const char inChr)
{	static const string sLegalChars("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
	return sLegalChars.find(inChr) != string::npos;
}

bool CNTV2DeviceScanner::IsLegalDecimalNumber (const string & inStr, const size_t inMaxLength)
{
	if (inStr.length() > inMaxLength)
		return false;	//	Too long
	for (size_t ndx(0);  ndx < inStr.size();  ndx++)
		if (!IsDecimalDigit(inStr.at(ndx)))
			return false;
	return true;
}

uint64_t CNTV2DeviceScanner::IsLegalHexSerialNumber (const string & inStr)	//	0x3236333331375458
{
	if (inStr.length() < 3)
		return 0ULL;	//	Too small
	string hexStr(::ToLower(inStr));
	if (hexStr[0] == '0'  &&  hexStr[1] == 'x')
		hexStr.erase(0, 2);	//	Remove '0x' if present
	if (hexStr.length() > 16)
		return 0ULL;	//	Too big
	for (size_t ndx(0);  ndx < hexStr.size();  ndx++)
		if (!IsHexDigit(hexStr.at(ndx)))
			return 0ULL;	//	Invalid hex digit
	while (hexStr.length() != 16)
		hexStr = '0' + hexStr;	//	prepend another '0'
	istringstream iss(hexStr);
	uint64_t u64(0);
	iss >> hex >> u64;
	return u64;
}

bool CNTV2DeviceScanner::IsAlphaNumeric (const string & inStr)
{
	for (size_t ndx(0);  ndx < inStr.size();  ndx++)
		if (!IsAlphaNumeric(inStr.at(ndx)))
			return false;
	return true;
}

bool CNTV2DeviceScanner::IsLegalSerialNumber (const string & inStr)
{
	if (inStr.length() != 8  &&  inStr.length() != 9)
		return false;
	return IsAlphaNumeric(inStr);
}


CNTV2DeviceScanner::CNTV2DeviceScanner (const bool inScanNow)
{
	if (inScanNow)
		ScanHardware();
}
CNTV2DeviceScanner::CNTV2DeviceScanner (bool inScanNow, UWord inDeviceMask)
{
	(void)inDeviceMask;
	if (inScanNow)
		ScanHardware();
}


CNTV2DeviceScanner & CNTV2DeviceScanner::operator = (const CNTV2DeviceScanner & boardScan)
{
	//	Avoid self-assignment...
	if (this != &boardScan)
		DeepCopy(boardScan);

	return *this;	//	operator= must return self reference

}	//	operator=


CNTV2DeviceScanner::CNTV2DeviceScanner (const CNTV2DeviceScanner & boardScan)
{
	DeepCopy(boardScan);
}


void CNTV2DeviceScanner::DeepCopy (const CNTV2DeviceScanner & boardScan)
{
	// Begin with a clear list
	_deviceInfoList.clear();
	
	// Copy over _deviceInfoList
	for (NTV2DeviceInfoListConstIter bilIter (boardScan._deviceInfoList.begin ());  bilIter != boardScan._deviceInfoList.end ();  ++bilIter)
	{
		NTV2DeviceInfo boardInfo;
		
		//	Move over all the easy stuff...
		boardInfo.deviceIndex = bilIter->deviceIndex;
		boardInfo.deviceID = bilIter->deviceID;
		boardInfo.pciSlot = bilIter->pciSlot;
		boardInfo.deviceIdentifier = bilIter->deviceIdentifier;
		boardInfo.deviceSerialNumber = bilIter->deviceSerialNumber;

		//	Now copy over each list within the list...
		boardInfo.audioSampleRateList.clear();
		for (NTV2AudioSampleRateList::const_iterator asrIter (bilIter->audioSampleRateList.begin());  asrIter != bilIter->audioSampleRateList.end();  ++asrIter)
			boardInfo.audioSampleRateList.push_back(*asrIter);

		boardInfo.audioNumChannelsList.clear();
		for (NTV2AudioChannelsPerFrameList::const_iterator ncIter (bilIter->audioNumChannelsList.begin());  ncIter != bilIter->audioNumChannelsList.end();  ++ncIter)
			boardInfo.audioNumChannelsList.push_back(*ncIter);

		boardInfo.audioBitsPerSampleList.clear();
		for (NTV2AudioBitsPerSampleList::const_iterator bpsIter (bilIter->audioBitsPerSampleList.begin());  bpsIter != bilIter->audioBitsPerSampleList.end();  ++bpsIter)
			boardInfo.audioBitsPerSampleList.push_back(*bpsIter);

		boardInfo.audioInSourceList.clear();
		for (NTV2AudioSourceList::const_iterator aslIter (bilIter->audioInSourceList.begin());  aslIter != bilIter->audioInSourceList.end();  ++aslIter)
			boardInfo.audioInSourceList.push_back(*aslIter);

		boardInfo.audioOutSourceList.clear();
		for (NTV2AudioSourceList::const_iterator aoslIter (bilIter->audioOutSourceList.begin());  aoslIter != bilIter->audioOutSourceList.end();  ++aoslIter)
			boardInfo.audioOutSourceList.push_back(*aoslIter);

		//	Add this boardInfo struct to the _deviceInfoList...
		_deviceInfoList.push_back(boardInfo);
	}
}	//	DeepCopy


void CNTV2DeviceScanner::ScanHardware (UWord inDeviceMask)
{	(void) inDeviceMask;
	ScanHardware();
}
void CNTV2DeviceScanner::ScanHardware (void)
{
	GetDeviceInfoList().clear();

	for (UWord boardNum(0);   ;   boardNum++)
	{
		CNTV2Card tmpDevice(boardNum);
		if (tmpDevice.IsOpen())
		{
			const NTV2DeviceID	deviceID (tmpDevice.GetDeviceID());

			if (deviceID != DEVICE_ID_NOTFOUND)
			{
				ostringstream	oss;
				NTV2DeviceInfo	info;
				bool bRetail = tmpDevice.DeviceIsDNxIV();

				info.deviceIndex		= boardNum;
				info.deviceID			= deviceID;
				info.pciSlot			= 0;
#if !defined(NTV2_DEPRECATE_16_0)
				info.pciSlot			= tmpDevice.GetPCISlotNumber();
#endif	//	!defined(NTV2_DEPRECATE_16_0)
				info.deviceSerialNumber	= tmpDevice.GetSerialNumber();

				oss << ::NTV2DeviceIDToString (deviceID, bRetail) << " - " << boardNum;
				if (info.pciSlot)
					oss << ", Slot " << info.pciSlot;

				info.deviceIdentifier = oss.str();
#if !defined (NTV2_DEPRECATE)
				strcpy(info.boardIdentifier, oss.str().c_str());
				info.boardID = deviceID;
				info.boardSerialNumber = tmpDevice.GetSerialNumber();
				info.boardNumber = boardNum;
#endif

				SetVideoAttributes(info);
				SetAudioAttributes(info, tmpDevice);
				GetDeviceInfoList().push_back(info);
			}
			tmpDevice.Close();
		}	//	if Open succeeded
		else
			break;
	}	//	boardNum loop

}	//	ScanHardware


#if !defined (NTV2_DEPRECATE)
	bool CNTV2DeviceScanner::BoardTypePresent (NTV2BoardType boardType, bool rescan)
	{
		if (rescan)
			ScanHardware(true);

		const NTV2DeviceInfoList & boardList(GetDeviceInfoList());
		for (NTV2DeviceInfoListConstIter boardIter(boardList.begin());  boardIter != boardList.end();  ++boardIter)
			if (boardIter->boardType == boardType)
				return true;	//	Found!

		return false;	//	Not found

	}	//	BoardTypePresent

#endif	//	else !defined (NTV2_DEPRECATE)


bool CNTV2DeviceScanner::DeviceIDPresent (const NTV2DeviceID inDeviceID, const bool inRescan)
{
	if (inRescan)
		ScanHardware();

	const NTV2DeviceInfoList & deviceInfoList(GetDeviceInfoList());
	for (NTV2DeviceInfoListConstIter iter(deviceInfoList.begin());  iter != deviceInfoList.end();  ++iter)
		if (iter->deviceID == inDeviceID)
			return true;	//	Found!
	return false;	//	Not found

}	//	DeviceIDPresent


bool CNTV2DeviceScanner::GetDeviceInfo (const ULWord inDeviceIndexNumber, NTV2DeviceInfo & outDeviceInfo, const bool inRescan)
{
	if (inRescan)
		ScanHardware();

	const NTV2DeviceInfoList & deviceList(GetDeviceInfoList());

	if (inDeviceIndexNumber < deviceList.size())
	{
		outDeviceInfo = deviceList[inDeviceIndexNumber];
		return outDeviceInfo.deviceIndex == inDeviceIndexNumber;
	}
	return false;	//	No devices with this index number

}	//	GetDeviceInfo

bool CNTV2DeviceScanner::GetDeviceAtIndex (const ULWord inDeviceIndexNumber, CNTV2Card & outDevice)
{
	outDevice.Close();
	CNTV2DeviceScanner	scanner;
	return size_t(inDeviceIndexNumber) < scanner.GetDeviceInfoList().size() ? reinterpret_cast<CNTV2DriverInterface&>(outDevice).Open(UWord(inDeviceIndexNumber)) : false;

}	//	GetDeviceAtIndex


bool CNTV2DeviceScanner::GetFirstDeviceWithID (const NTV2DeviceID inDeviceID, CNTV2Card & outDevice)
{
	outDevice.Close();
	CNTV2DeviceScanner	scanner;
	const NTV2DeviceInfoList &	deviceInfoList(scanner.GetDeviceInfoList());
	for (NTV2DeviceInfoListConstIter iter(deviceInfoList.begin());  iter != deviceInfoList.end();  ++iter)
		if (iter->deviceID == inDeviceID)
			return AsNTV2DriverInterfaceRef(outDevice).Open(UWord(iter->deviceIndex));	//	Found!
	return false;	//	Not found

}	//	GetFirstDeviceWithID


bool CNTV2DeviceScanner::GetFirstDeviceWithName (const string & inNameSubString, CNTV2Card & outDevice)
{
	outDevice.Close();
	if (!IsAlphaNumeric(inNameSubString))
	{
		if (inNameSubString.find(":") != string::npos)
			return outDevice.Open(inNameSubString);
		return false;
	}

	CNTV2DeviceScanner	scanner;
	string				nameSubString(::ToLower(inNameSubString));
	const NTV2DeviceInfoList &	deviceInfoList(scanner.GetDeviceInfoList ());

	for (NTV2DeviceInfoListConstIter iter(deviceInfoList.begin());  iter != deviceInfoList.end();  ++iter)
	{
		const string	deviceName(::ToLower(iter->deviceIdentifier));
		if (deviceName.find(nameSubString) != string::npos)
			return AsNTV2DriverInterfaceRef(outDevice).Open(UWord(iter->deviceIndex));	//	Found!
	}
	if (nameSubString == "io4kplus")
	{	//	Io4K+ == DNXIV...
		nameSubString = "avid dnxiv";
		for (NTV2DeviceInfoListConstIter iter(deviceInfoList.begin());  iter != deviceInfoList.end();  ++iter)
		{
			const string	deviceName(::ToLower(iter->deviceIdentifier));
			if (deviceName.find(nameSubString) != string::npos)
				return AsNTV2DriverInterfaceRef(outDevice).Open(UWord(iter->deviceIndex));	//	Found!
		}
	}
	return false;	//	Not found

}	//	GetFirstDeviceWithName


bool CNTV2DeviceScanner::GetFirstDeviceWithSerial (const string & inSerialStr, CNTV2Card & outDevice)
{
	CNTV2DeviceScanner	scanner;
	outDevice.Close();
	const string searchSerialStr(::ToLower(inSerialStr));
	const NTV2DeviceInfoList &	deviceInfos(scanner.GetDeviceInfoList());
	for (NTV2DeviceInfoListConstIter iter(deviceInfos.begin());  iter != deviceInfos.end();  ++iter)
	{
		CNTV2Card dev(UWord(iter->deviceIndex));
		string serNumStr;
		if (dev.GetSerialNumberString(serNumStr))
		{
			aja::lower(serNumStr);
			if (serNumStr.find(searchSerialStr) != string::npos)
				return AsNTV2DriverInterfaceRef(outDevice).Open(UWord(iter->deviceIndex));
		}
	}
	return false;
}


bool CNTV2DeviceScanner::GetDeviceWithSerial (const uint64_t inSerialNumber, CNTV2Card & outDevice)
{
	outDevice.Close();
	CNTV2DeviceScanner	scanner;
	const NTV2DeviceInfoList &	deviceInfos(scanner.GetDeviceInfoList());
	for (NTV2DeviceInfoListConstIter iter(deviceInfos.begin());  iter != deviceInfos.end();  ++iter)
		if (iter->deviceSerialNumber == inSerialNumber)
			return AsNTV2DriverInterfaceRef(outDevice).Open(UWord(iter->deviceIndex));
	return false;
}


bool CNTV2DeviceScanner::GetFirstDeviceFromArgument (const string & inArgument, CNTV2Card & outDevice)
{
	outDevice.Close();
	if (inArgument.empty())
		return false;

	//	Index number:
	if (IsLegalDecimalNumber(inArgument))
		return GetDeviceAtIndex (ULWord(aja::stoul(inArgument)), outDevice);

	CNTV2DeviceScanner	scanner;
	const NTV2DeviceInfoList &	infoList (scanner.GetDeviceInfoList());
	string upperArg(::ToUpper(inArgument));
	if (upperArg == "LIST" || upperArg == "?")
	{
		if (infoList.empty())
			cout << "No devices detected" << endl;
		else
			cout << DEC(infoList.size()) << " available " << (infoList.size() == 1 ? "device:" : "devices:") << endl;
		for (NTV2DeviceInfoListConstIter iter(infoList.begin());  iter != infoList.end();  ++iter)
		{
			const string serNum(CNTV2Card::SerialNum64ToString(iter->deviceSerialNumber));
			cout << DECN(iter->deviceIndex,2) << " | " << setw(8) << ::NTV2DeviceIDToString(iter->deviceID);
			if (!serNum.empty())
				cout << " | " << setw(9) << serNum << " | " << HEX0N(iter->deviceSerialNumber,8);
			cout << endl;
		}
		return false;
	}

	//	Serial number (string):
	if (IsLegalSerialNumber(upperArg))
	{
		if (upperArg.length() == 9)	//	Special case for DNXIV serial numbers
			upperArg.erase(0,1);	//	Remove 1st character
		for (NTV2DeviceInfoListConstIter iter(infoList.begin());  iter != infoList.end();  ++iter)
			if (CNTV2Card::SerialNum64ToString(iter->deviceSerialNumber) == upperArg)
				return AsNTV2DriverInterfaceRef(outDevice).Open(UWord(iter->deviceIndex));	//	Found!
	}

	//	Hex serial number:
	const uint64_t serialNumber(IsLegalHexSerialNumber(inArgument));
	if (serialNumber)
		if (GetDeviceWithSerial(serialNumber, outDevice))
			if (outDevice.IsOpen())
				return true;
#if 0
	if (::IsAlphaNumeric(inArgument))
	{
		const string			argLowerCase		(::ToLower(inArgument));
		const NTV2DeviceIDSet	supportedDeviceIDs	(::NTV2GetSupportedDevices());
		for (NTV2DeviceIDSetConstIter devIDiter(supportedDeviceIDs.begin());  devIDiter != supportedDeviceIDs.end();  ++devIDiter)
		{
			const string deviceName	(::ToLower(::NTV2DeviceIDToString(*devIDiter, false)));
			if (deviceName.find(argLowerCase) != string::npos)
				if (GetFirstDeviceWithID(*devIDiter, outDevice))
					if (outDevice.IsOpen())
						return true;	//	Found!
		}
	}
#endif
	//	First matching name:
	return GetFirstDeviceWithName (inArgument, outDevice);

}	//	GetFirstDeviceFromArgument


ostream &	operator << (ostream & inOutStr, const NTV2DeviceInfoList & inList)
{
	for (NTV2DeviceInfoListConstIter iter(inList.begin());  iter != inList.end();  ++iter)
		inOutStr << " " << *iter;
	return inOutStr;

}	//	NTV2DeviceInfoList ostream operator <<


bool NTV2DeviceInfo::operator == (const NTV2DeviceInfo & second) const
{
	const NTV2DeviceInfo &	first	(*this);
	size_t					diffs	(0);

	//	'memcmp' would be simpler, but because NTV2DeviceInfo has no constructor, the unfilled bytes in
	//	its "boardIdentifier" field are indeterminate, making it worthless for accurate comparisons.
	//	"boardSerialNumber" and boardNumber are the only required comparisons, but I also check boardType,
	//	boardID, and pciSlot for good measure...
	#if !defined (NTV2_DEPRECATE)
	if (first.boardType							!=	second.boardType)						diffs++;
	#endif	//	!defined (NTV2_DEPRECATE)
	if (first.deviceID							!=	second.deviceID)						diffs++;
	if (first.deviceIndex						!=	second.deviceIndex)						diffs++;
	if (first.deviceSerialNumber				!=	second.deviceSerialNumber)				diffs++;
	if (first.pciSlot							!=	second.pciSlot)							diffs++;
	
	// Needs to be fixed now that deviceIdentifier is a std::string
	//#if defined (AJA_DEBUG)
	//	if (::strncmp (first.deviceIdentifier.c_str (), second.deviceIdentifier.c_str (), first.deviceIdentifier.length ())))	diffs++;
	//	if (diffs)
	//		{cout << "## DEBUG:  " << diffs << " diff(s):" << endl << "#### first ####" << endl << first << "#### second ####" << endl << second << endl;}
	//#endif	//	AJA_DEBUG

	return diffs ? false : true;

}	//	equality operator


bool CNTV2DeviceScanner::CompareDeviceInfoLists (const NTV2DeviceInfoList & inOldList,
											const NTV2DeviceInfoList & inNewList,
											NTV2DeviceInfoList & outBoardsAdded,
											NTV2DeviceInfoList & outBoardsRemoved)
{
	NTV2DeviceInfoListConstIter	oldIter	(inOldList.begin ());
	NTV2DeviceInfoListConstIter	newIter	(inNewList.begin ());

	outBoardsAdded.clear ();
	outBoardsRemoved.clear ();

	while (true)
	{
		if (oldIter == inOldList.end () && newIter == inNewList.end ())
			break;	//	Done -- exit

		if (oldIter != inOldList.end () && newIter != inNewList.end ())
		{
			const NTV2DeviceInfo &  oldInfo (*oldIter),  newInfo (*newIter);

			if (oldInfo != newInfo)
			{
				//	Out with the old...
				outBoardsRemoved.push_back (oldInfo);

				//	In with the new...
				if (newInfo.deviceID && newInfo.deviceID != NTV2DeviceID(0xFFFFFFFF))
					outBoardsAdded.push_back (newInfo);
			}	//	if mismatch

			++oldIter;
			++newIter;
			continue;	//	Move along

		}	//	if both valid

		if (oldIter != inOldList.end () && newIter == inNewList.end ())
		{
			outBoardsRemoved.push_back (*oldIter);
			++oldIter;
			continue;	//	Move along
		}	//	old is valid, new is not valid

		if (oldIter == inOldList.end () && newIter != inNewList.end ())
		{
			if (newIter->deviceID && newIter->deviceID != NTV2DeviceID(0xFFFFFFFF))
				outBoardsAdded.push_back (*newIter);
			++newIter;
			continue;	//	Move along
		}	//	old is not valid, new is valid

		assert (false && "should never get here");

	}	//	loop til break

	//	Return 'true' if there were any changes...
	return !outBoardsAdded.empty () || !outBoardsRemoved.empty ();

}	//	CompareDeviceInfoLists


string CNTV2DeviceScanner::GetDeviceRefName (CNTV2Card & inDevice)
{	//	Name that will find given device via CNTV2DeviceScanner::GetFirstDeviceFromArgument
	if (!inDevice.IsOpen())
		return string();
	//	Nub address 1st...
	if (!inDevice.GetHostName().empty()  &&  inDevice.IsRemote())
		return inDevice.GetHostName();	//	Nub host/device

	//	Serial number 2nd...
	string str;
	if (inDevice.GetSerialNumberString(str))
		return str;

	//	Model name 3rd...
	str = ::NTV2DeviceIDToString(inDevice.GetDeviceID(), false);
	if (!str.empty() &&  str != "???")
		return str;

	//	Index number last...
	ostringstream oss;  oss << DEC(inDevice.GetIndexNumber());
	return oss.str();
}


ostream &	operator << (ostream & inOutStr, const NTV2AudioSampleRateList & inList)
{
	for (NTV2AudioSampleRateListConstIter iter (inList.begin ()); iter != inList.end (); ++iter)
		inOutStr << " " << *iter;

	return inOutStr;
}


ostream &	operator << (ostream & inOutStr, const NTV2AudioChannelsPerFrameList & inList)
{
	for (NTV2AudioChannelsPerFrameListConstIter iter (inList.begin ());  iter != inList.end ();  ++iter)
		inOutStr << " " << *iter;

	return inOutStr;
}


ostream &	operator << (ostream & inOutStr, const NTV2AudioSourceList & inList)
{
	for (NTV2AudioSourceListConstIter iter(inList.begin());  iter != inList.end();  ++iter)
		switch (*iter)	//	AudioSourceEnum
		{
			case kSourceSDI:	return inOutStr << " SDI";
			case kSourceAES:	return inOutStr << " AES";
			case kSourceADAT:	return inOutStr << " ADAT";
			case kSourceAnalog:	return inOutStr << " Analog";
			case kSourceNone:	return inOutStr << " None";
			case kSourceAll:	return inOutStr << " All";
		}
	return inOutStr << " ???";
}


ostream &	operator << (ostream & inOutStr, const NTV2AudioBitsPerSampleList & inList)
{
	for (NTV2AudioBitsPerSampleListConstIter iter (inList.begin ());  iter != inList.end ();  ++iter)
		inOutStr << " " << *iter;

	return inOutStr;
}


#if !defined (NTV2_DEPRECATE)

	void CNTV2DeviceScanner::DumpBoardInfo (const NTV2DeviceInfo & info)
	{
		#if defined (DEBUG) || defined (_DEBUG) || defined (AJA_DEBUG)
			cout << info << endl;
		#else
			(void) info;
		#endif
	}	//	DumpBoardInfo

#endif	//	!NTV2_DEPRECATE


ostream &	operator << (ostream & inOutStr, const NTV2DeviceInfo & inInfo)
{
	inOutStr	<< "Device Info for '" << inInfo.deviceIdentifier << "'" << endl
				<< "            Device Index Number: " << inInfo.deviceIndex << endl
				#if !defined (NTV2_DEPRECATE)
				<< "                    Device Type: 0x" << hex << inInfo.boardType << dec << endl
				#endif	//	!defined (NTV2_DEPRECATE)
				<< "                      Device ID: 0x" << hex << inInfo.deviceID << dec << endl
				<< "                  Serial Number: 0x" << hex << inInfo.deviceSerialNumber << dec << endl
				<< "                       PCI Slot: 0x" << hex << inInfo.pciSlot << dec << endl
				<< "                   Video Inputs: " << inInfo.numVidInputs << endl
				<< "                  Video Outputs: " << inInfo.numVidOutputs << endl
				#if defined (_DEBUG)
					<< "            Analog Video Inputs: " << inInfo.numAnlgVidInputs << endl
					<< "           Analog Video Outputs: " << inInfo.numAnlgVidOutputs << endl
					<< "              HDMI Video Inputs: " << inInfo.numHDMIVidInputs << endl
					<< "             HDMI Video Outputs: " << inInfo.numHDMIVidOutputs << endl
					<< "               Input Converters: " << inInfo.numInputConverters << endl
					<< "              Output Converters: " << inInfo.numOutputConverters << endl
					<< "                  Up Converters: " << inInfo.numUpConverters << endl
					<< "                Down Converters: " << inInfo.numDownConverters << endl
					<< "           Down Converter Delay: " << inInfo.downConverterDelay << endl
					<< "                       DVCProHD: " << (inInfo.dvcproHDSupport ? "Y" : "N") << endl
					<< "                           Qrez: " << (inInfo.qrezSupport ? "Y" : "N") << endl
					<< "                            HDV: " << (inInfo.hdvSupport ? "Y" : "N") << endl
					<< "                 Quarter Expand: " << (inInfo.quarterExpandSupport ? "Y" : "N") << endl
					<< "                    ISO Convert: " << (inInfo.isoConvertSupport ? "Y" : "N") << endl
					<< "                   Rate Convert: " << (inInfo.rateConvertSupport ? "Y" : "N") << endl
					<< "                        VidProc: " << (inInfo.vidProcSupport ? "Y" : "N") << endl
					<< "                      Dual-Link: " << (inInfo.dualLinkSupport ? "Y" : "N") << endl
					<< "               Color-Correction: " << (inInfo.colorCorrectionSupport ? "Y" : "N") << endl
					<< "               Programmable CSC: " << (inInfo.programmableCSCSupport ? "Y" : "N") << endl
					<< "               RGB Alpha Output: " << (inInfo.rgbAlphaOutputSupport ? "Y" : "N") << endl
					<< "                   Breakout Box: " << (inInfo.breakoutBoxSupport ? "Y" : "N") << endl
					<< "                        ProcAmp: " << (inInfo.procAmpSupport ? "Y" : "N") << endl
					<< "                             2K: " << (inInfo.has2KSupport ? "Y" : "N") << endl
					<< "                             4K: " << (inInfo.has4KSupport ? "Y" : "N") << endl
					<< "                             8K: " << (inInfo.has8KSupport ? "Y" : "N") << endl
                    << "            3G Level Conversion: " << (inInfo.has3GLevelConversion ? "Y" : "N") << endl
					<< "                         ProRes: " << (inInfo.proResSupport ? "Y" : "N") << endl
					<< "                         SDI 3G: " << (inInfo.sdi3GSupport ? "Y" : "N") << endl
					<< "                        SDI 12G: " << (inInfo.sdi12GSupport ? "Y" : "N") << endl
					<< "                             IP: " << (inInfo.ipSupport ? "Y" : "N") << endl
					<< "             SDI Bi-Directional: " << (inInfo.biDirectionalSDI ? "Y" : "N") << endl
					<< "                         LTC In: " << (inInfo.ltcInSupport ? "Y" : "N") << endl
					<< "                        LTC Out: " << (inInfo.ltcOutSupport ? "Y" : "N") << endl
					<< "             LTC In on Ref Port: " << (inInfo.ltcInOnRefPort ? "Y" : "N") << endl
					<< "                     Stereo Out: " << (inInfo.stereoOutSupport ? "Y" : "N") << endl
					<< "                      Stereo In: " << (inInfo.stereoInSupport ? "Y" : "N") << endl
					<< "             Audio Sample Rates: " << inInfo.audioSampleRateList << endl
					<< "           AudioNumChannelsList: " << inInfo.audioNumChannelsList << endl
					<< "         AudioBitsPerSampleList: " << inInfo.audioBitsPerSampleList << endl
					<< "              AudioInSourceList: " << inInfo.audioInSourceList << endl
					<< "             AudioOutSourceList: " << inInfo.audioOutSourceList << endl
					<< "                  Audio Streams: " << inInfo.numAudioStreams << endl
					<< "    Analog Audio Input Channels: " << inInfo.numAnalogAudioInputChannels << endl
					<< "   Analog Audio Output Channels: " << inInfo.numAnalogAudioOutputChannels << endl
					<< "       AES Audio Input Channels: " << inInfo.numAESAudioInputChannels << endl
					<< "      AES Audio Output Channels: " << inInfo.numAESAudioOutputChannels << endl
					<< "  Embedded Audio Input Channels: " << inInfo.numEmbeddedAudioInputChannels << endl
					<< " Embedded Audio Output Channels: " << inInfo.numEmbeddedAudioOutputChannels << endl
					<< "      HDMI Audio Input Channels: " << inInfo.numHDMIAudioInputChannels << endl
					<< "     HDMI Audio Output Channels: " << inInfo.numHDMIAudioOutputChannels << endl
					<< "                    DMA Engines: " << inInfo.numDMAEngines << endl
					<< "                   Serial Ports: " << inInfo.numSerialPorts << endl
				#endif	//	AJA_DEBUG
				<< "";

	return inOutStr;

}	//	NTV2DeviceInfo ostream operator <<


ostream &	operator << (ostream & inOutStr, const NTV2AudioPhysicalFormat & inFormat)
{
	inOutStr	<< "AudioPhysicalFormat:" << endl
				<< "    boardNumber: " << inFormat.boardNumber << endl
				<< "     sampleRate: " << inFormat.sampleRate << endl
				<< "    numChannels: " << inFormat.numChannels << endl
				<< "  bitsPerSample: " << inFormat.bitsPerSample << endl
	#if defined (DEBUG) || defined (AJA_DEBUG)
				<< "       sourceIn: 0x" << hex << inFormat.sourceIn << dec << endl
				<< "      sourceOut: 0x" << hex << inFormat.sourceOut << dec << endl
	#endif	//	DEBUG or AJA_DEBUG
				;

	return inOutStr;

}	//	AudioPhysicalFormat ostream operator <<


std::ostream &	operator << (std::ostream & inOutStr, const NTV2AudioPhysicalFormatList & inList)
{
	for (NTV2AudioPhysicalFormatListConstIter iter (inList.begin ()); iter != inList.end (); ++iter)
		inOutStr << *iter;

	return inOutStr;

}	//	AudioPhysicalFormatList ostream operator <<


#if !defined (NTV2_DEPRECATE)

	void CNTV2DeviceScanner::DumpAudioFormatInfo (const NTV2AudioPhysicalFormat & audioPhysicalFormat)
	{
		#if defined (DEBUG) || defined (AJA_DEBUG)
			cout << audioPhysicalFormat << endl;
		#else
			(void) audioPhysicalFormat;
		#endif
	}	//	DumpAudioFormatInfo

#endif	//	!NTV2_DEPRECATE


// Private methods

void CNTV2DeviceScanner::SetVideoAttributes (NTV2DeviceInfo & info)
{	
	info.numVidInputs			= NTV2DeviceGetNumVideoInputs		(info.deviceID);
	info.numVidOutputs			= NTV2DeviceGetNumVideoOutputs		(info.deviceID);
	info.numAnlgVidOutputs		= NTV2DeviceGetNumAnalogVideoOutputs	(info.deviceID);
	info.numAnlgVidInputs		= NTV2DeviceGetNumAnalogVideoInputs	(info.deviceID);
	info.numHDMIVidOutputs		= NTV2DeviceGetNumHDMIVideoOutputs	(info.deviceID);
	info.numHDMIVidInputs		= NTV2DeviceGetNumHDMIVideoInputs	(info.deviceID);
	info.numInputConverters		= NTV2DeviceGetNumInputConverters	(info.deviceID);
	info.numOutputConverters	= NTV2DeviceGetNumOutputConverters	(info.deviceID);
	info.numUpConverters		= NTV2DeviceGetNumUpConverters		(info.deviceID);
	info.numDownConverters		= NTV2DeviceGetNumDownConverters	(info.deviceID);
	info.downConverterDelay		= NTV2DeviceGetDownConverterDelay	(info.deviceID);
	info.dvcproHDSupport		= NTV2DeviceCanDoDVCProHD			(info.deviceID);
	info.qrezSupport			= NTV2DeviceCanDoQREZ				(info.deviceID);
	info.hdvSupport				= NTV2DeviceCanDoHDV				(info.deviceID);
	info.quarterExpandSupport	= NTV2DeviceCanDoQuarterExpand		(info.deviceID);
	info.colorCorrectionSupport	= NTV2DeviceCanDoColorCorrection	(info.deviceID);
	info.programmableCSCSupport	= NTV2DeviceCanDoProgrammableCSC	(info.deviceID);
	info.rgbAlphaOutputSupport	= NTV2DeviceCanDoRGBPlusAlphaOut	(info.deviceID);
	info.breakoutBoxSupport		= NTV2DeviceCanDoBreakoutBox		(info.deviceID);
	info.vidProcSupport			= NTV2DeviceCanDoVideoProcessing	(info.deviceID);
	info.dualLinkSupport		= NTV2DeviceCanDoDualLink			(info.deviceID);
	info.numDMAEngines			= static_cast <UWord> (NTV2GetNumDMAEngines (info.deviceID));
	info.pingLED				= NTV2DeviceGetPingLED				(info.deviceID);
	info.has2KSupport			= NTV2DeviceCanDo2KVideo			(info.deviceID);
	info.has4KSupport			= NTV2DeviceCanDo4KVideo			(info.deviceID);
	info.has8KSupport			= NTV2DeviceCanDo8KVideo			(info.deviceID);
    info.has3GLevelConversion   = NTV2DeviceCanDo3GLevelConversion  (info.deviceID);
	info.isoConvertSupport		= NTV2DeviceCanDoIsoConvert			(info.deviceID);
	info.rateConvertSupport		= NTV2DeviceCanDoRateConvert		(info.deviceID);
	info.proResSupport			= NTV2DeviceCanDoProRes				(info.deviceID);
	info.sdi3GSupport			= NTV2DeviceCanDo3GOut				(info.deviceID, 0);
    info.sdi12GSupport			= NTV2DeviceCanDo12GSDI				(info.deviceID);
    info.ipSupport				= NTV2DeviceCanDoIP					(info.deviceID);
    info.biDirectionalSDI		= NTV2DeviceHasBiDirectionalSDI		(info.deviceID);
	info.ltcInSupport			= NTV2DeviceGetNumLTCInputs			(info.deviceID) > 0;
	info.ltcOutSupport			= NTV2DeviceGetNumLTCOutputs		(info.deviceID) > 0;
	info.ltcInOnRefPort			= NTV2DeviceCanDoLTCInOnRefPort		(info.deviceID);
	info.stereoOutSupport		= NTV2DeviceCanDoStereoOut			(info.deviceID);
	info.stereoInSupport		= NTV2DeviceCanDoStereoIn			(info.deviceID);
	info.multiFormat			= NTV2DeviceCanDoMultiFormat		(info.deviceID);
	info.numSerialPorts			= NTV2DeviceGetNumSerialPorts		(info.deviceID);
	#if !defined (NTV2_DEPRECATE)
		info.procAmpSupport		= NTV2BoardCanDoProcAmp				(info.deviceID);
	#else
		info.procAmpSupport		= false;
	#endif

}	//	SetVideoAttributes

void CNTV2DeviceScanner::SetAudioAttributes(NTV2DeviceInfo & info, CNTV2Card & inBoard) const
{
	//	Start with empty lists...
	info.audioSampleRateList.clear();
	info.audioNumChannelsList.clear();
	info.audioBitsPerSampleList.clear();
	info.audioInSourceList.clear();
	info.audioOutSourceList.clear();


	if (::NTV2DeviceGetNumAudioSystems(info.deviceID))
	{
		ULWord audioControl;
		inBoard.ReadRegister(kRegAud1Control, audioControl);

		//audioSampleRateList
		info.audioSampleRateList.push_back(k48KHzSampleRate);
		if (::NTV2DeviceCanDoAudio96K(info.deviceID))
			info.audioSampleRateList.push_back(k96KHzSampleRate);

		//audioBitsPerSampleList
		info.audioBitsPerSampleList.push_back(k32bitsPerSample);

		//audioInSourceList
		info.audioInSourceList.push_back(kSourceSDI);
		if (audioControl & BIT(21))
			info.audioInSourceList.push_back(kSourceAES);
		if (::NTV2DeviceCanDoAnalogAudio(info.deviceID))
			info.audioInSourceList.push_back(kSourceAnalog);

		//audioOutSourceList
		info.audioOutSourceList.push_back(kSourceAll);

		//audioNumChannelsList
		if (::NTV2DeviceCanDoAudio2Channels(info.deviceID))
			info.audioNumChannelsList.push_back(kNumAudioChannels2);
		if (::NTV2DeviceCanDoAudio6Channels(info.deviceID))
			info.audioNumChannelsList.push_back(kNumAudioChannels6);
		if (::NTV2DeviceCanDoAudio8Channels(info.deviceID))
			info.audioNumChannelsList.push_back(kNumAudioChannels8);

		info.numAudioStreams = ::NTV2DeviceGetNumAudioSystems(info.deviceID);
	}

	info.numAnalogAudioInputChannels = ::NTV2DeviceGetNumAnalogAudioInputChannels(info.deviceID);
	info.numAESAudioInputChannels = ::NTV2DeviceGetNumAESAudioInputChannels(info.deviceID);
	info.numEmbeddedAudioInputChannels = ::NTV2DeviceGetNumEmbeddedAudioInputChannels(info.deviceID);
	info.numHDMIAudioInputChannels = ::NTV2DeviceGetNumHDMIAudioInputChannels(info.deviceID);
	info.numAnalogAudioOutputChannels = ::NTV2DeviceGetNumAnalogAudioOutputChannels(info.deviceID);
	info.numAESAudioOutputChannels = ::NTV2DeviceGetNumAESAudioOutputChannels(info.deviceID);
	info.numEmbeddedAudioOutputChannels = ::NTV2DeviceGetNumEmbeddedAudioOutputChannels(info.deviceID);
	info.numHDMIAudioOutputChannels = ::NTV2DeviceGetNumHDMIAudioOutputChannels(info.deviceID);

}	//	SetAudioAttributes


//	Sort functor based on PCI slot number...
static bool gCompareSlot (const NTV2DeviceInfo & b1, const NTV2DeviceInfo & b2)
{
	return b1.deviceIndex < b2.deviceIndex;
}


//	Sort boards in boardInfoList
void CNTV2DeviceScanner::SortDeviceInfoList (void)
{
	std::sort (_deviceInfoList.begin (), _deviceInfoList.end (), gCompareSlot);
}


//	This needs to be moved into a C++ compatible "device features" module:
bool NTV2DeviceGetSupportedVideoFormats (const NTV2DeviceID inDeviceID, NTV2VideoFormatSet & outFormats)
{
	bool	isOkay	(true);

	outFormats.clear();

    for (NTV2VideoFormat videoFormat(NTV2_FORMAT_UNKNOWN);  videoFormat < NTV2_MAX_NUM_VIDEO_FORMATS;  videoFormat = NTV2VideoFormat(videoFormat + 1))
	{
		if (!::NTV2DeviceCanDoVideoFormat (inDeviceID, videoFormat))
			continue;
		try
		{
			outFormats.insert(videoFormat);
		}
		catch (const std::bad_alloc &)
		{
			isOkay = false;
			outFormats.clear();
			break;
		}
	}	//	for each video format

	NTV2_ASSERT ((isOkay && !outFormats.empty())  ||  (!isOkay && outFormats.empty()));
	return isOkay;

}	//	NTV2DeviceGetSupportedVideoFormats


//	This needs to be moved into a C++ compatible "device features" module:
bool NTV2DeviceGetSupportedPixelFormats (const NTV2DeviceID inDeviceID, NTV2FrameBufferFormatSet & outFormats)
{
	bool	isOkay	(true);

	outFormats.clear ();

	for (NTV2PixelFormat pixelFormat(NTV2_FBF_FIRST);  pixelFormat < NTV2_FBF_LAST;  pixelFormat = NTV2PixelFormat(pixelFormat+1))
		if (::NTV2DeviceCanDoFrameBufferFormat (inDeviceID, pixelFormat))
			try
			{
				outFormats.insert (pixelFormat);
			}
			catch (const std::bad_alloc &)
			{
				isOkay = false;
				outFormats.clear ();
				break;
			}

	NTV2_ASSERT ((isOkay && !outFormats.empty () ) || (!isOkay && outFormats.empty () ));
	return isOkay;

}	//	NTV2DeviceGetSupportedPixelFormats


//	This needs to be moved into a C++ compatible "device features" module:
bool NTV2DeviceGetSupportedStandards (const NTV2DeviceID inDeviceID, NTV2StandardSet & outStandards)
{
	NTV2VideoFormatSet	videoFormats;
	outStandards.clear();
	if (!::NTV2DeviceGetSupportedVideoFormats(inDeviceID, videoFormats))
		return false;
	for (NTV2VideoFormatSetConstIter it(videoFormats.begin());  it != videoFormats.end();  ++it)
	{
		const NTV2Standard	std	(::GetNTV2StandardFromVideoFormat(*it));
		if (NTV2_IS_VALID_STANDARD(std)  &&  outStandards.find(std) == outStandards.end())
			outStandards.insert(std);
	}
	return true;
}


//	This needs to be moved into a C++ compatible "device features" module:
bool NTV2DeviceGetSupportedGeometries (const NTV2DeviceID inDeviceID, NTV2GeometrySet & outGeometries)
{
	NTV2VideoFormatSet	videoFormats;
	outGeometries.clear();
	if (!::NTV2DeviceGetSupportedVideoFormats(inDeviceID, videoFormats))
		return false;
	for (NTV2VideoFormatSetConstIter it(videoFormats.begin());  it != videoFormats.end();  ++it)
	{
		const NTV2FrameGeometry	fg	(::GetNTV2FrameGeometryFromVideoFormat(*it));
		if (NTV2_IS_VALID_NTV2FrameGeometry(fg))
			outGeometries += ::GetRelatedGeometries(fg);
	}
	return true;
}
