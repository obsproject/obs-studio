/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2bitfile.cpp
	@brief		Implementation of CNTV2Bitfile class.
	@copyright	(C) 2010-2021 AJA Video Systems, Inc.    All rights reserved.
**/
#include "ntv2bitfile.h"
#include "ntv2card.h"
#include "ntv2utils.h"
#include "ntv2endian.h"
#include "ajabase/system/debug.h"
#include "ajabase/common/common.h"
#include <iostream>
#include <sys/stat.h>
#include <assert.h>
#if defined (AJALinux) || defined (AJAMac)
	#include <arpa/inet.h>
#endif
#include <map>

using namespace std;


// TODO: Handle compressed bit-files
#define MAX_BITFILEHEADERSIZE 512
static const unsigned char gSignature[8]= {0xFF, 0xFF, 0xFF, 0xFF, 0xAA, 0x99, 0x55, 0x66};
static const unsigned char gHead13[]	= {0x00, 0x09, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x00, 0x00, 0x01};


CNTV2Bitfile::CNTV2Bitfile ()
{
	Close();	//	Initialize everything
}

CNTV2Bitfile::~CNTV2Bitfile ()
{
	Close();
}

void CNTV2Bitfile::Close (void)
{
	if (_fileReady)
		_bitFileStream.close();
	_fileHeader.clear();
	_fileProgrammingPosition = 0;
	_date = _time = _designName = _partName = _lastError = "";
	_numBytes = _fileSize = _programStreamPos = _fileStreamPos = 0;
	_fileReady = _tandem = _partial = _clear = _compress = false;
	_userID = _designID = _designVersion = _bitfileID = _bitfileVersion = 0;
}


#if !defined (NTV2_DEPRECATE)
	bool CNTV2Bitfile::Open (const char * const & inBitfilePath)		{ return Open (string (inBitfilePath)); }
#endif	//	!defined (NTV2_DEPRECATE)


bool CNTV2Bitfile::Open (const string & inBitfileName)
{
	Close();

	ostringstream oss;
	struct stat	fsinfo;
	::stat(inBitfileName.c_str(), &fsinfo);
	_fileSize = unsigned(fsinfo.st_size);
	_bitFileStream.open (inBitfileName.c_str(),  std::ios::binary | std::ios::in);

	do
	{
		if (_bitFileStream.fail())
			{oss << "Unable to open bitfile '" << inBitfileName << "'";  break;}

		//	Preload bitfile header into mem
		for (size_t ndx(0);  ndx < MAX_BITFILEHEADERSIZE - 1;  ndx++)
			_fileHeader.push_back (static_cast<unsigned char>(_bitFileStream.get()));

		oss << ParseHeader();
		if (!oss.str().empty())
			break;

		_fileReady = true;
	} while (false);

	_lastError = oss.str();
	return _fileReady;
}	//	Open


string CNTV2Bitfile::ParseHeaderFromBuffer (const uint8_t* inBitfileBuffer, const size_t inBufferSize)
{
	return ParseHeaderFromBuffer (NTV2_POINTER(inBitfileBuffer, inBufferSize));
}


string CNTV2Bitfile::ParseHeaderFromBuffer (const NTV2_POINTER & inBitfileBuffer)
{
	Close();
	//	Preload bitfile header into mem
	if (!inBitfileBuffer.GetU8s (_fileHeader, /*U8Offset*/ 0, /*MaxSize*/ MAX_BITFILEHEADERSIZE))
		{ostringstream oss;  oss << inBitfileBuffer;  return oss.str();}

	_lastError = ParseHeader();
	_fileReady = _lastError.empty();
	return _lastError;
}

#define BUMP_POS(_inc_)	pos += (_inc_);																			\
						if (pos >= headerLength)																\
						{																						\
							oss << "Failed, offset " << DEC(pos) << " is past end, length=" << DEC(headerLength);	\
							break;	\
						}


//	ParseHeader returns empty string if header looks OK, otherwise string will contain failure explanation
string CNTV2Bitfile::ParseHeader (void)
{
	static const NTV2_POINTER	Header13 (gHead13, sizeof(gHead13));
	static const NTV2_POINTER	Signature (gSignature, sizeof(gSignature));
	const NTV2_POINTER			fileHeader (&_fileHeader[0], _fileHeader.size());
	uint32_t		fieldLen	(0);	//	Holds size of various header fields, in bytes
	int				pos			(0);	//	Byte offset during parse
	char			testByte	(0);	//	Used when checking against expected header bytes
	ostringstream	oss;				//	Receives parse error information

	const int headerLength(int(_fileHeader.size()));

	do
	{
		//	Make sure first 13 bytes are what we expect...
		NTV2_POINTER portion;
		if (!Header13.IsContentEqual(fileHeader.Segment(portion, 0, 13)))
			{oss << "Failed, byte mismatch in first 13 bytes";	break;}
		BUMP_POS(13)					// skip over header header -- now pointing at 'a'


		//	'a' SECTION
		testByte = fileHeader.I8(pos);
		if (testByte != 'a')
			{oss << "Failed at byte offset " << DEC(pos) << ", expected " << xHEX0N(UWord('a'),2) << ", instead got " << xHEX0N(UWord(testByte),2);	break;}
		BUMP_POS(1)						//	skip 'a'

		if (fileHeader.Segment(portion, ULWord(pos), 2).IsNULL())	//	next 2 bytes have FileName length (big-endian)
			{oss << "Failed fetching 2-byte segment starting at offset " << DEC(pos) << " from " << DEC(headerLength) << "-byte header"; break;}
		fieldLen = NTV2EndianSwap16BtoH(portion.U16(0));	//	FileName length includes terminating NUL
		BUMP_POS(2)						//	now at start of FileName

		if (fileHeader.Segment(portion, ULWord(pos), fieldLen).IsNULL())	//	set DesignName/Flags/UserID portion
			{oss << "Failed fetching " << DEC(fieldLen) << "-byte segment starting at offset " << DEC(pos) << " from " << DEC(headerLength) << "-byte header"; break;}
		const string designBuffer (portion.GetString());
		if (!SetDesignName(designBuffer))	//	grab design Name
			{oss << "Bad design name in '" << designBuffer << "', offset=" << DEC(pos) << ", headerLength=" << DEC(headerLength); break;}
		SetDesignFlags(designBuffer);	//	grab design Flags
		SetDesignUserID(designBuffer);	//	grab design UserId
		BUMP_POS(fieldLen)				//	skip over DesignName - now at 'b'


		//	'b' SECTION
		testByte = fileHeader.I8(pos);
		if (testByte != 'b')
			{oss << "Failed at byte offset " << DEC(pos) << ", expected " << xHEX0N(UWord('b'),2) << ", instead got " << xHEX0N(UWord(testByte),2);	break;}
		BUMP_POS(1)						//	skip 'b'

		if (fileHeader.Segment(portion, ULWord(pos), 2).IsNULL())	//	next 2 bytes have PartName length (big-endian)
			{oss << "Failed fetching 2-byte segment starting at offset " << DEC(pos) << " from " << DEC(headerLength) << "-byte header"; break;}
		fieldLen = NTV2EndianSwap16BtoH(portion.U16(0));	//	PartName length includes terminating NUL
		BUMP_POS(2)						//	now at start of PartName

		if (fileHeader.Segment(portion, ULWord(pos), fieldLen).IsNULL())	//	set PartName portion
			{oss << "Failed fetching " << DEC(fieldLen) << "-byte segment starting at offset " << DEC(pos) << " from " << DEC(headerLength) << "-byte header"; break;}
		_partName = portion.GetString();//	grab PartName
		BUMP_POS(fieldLen)				//	skip past PartName - now at start of 'c' field


		//	'c' SECTION
		testByte = fileHeader.I8(pos);
		if (testByte != 'c')
			{oss << "Failed at byte offset " << DEC(pos) << ", expected " << xHEX0N(UWord('c'),2) << ", instead got " << xHEX0N(UWord(testByte),2);	break;}
		BUMP_POS(1)

		if (fileHeader.Segment(portion, ULWord(pos), 2).IsNULL())	//	next 2 bytes have Date length (big-endian)
			{oss << "Failed fetching 2-byte segment starting at offset " << DEC(pos) << " from " << DEC(headerLength) << "-byte header"; break;}
		fieldLen = NTV2EndianSwap16BtoH(portion.U16(0));	//	Date length includes terminating NUL
		BUMP_POS(2)						//	now at start of Date string

		if (fileHeader.Segment(portion, ULWord(pos), fieldLen).IsNULL())	//	set Date portion
			{oss << "Failed fetching " << DEC(fieldLen) << "-byte segment starting at offset " << DEC(pos) << " from " << DEC(headerLength) << "-byte header"; break;}
		_date = portion.GetString(0, 10);	//	grab Date string (10 chars max)
		BUMP_POS(fieldLen)					//	skip past Date string - now at start of 'd' field


		//	'd' SECTION
		testByte = fileHeader.I8(pos);
		if (testByte != 'd')
			{oss << "Failed at byte offset " << DEC(pos) << ", expected " << xHEX0N(UWord('d'),2) << ", instead got " << xHEX0N(UWord(testByte),2);	break;}
		BUMP_POS(1)

		if (fileHeader.Segment(portion, ULWord(pos), 2).IsNULL())	//	next 2 bytes have Time length (big-endian)
			{oss << "Failed fetching 2-byte segment starting at offset " << DEC(pos) << " from " << DEC(headerLength) << "-byte header"; break;}
		fieldLen = NTV2EndianSwap16BtoH(portion.U16(0));	//	Time length includes terminating NUL
		BUMP_POS(2)						//	now at start of Time string

		if (fileHeader.Segment(portion, ULWord(pos), fieldLen).IsNULL())	//	set Time portion
			{oss << "Failed fetching " << DEC(fieldLen) << "-byte segment starting at offset " << DEC(pos) << " from " << DEC(headerLength) << "-byte header"; break;}
		_time = portion.GetString(0, 8);//	grab Time string (8 chars max)
		BUMP_POS(fieldLen)				//	skip past Time string - now at start of 'e' field


		//	'e' SECTION
		testByte = fileHeader.I8(pos);
		if (testByte != 'e')
			{oss << "Failed at byte offset " << DEC(pos) << ", expected " << xHEX0N(UWord('e'),2) << ", instead got " << xHEX0N(UWord(testByte),2);	break;}
		BUMP_POS(1)						//	skip past 'e'

		if (fileHeader.Segment(portion, ULWord(pos), 4).IsNULL())	//	next 4 bytes have Raw Program Data length (big-endian)
			{oss << "Failed fetching 4-byte segment starting at offset " << DEC(pos) << " from " << DEC(headerLength) << "-byte header"; break;}
		_numBytes = NTV2EndianSwap32BtoH(portion.U32(0));	//	Raw Program Data length, in bytes
		BUMP_POS(4)						//	now at start of Program Data
		
		_fileProgrammingPosition = pos;	// this is where to start the programming stream

		//	Search for the start signature...
		bool bFound(false);		int ndx(0);
		while (!bFound  &&  ndx < 1000  &&  pos < headerLength)
		{
			bFound = fileHeader.Segment(portion, ULWord(pos), 8).IsContentEqual(Signature);
			if (!bFound)
				{ndx++;  pos++;}
		}
		if (!bFound)
			{oss << "Failed at byte offset " << DEC(pos) << ", signature not found"; break;}

		NTV2_ASSERT(oss.str().empty());	//	If we made it this far, the header must be OK -- no error messages
	} while (false);

	if (!oss.str().empty())
		AJA_sERROR(AJA_DebugUnit_Application, AJAFUNC << ": " << oss.str());
	return oss.str();

}	//	ParseHeader


// Get maximum of bufferLength bytes worth of configuration stream data in buffer.
// Return value:
//   number of bytes of data copied into buffer. This can be <= bufferLength
//   zero means configuration stream has finished.
//   Return value 0 indicates error
size_t CNTV2Bitfile::GetProgramByteStream (unsigned char * pOutBuffer, const size_t bufferLength)
{
	size_t	posInBuffer			= 0;
	size_t	programStreamLength	= GetProgramStreamLength();

	if (!pOutBuffer)
		return 0;
	if (!_fileReady)
		return 0;

	_bitFileStream.seekg (_fileProgrammingPosition, std::ios::beg);

	while (_programStreamPos < programStreamLength)
	{
		if (_bitFileStream.eof())
		{	//	Unexpected end of file!
			ostringstream	oss;
			oss << "Unexpected EOF at " << _programStreamPos << "bytes";
			_lastError = oss.str ();
			return 0;
		}
		pOutBuffer[posInBuffer++] = static_cast<unsigned char>(_bitFileStream.get());
		_programStreamPos++;
		if (posInBuffer == bufferLength)
			break;
	}
	return posInBuffer;

}	//	GetProgramByteStream


size_t CNTV2Bitfile::GetProgramByteStream (NTV2_POINTER & outBuffer)
{
	const size_t programStreamLength(GetProgramStreamLength());
	if (!programStreamLength)
		return 0;
	if (!_fileReady)
		return 0;

	ostringstream oss;
	if (outBuffer.GetByteCount() < programStreamLength)	//	If buffer IsNULL or too small...
		outBuffer.Allocate(programStreamLength);			//	...then Allocate or resize it
	if (outBuffer.GetByteCount() < programStreamLength)	//	Check again
	{	oss << "Buffer size " << DEC(outBuffer.GetByteCount()) << " < " << DEC(programStreamLength);
		_lastError = oss.str();
		return 0;
	}
	return GetProgramByteStream (reinterpret_cast<unsigned char *>(outBuffer.GetHostAddress(0)),
								outBuffer.GetByteCount());
}


// Get maximum of fileLength bytes worth of configuration stream data in buffer.
// Return value:
//   number of bytes of data copied into buffer. This can be <= bufferLength
//   zero means configuration stream has finished.
//   Return value 0 indicates error
size_t CNTV2Bitfile::GetFileByteStream (unsigned char * pOutBuffer, const size_t bufferLength)
{
	size_t	posInBuffer			(0);
	size_t	fileStreamLength	(GetFileStreamLength());
	if (!pOutBuffer)
		return 0;
	if (!_fileReady)
		return 0;

	_bitFileStream.seekg (0, std::ios::beg);

	while (_fileStreamPos < fileStreamLength)
	{
		if (_bitFileStream.eof())
		{	//	Unexpected end of file!
			ostringstream	oss;
			oss << "Unexpected EOF at " << _programStreamPos << "bytes";
			_lastError = oss.str();
			return 0;
		}

		pOutBuffer[posInBuffer++] = static_cast<unsigned char>(_bitFileStream.get());
		_programStreamPos++;
		if (posInBuffer == bufferLength)
			break;
	}
	return posInBuffer;

}	//	GetFileByteStream

size_t CNTV2Bitfile::GetFileByteStream (NTV2_POINTER & outBuffer)
{
	const size_t fileStreamLength(GetFileStreamLength());
	if (!fileStreamLength)
		return 0;
	if (!_fileReady)
		return 0;

	ostringstream oss;
	if (outBuffer.GetByteCount() < fileStreamLength)	//	If buffer IsNULL or too small...
		outBuffer.Allocate(fileStreamLength);			//	...then Allocate or resize it
	if (outBuffer.GetByteCount() < fileStreamLength)	//	Check again
	{	oss << "Buffer size " << DEC(outBuffer.GetByteCount()) << " < " << DEC(fileStreamLength);
		_lastError = oss.str();
		return 0;
	}
	return GetFileByteStream (reinterpret_cast<unsigned char *>(outBuffer.GetHostAddress(0)),
								outBuffer.GetByteCount());
}


bool CNTV2Bitfile::SetDesignName (const string & inBuffer)
{
	for (size_t pos(0);  pos < inBuffer.length();  pos++)
	{
		const char ch (inBuffer.at(pos));
		if ((ch < 'A' || ch > 'Z') && (ch < 'a' || ch > 'z') && (ch < '0' || ch > '9') && ch != '_')
			break;	//	Stop here
		_designName += ch;
	}
	return _designName.length() > 0;
}


bool CNTV2Bitfile::SetDesignFlags (const string & inBuffer)
{
	if (inBuffer.find("TANDEM=TRUE") != string::npos)
		_tandem = true;
	if (inBuffer.find("PARTIAL=TRUE") != string::npos)
		_partial = true;
	if (inBuffer.find("CLEAR=TRUE") != string::npos)
		_clear = true;
	if (inBuffer.find("COMPRESS=TRUE") != string::npos)
		_compress = true;
	return true;
}


bool CNTV2Bitfile::SetDesignUserID (const string & inBuffer)
{
	_userID = _designID = _designVersion = _bitfileID = _bitfileVersion = 0;
	ULWord userID (0xffffffff);
	size_t pos (inBuffer.find("UserID=0X"));
	if (pos != string::npos  &&  pos <= (inBuffer.length() - 17))
		//	FUTURE: _userID = aja::stoull(inBuffer.substr(pos+9, 8), AJA_NULL, 16);
		::sscanf(inBuffer.substr(pos+9, 8).c_str(), "%x", &userID);	//	FOR NOW

	if (userID == 0xffffffff)
	{
		pos = inBuffer.find("UserID=");
		if (pos != string::npos  &&  pos <= (inBuffer.length() - 15))
			//	FUTURE: _userID = aja::stoull(inBuffer.substr(pos+7, 8), AJA_NULL, 16);
			::sscanf(inBuffer.substr(pos+7, 8).c_str(), "%x", &userID);	//	FOR NOW
	}

	_userID			= userID;
	_designID		= GetDesignID(userID);
	_designVersion	= GetDesignVersion(userID);
	_bitfileID		= GetBitfileID(userID);
	_bitfileVersion	= GetBitfileVersion(userID);
	return true;
}


static string NTV2GetPrimaryHardwareDesignName (const NTV2DeviceID inDeviceID)
{
	switch (inDeviceID)
	{
		case DEVICE_ID_NOTFOUND:			break;
		case DEVICE_ID_CORVID1:				return "corvid1pcie";		//	top.ncd
		case DEVICE_ID_CORVID3G:			return "corvid1_3Gpcie";	//	corvid1_3Gpcie
		case DEVICE_ID_CORVID22:			return "top_c22";			//	top_c22.ncd
		case DEVICE_ID_CORVID24:			return "corvid24_quad";		//	corvid24_quad.ncd
		case DEVICE_ID_CORVID44:			return "corvid_44";			//	corvid_44
		case DEVICE_ID_CORVID88:			return "corvid_88";			//	CORVID88
		case DEVICE_ID_CORVIDHEVC:			return "corvid_hevc";       //	CORVIDHEVC
		case DEVICE_ID_KONA3G:				return "K3G_top";			//	K3G_top.ncd
		case DEVICE_ID_KONA3GQUAD:			return "K3G_quad";			//	K3G_quad.ncd
		case DEVICE_ID_KONA4:				return "kona_4_quad";		//	kona_4_quad
		case DEVICE_ID_KONA4UFC:			return "kona_4_ufc";		//	kona_4_ufc
		case DEVICE_ID_IO4K:				return "IO_XT_4K";			//	IO_XT_4K
		case DEVICE_ID_IO4KUFC:				return "IO_XT_4K_UFC";		//	IO_XT_4K_UFC
		case DEVICE_ID_IOEXPRESS:			return "chekov_00_pcie";	//	chekov_00_pcie.ncd
		case DEVICE_ID_IOXT:				return "top_IO_TX";			//	top_IO_TX.ncd
		case DEVICE_ID_KONALHEPLUS:			return "lhe_12_pcie";		//	lhe_12_pcie.ncd
		case DEVICE_ID_KONALHI:				return "top_pike";			//	top_pike.ncd
		case DEVICE_ID_TTAP:				return "t_tap_top";			//	t_tap_top.ncd
		case DEVICE_ID_CORVIDHBR:			return "corvid_hb_r";		//	corvidhb-r
		case DEVICE_ID_IO4KPLUS:			return "io4kp";
		case DEVICE_ID_IOIP_2022:			return "ioip_s2022";
		case DEVICE_ID_KONAIP_2110:			return "konaip_s2110";
		case DEVICE_ID_KONAIP_2110_RGB12:	return "konaip_s2110_RGB12";
		case DEVICE_ID_IOIP_2110:			return "ioip_s2110";
		case DEVICE_ID_IOIP_2110_RGB12:		return "ioip_s2110_RGB12";
		case DEVICE_ID_KONA1:				return "kona1";
		case DEVICE_ID_KONAHDMI:			return "kona_hdmi_4rx";
		case DEVICE_ID_KONA5:				return "kona5_retail";
		case DEVICE_ID_KONA5_8KMK:			return "kona5_8k_mk";
		case DEVICE_ID_KONA5_8K:			return "kona5_8k";
		case DEVICE_ID_KONA5_2X4K:			return "kona5_2x4k";
		case DEVICE_ID_KONA5_3DLUT:			return "kona5_3d_lut";
		case DEVICE_ID_KONA5_OE1:			return "kona5_oe_cfg1";
		case DEVICE_ID_KONA5_OE2:			return "kona5_oe_cfg2";
		case DEVICE_ID_KONA5_OE3:			return "kona5_oe_cfg3";
		case DEVICE_ID_KONA5_OE4:			return "kona5_oe_cfg4";
		case DEVICE_ID_KONA5_OE5:			return "kona5_oe_cfg5";
		case DEVICE_ID_KONA5_OE6:			return "kona5_oe_cfg6";
		case DEVICE_ID_KONA5_OE7:			return "kona5_oe_cfg7";
		case DEVICE_ID_KONA5_OE8:			return "kona5_oe_cfg8";
		case DEVICE_ID_KONA5_OE9:			return "kona5_oe_cfg9";
		case DEVICE_ID_KONA5_OE10:			return "kona5_oe_cfg10";
		case DEVICE_ID_KONA5_OE11:			return "kona5_oe_cfg11";
		case DEVICE_ID_KONA5_OE12:			return "kona5_oe_cfg12";
		case DEVICE_ID_CORVID44_8KMK:		return "c44_12g_8k_mk";
		case DEVICE_ID_CORVID44_8K:			return "c44_12g_8k";
		case DEVICE_ID_CORVID44_2X4K:		return "c44_12g_2x4k";
		case DEVICE_ID_CORVID44_PLNR:		return "c44_12g_plnr";
		case DEVICE_ID_TTAP_PRO:			return "t_tap_pro";
		case DEVICE_ID_IOX3:				return "iox3";
		default:							break;
	}
	return "";
}

bool CNTV2Bitfile::CanFlashDevice (const NTV2DeviceID inDeviceID) const
{
	if (IsPartial () || IsClear ())
		return false;
	
	if (_designName == ::NTV2GetPrimaryHardwareDesignName (inDeviceID))
		return true;

	//	Special cases -- e.g. bitfile flipping, P2P, etc...
	switch (inDeviceID)
	{
		case DEVICE_ID_CORVID44:			return ::NTV2GetPrimaryHardwareDesignName(DEVICE_ID_CORVID44) == _designName
													|| _designName == "corvid_446";	//	Corvid 446
		case DEVICE_ID_KONA3GQUAD:			return ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA3G) == _designName
													|| _designName == "K3G_quad_p2p";	//	K3G_quad_p2p.ncd
		case DEVICE_ID_KONA3G:				return ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA3GQUAD) == _designName
													|| _designName == "K3G_p2p";		//	K3G_p2p.ncd

		case DEVICE_ID_KONA4:				return ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA4UFC) == _designName;
		case DEVICE_ID_KONA4UFC:			return ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA4) == _designName;

		case DEVICE_ID_IO4K:				return ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_IO4KUFC) == _designName;
		case DEVICE_ID_IO4KUFC:				return ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_IO4K) == _designName;

		case DEVICE_ID_CORVID88:			return ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_CORVID88) == _designName
													|| _designName == "CORVID88"
													|| _designName == "corvid88_top";
		case DEVICE_ID_CORVIDHBR:			return ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_CORVIDHBR) == _designName
													|| _designName == "ZARTAN";
		case DEVICE_ID_IO4KPLUS:			return ::NTV2GetPrimaryHardwareDesignName(DEVICE_ID_IO4KPLUS) == _designName;
		case DEVICE_ID_IOIP_2022:			return ::NTV2GetPrimaryHardwareDesignName(DEVICE_ID_IOIP_2022) == _designName;
		case DEVICE_ID_KONAIP_2110:			return ::NTV2GetPrimaryHardwareDesignName(DEVICE_ID_KONAIP_2110) == _designName;
		case DEVICE_ID_KONAIP_2110_RGB12:	return ::NTV2GetPrimaryHardwareDesignName(DEVICE_ID_KONAIP_2110_RGB12) == _designName;
		case DEVICE_ID_IOIP_2110:			return ::NTV2GetPrimaryHardwareDesignName(DEVICE_ID_IOIP_2110) == _designName;
		case DEVICE_ID_IOIP_2110_RGB12:		return ::NTV2GetPrimaryHardwareDesignName(DEVICE_ID_IOIP_2110_RGB12) == _designName;
		case DEVICE_ID_KONAHDMI:			return ::NTV2GetPrimaryHardwareDesignName(DEVICE_ID_KONAHDMI) == _designName
													|| _designName == "Corvid_HDMI_4Rx_Top";
		case DEVICE_ID_KONA5_OE1:
		case DEVICE_ID_KONA5_OE2:
		case DEVICE_ID_KONA5_OE3:
		case DEVICE_ID_KONA5_OE4:
		case DEVICE_ID_KONA5_OE5:
		case DEVICE_ID_KONA5_OE6:
		case DEVICE_ID_KONA5_OE7:
		case DEVICE_ID_KONA5_OE8:
		case DEVICE_ID_KONA5_OE9:
		case DEVICE_ID_KONA5_OE10:
		case DEVICE_ID_KONA5_OE11:
		case DEVICE_ID_KONA5_OE12:
		case DEVICE_ID_KONA5_3DLUT:
		case DEVICE_ID_KONA5_2X4K:
		case DEVICE_ID_KONA5_8KMK:
		case DEVICE_ID_KONA5_8K:
        case DEVICE_ID_KONA5:		return ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA5) == _designName
                                            || _designName == ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_8KMK)
											|| _designName == "kona5"
											|| _designName == ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_8K)
											|| _designName == ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_3DLUT)
											|| _designName == ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_2X4K)
											|| _designName == ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_OE1)
											|| _designName == ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_OE2).append("_tprom")
											|| _designName == ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_OE3).append("_tprom")
											|| _designName == ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_OE4).append("_tprom")
											|| _designName == ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_OE5).append("_tprom")
											|| _designName == ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_OE6).append("_tprom")
											|| _designName == ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_OE7).append("_tprom")
											|| _designName == ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_OE8).append("_tprom")
											|| _designName == ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_OE9).append("_tprom")
											|| _designName == ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_OE10).append("_tprom")
											|| _designName == ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_OE11).append("_tprom")
											|| _designName == ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_OE12).append("_tprom")
											|| _designName == ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA5).append("_tprom")
											|| _designName == ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_8KMK).append("_tprom")
											|| _designName == ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_8K).append("_tprom")
											|| _designName == ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_3DLUT).append("_tprom")
											|| _designName == ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_2X4K).append("_tprom")
											|| _designName == ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_OE1).append("_tprom");
		case DEVICE_ID_CORVID44_8KMK:
		case DEVICE_ID_CORVID44_8K:
		case DEVICE_ID_CORVID44_PLNR:
		case DEVICE_ID_CORVID44_2X4K: return ::NTV2GetPrimaryHardwareDesignName(DEVICE_ID_CORVID44_8KMK) == _designName
											|| _designName == ::NTV2GetPrimaryHardwareDesignName(DEVICE_ID_CORVID44_8K)
											|| _designName == ::NTV2GetPrimaryHardwareDesignName(DEVICE_ID_CORVID44_2X4K)
											|| _designName == ::NTV2GetPrimaryHardwareDesignName(DEVICE_ID_CORVID44_PLNR)
											|| _designName == "c44_12g";
		case DEVICE_ID_TTAP_PRO:		return ::NTV2GetPrimaryHardwareDesignName (DEVICE_ID_TTAP_PRO) == _designName;
		default:					break;
	}
	return false;
}

typedef map <string, NTV2DeviceID>			DesignNameToIDMap;
typedef DesignNameToIDMap::iterator			DesignNameToIDIter;
typedef DesignNameToIDMap::const_iterator	DesignNameToIDConstIter;	
static DesignNameToIDMap					sDesignNameToIDMap;

class CDesignNameToIDMapMaker
{
	public:

		CDesignNameToIDMapMaker ()
		{
			assert (sDesignNameToIDMap.empty ());
			const NTV2DeviceIDSet goodDeviceIDs (::NTV2GetSupportedDevices ());
			for (NTV2DeviceIDSetConstIter iter (goodDeviceIDs.begin ());  iter != goodDeviceIDs.end ();  ++iter)
				sDesignNameToIDMap[::NTV2GetPrimaryHardwareDesignName (*iter)] = *iter;
			sDesignNameToIDMap["K3G_quad_p2p"] = DEVICE_ID_KONA3GQUAD;	//	special case
			sDesignNameToIDMap["K3G_p2p"] = DEVICE_ID_KONA3G;			//	special case
			sDesignNameToIDMap["CORVID88"] = DEVICE_ID_CORVID88;		//	special case
			sDesignNameToIDMap["ZARTAN"] = DEVICE_ID_CORVIDHBR;			//	special case
		}

		~CDesignNameToIDMapMaker ()
		{
			sDesignNameToIDMap.clear ();
		}

		static NTV2DeviceID DesignNameToID (const string & inDesignName)
		{
			assert (!sDesignNameToIDMap.empty ());
			const DesignNameToIDConstIter	iter	(sDesignNameToIDMap.find (inDesignName));
			return iter != sDesignNameToIDMap.end () ? iter->second : DEVICE_ID_NOTFOUND;
		}
};

static CDesignNameToIDMapMaker				sDesignNameToIDMapMaker;

typedef pair <ULWord, ULWord>				DesignPair;
typedef map <DesignPair, NTV2DeviceID>		DesignPairToIDMap;
typedef DesignPairToIDMap::const_iterator	DesignPairToIDMapConstIter;
static DesignPairToIDMap					sDesignPairToIDMap;

class CDesignPairToIDMapMaker
{
public:

	CDesignPairToIDMapMaker ()
		{
			assert (sDesignPairToIDMap.empty ());
			sDesignPairToIDMap[make_pair(0x01, 0x00)] = DEVICE_ID_KONA5;
			sDesignPairToIDMap[make_pair(0x01, 0x01)] = DEVICE_ID_KONA5_8KMK;
            sDesignPairToIDMap[make_pair(0x01, 0x02)] = DEVICE_ID_KONA5_8K;
            sDesignPairToIDMap[make_pair(0x01, 0x03)] = DEVICE_ID_KONA5_2X4K;
			sDesignPairToIDMap[make_pair(0x01, 0x04)] = DEVICE_ID_KONA5_3DLUT;
			sDesignPairToIDMap[make_pair(0x01, 0x05)] = DEVICE_ID_KONA5_OE1;
			sDesignPairToIDMap[make_pair(0x02, 0x00)] = DEVICE_ID_CORVID44_8KMK;
			sDesignPairToIDMap[make_pair(0x02, 0x01)] = DEVICE_ID_CORVID44_8K;
			sDesignPairToIDMap[make_pair(0x02, 0x02)] = DEVICE_ID_CORVID44_2X4K;
			sDesignPairToIDMap[make_pair(0x02, 0x03)] = DEVICE_ID_CORVID44_PLNR;
            sDesignPairToIDMap[make_pair(0x03, 0x03)] = DEVICE_ID_KONA5_2X4K;
			sDesignPairToIDMap[make_pair(0x03, 0x04)] = DEVICE_ID_KONA5_3DLUT;
			sDesignPairToIDMap[make_pair(0x03, 0x05)] = DEVICE_ID_KONA5_OE1;
			sDesignPairToIDMap[make_pair(0x03, 0x06)] = DEVICE_ID_KONA5_OE2;
			sDesignPairToIDMap[make_pair(0x03, 0x07)] = DEVICE_ID_KONA5_OE3;
			sDesignPairToIDMap[make_pair(0x03, 0x08)] = DEVICE_ID_KONA5_OE4;
			sDesignPairToIDMap[make_pair(0x03, 0x09)] = DEVICE_ID_KONA5_OE5;
			sDesignPairToIDMap[make_pair(0x03, 0x0A)] = DEVICE_ID_KONA5_OE6;
			sDesignPairToIDMap[make_pair(0x03, 0x0B)] = DEVICE_ID_KONA5_OE7;
			sDesignPairToIDMap[make_pair(0x03, 0x0C)] = DEVICE_ID_KONA5_OE8;
			sDesignPairToIDMap[make_pair(0x03, 0x0D)] = DEVICE_ID_KONA5_OE9;
			sDesignPairToIDMap[make_pair(0x03, 0x0E)] = DEVICE_ID_KONA5_OE10;
			sDesignPairToIDMap[make_pair(0x03, 0x0F)] = DEVICE_ID_KONA5_OE11;
			sDesignPairToIDMap[make_pair(0x03, 0x10)] = DEVICE_ID_KONA5_OE12;
        }
	~CDesignPairToIDMapMaker ()
		{
			sDesignPairToIDMap.clear ();
		}
	static NTV2DeviceID DesignPairToID(ULWord designID, ULWord bitfileID)
		{
			assert (!sDesignPairToIDMap.empty ());
			const DesignPairToIDMapConstIter	iter	(sDesignPairToIDMap.find (make_pair(designID, bitfileID)));
			return iter != sDesignPairToIDMap.end () ? iter->second : DEVICE_ID_NOTFOUND;
		}
	static ULWord DeviceIDToDesignID(NTV2DeviceID deviceID)
		{
			assert (!sDesignPairToIDMap.empty ());
			DesignPairToIDMapConstIter iter;

			for (iter = sDesignPairToIDMap.begin(); iter != sDesignPairToIDMap.end(); ++iter)
			{
				if (iter->second == deviceID)
					return iter->first.first;
			}

			return 0;
		}
	static ULWord DeviceIDToBitfileID(NTV2DeviceID deviceID)
		{
			assert (!sDesignPairToIDMap.empty ());
			DesignPairToIDMapConstIter iter;

			for (iter = sDesignPairToIDMap.begin(); iter != sDesignPairToIDMap.end(); ++iter)
			{
				if (iter->second == deviceID)
					return iter->first.second;
			}

			return 0;
		}
};

static CDesignPairToIDMapMaker 				sDesignPairToIDMapMaker;

NTV2DeviceID CNTV2Bitfile::GetDeviceID (void) const
{
	if ((_userID != 0) && (_userID != 0xffffffff))
	{
		return sDesignPairToIDMapMaker.DesignPairToID(_designID, _bitfileID);
	}
	
	return sDesignNameToIDMapMaker.DesignNameToID (_designName);
}

NTV2DeviceID CNTV2Bitfile::ConvertToDeviceID (const ULWord designID, const ULWord bitfileID)
{
	return sDesignPairToIDMapMaker.DesignPairToID (designID, bitfileID);
}

ULWord CNTV2Bitfile::ConvertToDesignID (const NTV2DeviceID deviceID)
{
	return sDesignPairToIDMapMaker.DeviceIDToDesignID(deviceID);
}

ULWord CNTV2Bitfile::ConvertToBitfileID (const NTV2DeviceID deviceID)
{
	return sDesignPairToIDMapMaker.DeviceIDToBitfileID(deviceID);
}
