/* SPDX-License-Identifier: MIT */
/**
	@file		timecode.cpp
	@brief		Implements the AJATimeCode class.
	@copyright	(C) 2010-2021 AJA Video Systems, Inc.  All rights reserved.
**/

//---------------------------------------------------------------------------------------------------------------------
//  Includes
//---------------------------------------------------------------------------------------------------------------------
#include "ajabase/common/common.h"
#include "ajabase/common/timecode.h"

#include <iomanip>
#include <string>
#include <vector>
using namespace std;

#if defined(AJA_MAC) || defined(AJA_LINUX)
#include <stdlib.h>
#include <string.h>
#include <wctype.h>
#endif

//---------------------------------------------------------------------------------------------------------------------
//  Defines and structures
//---------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------
//  Utility Functions
//---------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------
//  Public Functions and Class Methods
//---------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------
//  Name:   AJATimeCode
// Notes:   http://www.andrewduncan.ws/Timecodes/Timecodes.html
//          http://en.wikipedia.org/wiki/SMPTE_time_code
//          Drop frame is lifted from http://www.davidheidelberger.com/blog/?p=29
//---------------------------------------------------------------------------------------------------------------------
AJATimeCode::AJATimeCode() :
	m_frame(0),
	m_stdTimecodeForHfr(true)
{
}

AJATimeCode::AJATimeCode(uint32_t frame) :
	m_stdTimecodeForHfr(true)
{
	Set(frame);
}

AJATimeCode::AJATimeCode(const std::string &str, const AJATimeBase& timeBase, bool bDropFrame, bool bStdTc)
    : m_stdTimecodeForHfr(bStdTc)
{
    Set(str.c_str(), timeBase, bDropFrame);
}

AJATimeCode::AJATimeCode(const std::string &str, const AJATimeBase& timeBase)
    : m_stdTimecodeForHfr(true)
{
    Set(str.c_str(), timeBase);
}

AJATimeCode::AJATimeCode(const AJATimeCode& other)
{
	m_frame = other.m_frame;
	m_stdTimecodeForHfr = other.m_stdTimecodeForHfr;
}

//---------------------------------------------------------------------------------------------------------------------
//  Name:   ~AJATimeCode
//---------------------------------------------------------------------------------------------------------------------
AJATimeCode::~AJATimeCode()
{
} //end ~AJATimeCode

bool AJATimeCode::QueryIsDropFrame(const string &str)
{
	bool bHasSemicolon = false;
    if (str.find(";", 0) != string::npos)
		bHasSemicolon = true;
	return bHasSemicolon;
}

int AJATimeCode::QueryStringSize(void)
{
	return (int)::strlen("00:00:00:00") + 1;
}

uint32_t AJATimeCode::QueryFrame(void) const
{
	return m_frame;
}

inline uint32_t AJATimeCodeRound(double f)
{
	return (uint32_t)(f + .5);
}

inline int64_t AJATimeCodeAbs(int64_t x)
{
    return (((x)>=0)?(x):-(x));
}

void AJATimeCode::QueryHmsf(uint32_t &h, uint32_t &m, uint32_t &s, uint32_t &f, const AJATimeBase& timeBase, bool bDropFrame) const
{
	// This code was pulled from the NTV2RP188 class and appears to work correctly.
	int64_t frame, frameRate,frameDuration;
	timeBase.GetFrameRate(frameRate,frameDuration);
	AJA_FrameRate ajaFrameRate = timeBase.GetAJAFrameRate();
	
	frame = m_frame;
	if (ajaFrameRate >= AJA_FrameRate_4795 && m_stdTimecodeForHfr == true)
	{
		frame /= 2;
		frameRate /= 2;
	}
	
	if ((frameRate == 0) || (frameDuration == 0))
	{
		h = m = s = f = 0;
	}
	else if (frameRate < frameDuration)
	{
		h = m = s = f = 0;
	}
	else
	{
		// this is just good for 29.97, 59.94, 23.976
		double dFrameRate = double(frameRate) / double(frameDuration);
		
		// non-dropframe
		uint32_t framesPerSec = AJATimeCodeRound(dFrameRate);
		uint32_t framesPerMin = framesPerSec * 60;				// 60 seconds/minute
		uint32_t framesPerHr  = framesPerMin * 60;				// 60 minutes/hr.
		uint32_t framesPerDay = framesPerHr  * 24;				// 24 hours/day
		
		if (! bDropFrame)
		{
			// make sure we don't have more than 24 hours worth of frames
			frame = frame % framesPerDay;
			
			// how many hours?
			h = uint32_t(frame / framesPerHr);
			frame = frame % framesPerHr;
			
			// how many minutes?
			m = uint32_t(frame / framesPerMin);
			frame = frame % framesPerMin;
			
			// how many seconds?
			s = uint32_t(frame / framesPerSec);
			
			// what's left is the frame count
			f = uint32_t(frame % framesPerSec);
		}
		else
		{
			// dropframe
			uint32_t droppedFrames			= AJATimeCodeRound(dFrameRate * .066666);	// number of frames dropped in a "drop second"
			uint32_t dropFramesPerSec		= framesPerSec - droppedFrames;
			uint32_t dropframesPerMin		= (59 * framesPerSec) + dropFramesPerSec;	// every minute we get 1 drop and 59 regular seconds
			uint32_t dropframesPerTenMin	= (9 * dropframesPerMin) + framesPerMin;	// every ten minutes we get 1 regular and 9 drop minutes
			uint32_t dropframesPerHr		= dropframesPerTenMin * 6;					// 60 minutes/hr.
			uint32_t dropframesPerDay		= dropframesPerHr * 24;						// 24 hours/day
			
			// make sure we don't have more than 24 hours worth of frames
			frame = frame % dropframesPerDay;
			
			// how many hours?
			h  = uint32_t(frame / dropframesPerHr);
			frame = frame % dropframesPerHr;
			
			// how many tens of minutes?
			m = uint32_t(10 * (frame / dropframesPerTenMin));
			frame = frame % dropframesPerTenMin;
			
			// how many units of minutes?
			if (frame >= framesPerMin)
			{
				m += 1;	// got at least one minute (the first one is a non-drop minute)
				frame  = frame - framesPerMin;
				
				// any remaining minutes are drop-minutes
				m += uint32_t(frame / dropframesPerMin);
				frame  = frame % dropframesPerMin;
			}
			
			// how many seconds? depends on whether this was a regular or a drop minute...
			s = 0;
			if (m % 10 == 0)
			{
				// regular minute: all seconds are full length
				s = uint32_t(frame / framesPerSec);
				frame = frame % framesPerSec;
			}
			else
			{
				// drop minute: the first second is a drop second
				if (frame >= dropFramesPerSec)
				{
					s += 1;	// got at least one (the first one is a drop second)
					frame = frame - dropFramesPerSec;
					
					// any remaining seconds are full-length
					s += uint32_t(frame / framesPerSec);
					frame = frame % framesPerSec;
				}
			}
			
			// what's left is the frame count
			f = uint32_t(frame);
			
			// if we happened to land on a drop-second, add 2 frames (the 28 frames are numbered 2 - 29, not 0 - 27)
			if ( (s == 0) && (m % 10 != 0))
				f += droppedFrames;
		}
	}
}

void AJATimeCode::QueryString(std::string &str, const AJATimeBase& timeBase, bool bDropFrame)
{
    uint32_t h = 0,m = 0,s = 0,f = 0;
    QueryHmsf(h,m,s,f,timeBase,bDropFrame);

    std::ostringstream oss;
    if (bDropFrame)
    {
        oss << setfill('0') << setw(2) << h << ":"
            << setfill('0') << setw(2) << m << ":"
            << setfill('0') << setw(2) << s << ";"
            << setfill('0') << setw(2) << f;
    }
    else
    {
        oss << setfill('0') << setw(2) << h << ":"
            << setfill('0') << setw(2) << m << ":"
            << setfill('0') << setw(2) << s << ":"
            << setfill('0') << setw(2) << f;
    }
    str.assign(oss.str());
}

void AJATimeCode::QueryString(char *pString, const AJATimeBase& timeBase, bool bDropFrame)
{
    string s;
    QueryString(s, timeBase, bDropFrame);
    strncpy(pString, s.c_str(), s.length());
    pString[11] = '\0';
}

int AJATimeCode::QuerySMPTEStringSize(void)
{
	return 4;
}

void AJATimeCode::QuerySMPTEString(char *pBufr,const AJATimeBase& timeBase,bool bDrop)
{
	uint32_t h=0, m=0, s=0, f=0;
    QueryHmsf(h,m,s,f,timeBase,bDrop);
	
	pBufr[0] = ((f/10) << 4) + (f % 10);
	pBufr[1] = ((s/10) << 4) + (s % 10);
	pBufr[2] = ((m/10) << 4) + (m % 10);
	pBufr[3] = ((h/10) << 4) + (h % 10);
	if (bDrop)
		pBufr[0] = pBufr[0] | 0x40;
}

//---------------------------------------------------------------------------------------------------------------------
//  Name:   Set
//  Notes:  If we need to either clamp or roll over the frame number, the uint32_t version of Set() is a good place
//          to do it.
//---------------------------------------------------------------------------------------------------------------------
void AJATimeCode::Set(uint32_t frame)
{
	m_frame = frame;
}

void AJATimeCode::SetHmsf(uint32_t h, uint32_t m, uint32_t s, uint32_t f, const AJATimeBase& timeBase, bool bDropFrame)
{
	int64_t frameRate, frameRate2, frameDuration;
	timeBase.GetFrameRate(frameRate, frameDuration);
	AJA_FrameRate ajaFrameRate = timeBase.GetAJAFrameRate();
	
	frameRate2 = frameRate;
	if (ajaFrameRate >= AJA_FrameRate_4795 && m_stdTimecodeForHfr == true)
	{
		frameRate2 /=2;
	}
	
	uint32_t frame;
	if ((frameRate == 0) || (frameDuration == 0))
	{
		frame = 0;
	}
	else if (bDropFrame)
	{
		// this is just good for 29.97, 59.94, 23.976
		double dFrameRate   = double(frameRate2) / double(frameDuration);
		uint32_t dropFrames = AJATimeCodeRound(dFrameRate*.066666);		//Number of drop frames is 6% of framerate rounded to nearest integer
		uint32_t tb         = AJATimeCodeRound(dFrameRate);				//We don't need the exact framerate anymore, we just need it rounded to nearest integer
		
		uint32_t hourFrames		= tb*60*60;								//Number of frames per hour (non-drop)
		uint32_t minuteFrames	= tb*60;								//Number of frames per minute (non-drop)
		uint32_t totalMinutes	= (60*h) + m;							//Total number of minutes
		
		// if TC does not exist then we need to round up to the next valid frame
		if ( (s == 0) && ((m % 10) > 0) && ((f & ~1) == 0))
			f = 2;
		
		frame = ((hourFrames * h) + (minuteFrames * m) + (tb * s) + f) - (dropFrames * (totalMinutes - (totalMinutes / 10)));
	}
	else
	{
		double dFrameRate   = double(frameRate2) / double(frameDuration);
		uint32_t tb         = AJATimeCodeRound(dFrameRate);         //We don't need the exact framerate anymore, we just need it rounded to nearest integer
		
		uint32_t hourFrames   = tb*60*60;							//Number of frames per hour (non-drop)
		uint32_t minuteFrames = tb*60;								//Number of frames per minute (non-drop)
		//uint32_t totalMinutes = (60*h) + m;						//Total number of minutes
		frame = ((hourFrames * h) + (minuteFrames * m) + (tb * s) + f);
	}
	
	if (ajaFrameRate >= AJA_FrameRate_4795 && m_stdTimecodeForHfr == true)
	{
		frame *=2;
	}
	
	Set(frame);
} //end SetHmsf

void AJATimeCode::Set(const std::string &str, const AJATimeBase& timeBase, bool bDropFrame)
{
    const int valCount = 4;
    uint32_t val[valCount];
    ::memset(val,0,sizeof(val));

    // work from bottom up so that partial time code
    // (ie. something like 10:02 rather than 00:00:10:02)
    // is handled
    size_t len = str.length();
    int valOffset = 0;
    int valMult   = 1;
    for (size_t i = 0; i < len; i++)
    {
        char theChar = str[len - i - 1];
        if (::isdigit(theChar))
        {
            val[valOffset] = val[valOffset] + ((theChar - '0') * valMult);
            valMult *= 10;
        }
        else
        {
            valOffset++;
            valMult = 1;
        }

        if (valOffset >= 4)
            break;
    }

    SetHmsf(val[3], val[2], val[1], val[0], timeBase, bDropFrame);
}

void AJATimeCode::Set(const std::string &str, const AJATimeBase& timeBase)
{
    bool bDropFrame = false;
    std::string::const_iterator it = str.begin();
    while(it != str.end())
    {
        if ((*it == ';') || (*it == '.'))
        {
            bDropFrame = true;
            break;
        }
        ++it;
    }
    Set(str, timeBase, bDropFrame);
}

void AJATimeCode::SetWithCleanup(const std::string &str, const AJATimeBase& timeBase, bool bDrop)
{
    if (str.empty())
        return;

    bool bHasMark = false;
    if ( (str.find(";", 0) != string::npos) || (str.find(":", 0) != string::npos) )
    {
        bHasMark = true;
    }

    if (bHasMark)
    {
        std::string tmp(str);
        aja::strip(tmp);

        if (tmp.length() > 11)
            tmp.resize(11);

        Set(tmp, timeBase);
    }
    else
    {
        std::string tmp;
        if (bDrop)
            tmp = "00:00:00;00";
        else
            tmp = "00:00:00:00";

        size_t len = str.length();
        int tgtOffset = 10;
        for (size_t i = 0; i < len; i++)
        {
            size_t srcOffset = len - i - 1;
            if ((str[srcOffset] >= '0') && (str[srcOffset] <= '9'))
            {
                tmp[tgtOffset] = str[srcOffset];
                tgtOffset--;
                if ((tgtOffset == 8) || (tgtOffset == 5) || (tgtOffset == 2))
                    tgtOffset--;

                if (tgtOffset < 0)
                    break;
            }
        }
        Set(tmp, timeBase);
    }
}

void AJATimeCode::SetSMPTEString(const char *pBufr, const AJATimeBase& timeBase)
{
	bool bDrop = false;
	if (pBufr[0] & 0x40)
		bDrop = true;
	
    uint32_t f = (((pBufr[0] & 0x30) >> 4) * 10) + (pBufr[0] & 0x0f);
    uint32_t s = (((pBufr[1] & 0x70) >> 4) * 10) + (pBufr[1] & 0x0f);
    uint32_t m = (((pBufr[2] & 0x70) >> 4) * 10) + (pBufr[2] & 0x0f);
    uint32_t h = (((pBufr[3] & 0x30) >> 4) * 10) + (pBufr[3] & 0x0f);
	
    SetHmsf(h,m,s,f,timeBase,bDrop);
}

bool AJATimeCode::QueryIsRP188DropFrame (const uint32_t inDBB, const uint32_t inLo, const uint32_t inHi)		//	STATIC
{
    AJA_UNUSED(inDBB);
    AJA_UNUSED(inHi);
	return (inLo >> 10) & 0x01;
}


void AJATimeCode::SetRP188 (const uint32_t inDBB, const uint32_t inLo, const uint32_t inHi, const AJATimeBase & inTimeBase)
{
	AJATimeBase tb25(25000,1000);
	AJATimeBase	tb50(50000,1000);
	AJATimeBase tb60(60000,1000);
	AJATimeBase tb5994(60000,1001);

	//	HRS
	const uint32_t h0 (((inHi >> 16) & 0xF)     );
	const uint32_t h1 (((inHi >> 24) & 0x3) * 10);
	//	MINS
	const uint32_t m0 (((inHi      ) & 0xF)     );
	const uint32_t m1 (((inHi >>  8) & 0x7) * 10);
	//	SECS
	const uint32_t s0 (((inLo >> 16) & 0xF)     );
	const uint32_t s1 (((inLo >> 24) & 0x7) * 10);
	//	FRAMES
	uint32_t f0(0);
	uint32_t f1(0);

	if (!m_stdTimecodeForHfr  &&  (inTimeBase.IsCloseTo(tb50) || inTimeBase.IsCloseTo(tb60) || inTimeBase.IsCloseTo(tb5994)))
	{
		// for frame rates > 39 fps, we need an extra bit for the frame "10s". By convention,
		// we use the field ID bit to be the LS bit of the three bit number.
		bool fieldID;
		
		// Note: FID is in different words for PAL & NTSC!
		if(inTimeBase.IsCloseTo(tb25) || inTimeBase.IsCloseTo(tb50))
			fieldID = ((inHi & (1u<<27)) != 0);
		else
			fieldID = ((inLo & (1u<<27)) != 0);

		//	Double the regular frame count and add fieldID...
		const uint32_t numFrames = (((((inLo >> 8) & 0x3) * 10)  +  (inLo & 0xF)) * 2)  +  uint32_t(fieldID);
		f0 = numFrames % 10;
		f1 = (numFrames / 10) * 10;
	}
	else
	{
		f0 = ((inLo     ) & 0xF);
		f1 = ((inLo >> 8) & 0x3) * 10;
	}

    SetHmsf (h0+h1, m0+m1, s0+s1, f0+f1, inTimeBase, AJATimeCode::QueryIsRP188DropFrame(inDBB, inLo, inHi));
}


void AJATimeCode::QueryRP188(uint32_t *pDbb, uint32_t *pLow, uint32_t *pHigh, const AJATimeBase& timeBase, bool bDrop)
{
	uint32_t dbb(0), low(0), high(0);
	QueryRP188(dbb, low, high, timeBase, bDrop);
	if (*pDbb)  *pDbb = dbb;
	if (*pLow)  *pLow = low;
	if (*pHigh) *pHigh = high;
}

void AJATimeCode::QueryRP188(uint32_t & outDBB, uint32_t & outLo, uint32_t & outHi, const AJATimeBase & timeBase, const bool bDrop)
{
    AJA_UNUSED(timeBase);
    AJA_UNUSED(bDrop);
    
	uint32_t dbb  = 0;
	uint32_t low  = 0;
	uint32_t high = 0;
	//	UNIMPLEMENTED -- FINISH
	outDBB = dbb;
	outLo = low;
	outHi = high;
}


//---------------------------------------------------------------------------------------------------------------------
//  Name:   = operator
//---------------------------------------------------------------------------------------------------------------------
AJATimeCode& AJATimeCode::operator=(const AJATimeCode &val)
{
	if (this != &val)
	{
		m_frame = val.m_frame;
		m_stdTimecodeForHfr = val.m_stdTimecodeForHfr;
	}
	return *this;
} //end '='

//---------------------------------------------------------------------------------------------------------------------
//  Name:   == operator
//---------------------------------------------------------------------------------------------------------------------
bool AJATimeCode::operator==(const AJATimeCode &val) const
{
	bool bIsSame = false;
	if (m_frame == val.m_frame)
	{
	    bIsSame = true;
	}
	return bIsSame;
}

//---------------------------------------------------------------------------------------------------------------------
//  Name:   < operator
//---------------------------------------------------------------------------------------------------------------------
bool AJATimeCode::operator<(const AJATimeCode &val) const
{
	bool bIsLess = (m_frame < val.m_frame);
	return bIsLess;
}

bool AJATimeCode::operator<(const int32_t val) const
{
	bool bIsLess = (m_frame < (uint32_t)val);
	return bIsLess;
}


//---------------------------------------------------------------------------------------------------------------------
//  Name:   > operator
//---------------------------------------------------------------------------------------------------------------------
bool AJATimeCode::operator>(const AJATimeCode &val) const
{
	bool bIsGreater = (m_frame > val.m_frame);
	return bIsGreater;
}

bool AJATimeCode::operator>(const int32_t val) const
{
	bool bIsGreater = (m_frame > (uint32_t)val);
	return bIsGreater;
}


//---------------------------------------------------------------------------------------------------------------------
//  Name:   != operator
//---------------------------------------------------------------------------------------------------------------------
bool AJATimeCode::operator!=(const AJATimeCode &val) const
{
	return !(*this == val);
}

//---------------------------------------------------------------------------------------------------------------------
//  Name:   += operator
//---------------------------------------------------------------------------------------------------------------------
AJATimeCode& AJATimeCode::operator+=(const AJATimeCode &val)
{
    m_frame += val.m_frame;
	return *this;
}

AJATimeCode& AJATimeCode::operator+=(const int32_t val)
{
	m_frame += val;
	return *this;
}

//---------------------------------------------------------------------------------------------------------------------
//  Name:   -= operator
//---------------------------------------------------------------------------------------------------------------------
AJATimeCode& AJATimeCode::operator-=(const AJATimeCode &val)
{
	if(val.m_frame > m_frame)
		m_frame = 0;
	else
		m_frame -= val.m_frame;
	
	return *this;
}

AJATimeCode& AJATimeCode::operator-=(const int32_t val)
{
	if((uint32_t)val > m_frame)
		m_frame = 0;
	else
		m_frame -= val;
	
	return *this;
}

//---------------------------------------------------------------------------------------------------------------------
//  Name:   + operator
//---------------------------------------------------------------------------------------------------------------------
const AJATimeCode AJATimeCode::operator+(const AJATimeCode &val) const
{
	return AJATimeCode(*this) += val;
}

const AJATimeCode AJATimeCode::operator+(const int32_t val) const
{
	return AJATimeCode(*this) += val;
}

//---------------------------------------------------------------------------------------------------------------------
//  Name:   - operator
//---------------------------------------------------------------------------------------------------------------------
const AJATimeCode AJATimeCode::operator-(const AJATimeCode &val) const
{
	return AJATimeCode(*this) -= val;
}

const AJATimeCode AJATimeCode::operator-(const int32_t val) const
{
	return AJATimeCode(*this) -= val;
}
