/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2rp188.h
	@brief		Declares the CRP188 class. See SMPTE RP188 standard for details.
	@copyright	(C) 2007-2021 AJA Video Systems, Inc.
**/

#ifndef __NTV2_RP188_
#define __NTV2_RP188_

#include <string>
#include <sstream>

#include "ajaexport.h"
#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2videodefines.h"
#include "ntv2publicinterface.h"



#if defined(AJALinux)
# include <stdlib.h>		/* malloc/free */
# include <string.h>		/* memset, memcpy */
#endif


typedef enum
{
	kTCFormatUnknown,
	kTCFormat24fps,
	kTCFormat25fps,
	kTCFormat30fps,
	kTCFormat30fpsDF,
	kTCFormat48fps,
	kTCFormat50fps,
	kTCFormat60fps,
	kTCFormat60fpsDF
} TimecodeFormat;

typedef enum
{
	kTCBurnTimecode,		// display current timecode
	kTCBurnUserBits,		// display current user bits
	kTCBurnFrameCount,		// display current frame count
	kTCBurnBlank			// display --:--:--:--
} TimecodeBurnMode;


const int64_t kDefaultFrameCount = 0x80000000;


//--------------------------------------------------------------------------------------------------------------------
//	class CRP188
//--------------------------------------------------------------------------------------------------------------------
class AJAExport CRP188
{
public:
    // Constructors
						CRP188 ();
						CRP188 (const RP188_STRUCT & rp188, const TimecodeFormat tcFormat = kTCFormat30fps);
						CRP188 (const NTV2_RP188 & rp188, const TimecodeFormat tcFormat = kTCFormat30fps);
						CRP188 (const std::string & sRP188, const TimecodeFormat tcFormat = kTCFormat30fps);
						CRP188 (ULWord ulFrms, ULWord ulSecs, ULWord ulMins, ULWord ulHrs, const TimecodeFormat tcFormat = kTCFormat30fps);
						CRP188 (ULWord frames, const TimecodeFormat tcFormat = kTCFormat30fps);
	virtual 			~CRP188();
	void				Init ();
	bool				operator==( const CRP188& s);

    // Setters
	void				SetRP188 (ULWord ulFrms, ULWord ulSecs, ULWord ulMins, ULWord ulHrs,
									NTV2FrameRate frameRate, const bool bDropFrame = false, const bool bSMPTE372 = false);
    void				SetRP188 (const RP188_STRUCT & rp188, const TimecodeFormat tcFormat = kTCFormatUnknown);
    void				SetRP188 (const NTV2_RP188 & rp188, const TimecodeFormat tcFormat = kTCFormatUnknown);
    void				SetRP188 (const std::string &sRP188, const TimecodeFormat tcFormat = kTCFormatUnknown);
    void				SetRP188 (ULWord ulFrms, ULWord ulSecs, ULWord ulMins, ULWord ulHrs, const TimecodeFormat tcFormat = kTCFormatUnknown);
	void				SetRP188 (ULWord frames, const TimecodeFormat tcFormat = kTCFormatUnknown);

	void				SetDropFrame (bool bDropFrameFlag);
	void				SetColorFrame (bool bColorFrameFlag);
	void				SetVaricamFrameActive (bool bVaricamActive, ULWord frame);
	void				SetVaricamRate (NTV2FrameRate frameRate);
	void				SetFieldID (ULWord fieldID);
	bool				GetFieldID (void);
	void				SetBFGBits (bool bBFG0, bool bBFG1, bool bBFG2);
	void				SetSource (UByte src);
	void				SetOutputFilter (UByte src);

    // Getters
    bool				GetRP188Str  (std::string & sRP188) const;
	const char *		GetRP188CString () const;
    bool				GetRP188Secs (ULWord & ulSecs) const;
    bool				GetRP188Frms (ULWord & ulFrms) const;
    bool				GetRP188Mins (ULWord & ulMins) const;
    bool				GetRP188Hrs  (ULWord & ulHrs) const;
    bool				GetRP188Reg  (RP188_STRUCT & outRP188) const;
    bool				GetRP188Reg  (NTV2_RP188 & outRP188) const;
	bool				GetFrameCount (ULWord & frameCount);
	ULWord				MaxFramesPerDay (TimecodeFormat format = kTCFormatUnknown) const;
	void				ConvertTimecode (ULWord & frameCount, TimecodeFormat format, ULWord hours, ULWord minutes, ULWord seconds, ULWord frames);
	void				ConvertFrameCount (ULWord frameCount, TimecodeFormat format, ULWord & hours, ULWord & minutes, ULWord & seconds, ULWord & frames);
	ULWord				AddFrames (ULWord frameCount);
	ULWord				SubtractFrames (ULWord frameCount);
	bool				GetRP188UserBitsStr (std::string & sRP188UB);
	const char *		GetRP188UserBitsCString ();
	UByte				GetSource () const ;
	UByte				GetOutputFilter () const ;
	TimecodeFormat		GetTimecodeFormat() { return _tcFormat; }

	// these tests are ONLY valid if the CRP188 object is instantiated or set with an RP188_STRUCT
	bool				IsFreshRP188 (void)
							{ return(_bFresh); }					// true if RP188 data is fresh this past frame
	bool				VaricamFrame0 (void)
							{ return(_bVaricamActiveF0); }			// true if Varicam "Frame0 Active" bit is set
	bool				VaricamFrame1 (void)
							{ return(_bVaricamActiveF1); }			// true if Varicam "Frame1 Active" bit is set
	ULWord				VaricamFrameRate (void);
	NTV2FrameRate		defaultFrameRateForTimecodeFormat (TimecodeFormat format = kTCFormatUnknown) const;
	bool				FormatIsDropFrame (TimecodeFormat format = kTCFormatUnknown) const;
	bool				FormatIs60_50fps (TimecodeFormat format = kTCFormatUnknown) const;
	bool				FormatIsPAL (TimecodeFormat format = kTCFormatUnknown) const;

	ULWord				FieldID (void)
							{ return(_fieldID); }					// fieldID bit
	bool				DropFrame (void)
							{ return(_bDropFrameFlag);  }			// drop frame bit
	bool				ColorFrame (void)
							{ return(_bColorFrameFlag); }			// color frame bit
	ULWord				BinaryGroup (ULWord smpteNum);

	// For historical reasons, calling SetRP188 clears the user bits, so a call to either of these two functions
	// should be done after the timecode value is set
	bool				SetBinaryGroup (ULWord smpteNum, ULWord bits);
	bool				SetUserBits (ULWord bits);					// eight groups of four bits each

	ULWord				UDW (ULWord smpteUDW);
	ULWord				FramesPerSecond (TimecodeFormat format = kTCFormatUnknown) const;
	NTV2FrameRate		DefaultFrameRateForTimecodeFormat (TimecodeFormat format = kTCFormatUnknown) const;

    // Modifiers
	bool				InitBurnIn (NTV2FrameBufferFormat frameBufferFormat, NTV2FrameDimensions frameDimensions, LWord percentY = 0);
	void				writeV210Pixel (char **pBytePtr, int x, int c, int y);
	bool				BurnTC (char *pBaseVideoAddress, int rowBytes, TimecodeBurnMode burnMode, int64_t frameCount = kDefaultFrameCount, bool bDisplay60_50fpsAs30_25 = false);
	void				CopyDigit (char *pDigit, int digitWidth, int digitHeight, char *pFrameBuff, int fbRowBytes);
	std::string			GetTimeCodeString(bool bDisplay60_50fpsAs30_25 = false);
	
private:
    void				ConvertTcStrToVal (void);   // converts _sHMSF to _ulVal
    void				ConvertTcStrToReg (void);   // converts _sHMSF to _rp188
	void				RP188ToUserBits (void);		// derives _ulUserBits and _sUserBits from RP188 struct


private:
	TimecodeFormat		_tcFormat;			// fps, drop- or non-drop frame
    bool				_bInitialized;		// if constructed with no args, set to false
	bool				_bFresh;			// true if hardware told us this was new ANC data (not just what was lying around in the registers)
	//bool				_bDropFrame;		// we have to be told whether we are df or ndf
	//bool				_b50Hz;				// if true, interpret FieldID and Binary Group Flags per 50 Hz spec

	// RP188 user bits
	bool				_bVaricamActiveF0;  // Varicam "Active Frame 0" user bit flag
	bool				_bVaricamActiveF1;  // Varicam "Active Frame 1" user bit flag
	ULWord				_fieldID;			// FieldID bit: '0' or '1'
	bool				_bDropFrameFlag;	// Drop Frame bit: '0' or '1'
	bool				_bColorFrameFlag;   // Color Frame bit: '0' or '1'
	ULWord				_varicamRate;		// Varicam rate expressed as bits [0..3] 1 units, bits [4..7] tens unit.

    std::string			_sHMSF;				// hour:minute:second:frame in string format
	std::string			_sUserBits;			// Binary Groups 8-1 in string format
    ULWord				_ulVal[4];			// [0]=frame, [1]=seconds, etc.
	ULWord				_ulUserBits[8];		// [0] = Binary Group 1, [1] = Binary Group 2, etc. (note: SMPTE labels them 1 - 8)
    RP188_STRUCT		_rp188;				// AJA native format

	bool				_bRendered;			// set 'true' when Burn-In character map has been rendered
	char *				_pCharRenderMap;	// ptr to rendered Burn-In character set
	NTV2FrameBufferFormat _charRenderFBF;	// frame buffer format of rendered characters
	ULWord				_charRenderHeight;	// frame height for which rendered characters were rendered
	ULWord				_charRenderWidth;	// frame width for which rendered characters were rendered
	int					_charWidthBytes;	// rendered character width in bytes
	int					_charHeightLines;	// rendered character height in frame lines
	int					_charPositionX;		// offset (in bytes) from left side of screen to first burn-in character
	int					_charPositionY;		// offset (in lines) from top of screen to top of burn-in characters
};	//	CRP188

/**
	@brief		Prints the given CRP188's contents into the given output stream.
	@param		outputStream	The stream into which the human-readable timecode will be written.
	@param[in]	inObj			Specifies the CRP188 instance to be streamed.
	@return		The ostream that was specified.
**/
AJAExport std::ostream & operator << (std::ostream & outputStream, const CRP188 & inObj);

#endif	//	__NTV2_RP188_
