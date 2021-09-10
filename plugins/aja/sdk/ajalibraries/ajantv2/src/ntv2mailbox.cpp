/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2mailbox.cpp
	@brief		Implementation of CNTV2MailBox class.
	@copyright	(C) 2015-2021 AJA Video Systems, Inc.
**/

#include "ntv2mailbox.h"
#include "ntv2utils.h"
#include <string.h>

#if defined(AJA_MAC)
    #include <mach/mach_time.h>
    #include <CoreServices/CoreServices.h>
    static int64_t s_PerformanceFrequency;
    static bool s_bPerformanceInit = false;
#endif


#if defined(MSWindows)
    static LARGE_INTEGER s_PerformanceFrequency;
    static bool s_bPerformanceInit = false;
#endif

#if defined(AJALinux) || defined(AJA_LINUX)
    #include <unistd.h>
    #if (_POSIX_TIMERS > 0)
        #ifdef _POSIX_MONOTONIC_CLOCK
            #define AJA_USE_CLOCK_GETTIME
        #else
            #undef AJA_USE_CLOCK_GETTIME
        #endif
    #endif

    #ifdef AJA_USE_CLOCK_GETTIME
        #include <time.h>
    #else
        // Use gettimeofday - this is not really desirable
        #include <sys/time.h>
    #endif
#endif


CNTV2MailBox::CNTV2MailBox(CNTV2Card & device) : mDevice( device )
{

    bOffset = SAREK_MAILBOX;

    memset(rxBuf,0,sizeof(rxBuf));
    memset(txBuf,0,sizeof(txBuf));

    _seqNum = SEQNUM_MIN;
}

CNTV2MailBox::~CNTV2MailBox()
{
}

bool CNTV2MailBox::sendMsg(uint32_t timeout)
{
    // assumes pointer to a buf longer than the string;
    int byte_len = (int)strlen((char*)txBuf);
    int word_len = ((byte_len + 4) - (byte_len %4))/4;

    // write message
    bool rv = writeMB(0xffffffff);  // SOM
    if (!rv)
    {
        mIpErrorCode = NTV2IpErrWriteSOMToMB;
        return false;
    }
    rv = writeMB(nextSeqNum());  // sequence number
    if (!rv)
    {
        mIpErrorCode = NTV2IpErrWriteSeqToMB;
        return false;
    }
    rv = writeMB(word_len);    // write wordcount
    if (!rv)
    {
        mIpErrorCode = NTV2IpErrWriteCountToMB;
        return false;
    }

    uint32_t * pBuf = txBuf;
    for (int i = 0; i < word_len; i++)
    {
        writeMB( *pBuf++);    // write message
    }

    return rcvMsg(timeout);
}

bool CNTV2MailBox::sendMsg(char * msg, uint32_t timeout)
{
    // make local safe copy
    strncpy((char*)txBuf,msg,sizeof(txBuf));

    return sendMsg(timeout);
}

bool CNTV2MailBox::rcvMsg(uint32_t timeout)
{
    uint32_t * pBuf = (uint32_t*)rxBuf;
    memset(rxBuf,0,sizeof(rxBuf));

    // drain until next message (valid SAOM and valid seqNum)
    bool rv;
    bool valid = false;
    do
    {
        rv = waitSOM(timeout);
        if (!rv)
        {
            mIpErrorCode = NTV2IpErrTimeoutNoSOM;
            return false;
        }

        uint32_t seqNum;
        rv = readMB(seqNum);
        if (!rv)
        {
            mIpErrorCode = NTV2IpErrTimeoutNoSeq;
            return false;
        }
        if (seqNum == currentSeqNum())
        {
            valid = true;
        }
    } while (!valid);

    uint32_t count;
    rv = readMB(count);
    if (!rv)
    {
        mIpErrorCode = NTV2IpErrTimeoutNoBytecount;
        return false;
    }

    if (count > FIFO_SIZE)
    {
        mIpErrorCode = NTV2IpErrExceedsFifo;
        return false;
    }

    if (count == 0)
    {
        mIpErrorCode = NTV2IpErrNoResponseFromMB;
        return false;
    }

    for (uint32_t i=0; i<count; i++)
    {
        uint32_t val;
        readMB(val);
        *pBuf++ = val;
    }
    *pBuf = 0;

    return true;
}

bool CNTV2MailBox::writeMB(uint32_t val, uint32_t timeout)
{
    bool rv = waitTxReady(timeout);
    if (rv)
    {
        mDevice.WriteRegister(bOffset + MB_tWRDATA,val);
    }
    return rv;
}

bool CNTV2MailBox::readMB(uint32_t & val, uint32_t timeout)
{
    bool rv = waitRxReady(timeout);
    if (rv)
    {
         mDevice.ReadRegister(bOffset + MB_tRDDATA, val);
    }
    return rv;
}

bool  CNTV2MailBox::waitSOM(uint32_t timeout)
{
    // this keeps draining until a SOM is read or a timeout
    uint32_t SOM = 0;
    while (SOM != 0xffffffff)
    {
        bool rv = readMB(SOM,timeout);
        if (!rv)
        {
            return false;
        }
    }
    return true;
}

bool CNTV2MailBox::rxReady()
{
    bool empty = getStatus() & MBS_RX_EMPTY;
    return !empty;
}

bool CNTV2MailBox::waitRxReady(uint32_t timeout)
{
    startTimer();
    while (!rxReady())
    {
        if (getElapsedTime() > timeout)
        {
            return false;
        }
    }
    return true;
}

bool CNTV2MailBox::waitTxReady(uint32_t timeout)
{
    startTimer();
    while (getStatus() & MBS_TX_FULL)
    {
        if (getElapsedTime() > timeout)
        {
            return false;
        }
    }
    return true;
}

uint32_t CNTV2MailBox::getStatus()
{
    uint32_t val;
    mDevice.ReadRegister(bOffset + MB_tSTATUS, val);
    return val;
}

bool CNTV2MailBox::ReadChannelRegister (const ULWord inReg, ULWord & outValue, const ULWord inMask, const ULWord inShift)
{
    NTV2RegInfo	regInfo	(inReg, 0, inMask, inShift);
    bool rv = mDevice.BankSelectReadRegister (NTV2RegInfo (chanOffset, chanNumber), regInfo);
    if (rv)
        outValue = regInfo.registerValue;
    return rv;
}

bool CNTV2MailBox::WriteChannelRegister(ULWord reg, ULWord value, ULWord mask, ULWord shift)
{
    return mDevice.BankSelectWriteRegister (NTV2RegInfo (chanOffset, chanNumber), NTV2RegInfo (reg, value, mask, shift));
}

void CNTV2MailBox::SetChannel(ULWord channelOffset, ULWord channelNumber)
{
    chanOffset  = channelOffset;
    chanNumber  = channelNumber;
}

void CNTV2MailBox::getError(std::string & error)
{
    if (mIpErrorCode == NTV2IpErrMBStatusFail)
    {
        error = mIpInternalErrorString;
    }
    else
    {
        error = NTV2IpErrorEnumToString(mIpErrorCode);
    }
    mIpErrorCode = NTV2IpErrNone;
}

bool CNTV2MailBox::AcquireMailbox()
{
    // If no MB do nothing, this is for Cochrane
    if (!(getFeatures() & SAREK_MB_PRESENT))
        return true;

    int waitCount = 20;
    while (waitCount--)
    {
        if (mDevice.AcquireMailBoxLock())
        {
            return true;
        }
        mDevice.WaitForOutputVerticalInterrupt();
    }

    // timeout
    mIpErrorCode = NTV2IpErrAcquireMBTimeout;
    return false;
}

void CNTV2MailBox::ReleaseMailbox()
{
    // If no MB do nothing, this is for Cochrane
    uint32_t features = getFeatures();
    if (features & SAREK_MB_PRESENT)
    {
        mDevice.ReleaseMailBoxLock();
    }
}

void CNTV2MailBox::getResponse(std::string & response)
{
    response.assign((char*)rxBuf);
}

void   CNTV2MailBox::startTimer()
{
    _startTime = getSystemMilliseconds();
}

uint64_t CNTV2MailBox::getElapsedTime()
{
    return (getSystemMilliseconds() - _startTime);
}

int64_t CNTV2MailBox::getSystemCounter()
{
#if defined(MSWindows)
    LARGE_INTEGER performanceCounter;

    performanceCounter.QuadPart = 0;
    if (!QueryPerformanceCounter(&performanceCounter))
    {
        return 0;
    }

    return (int64_t)performanceCounter.QuadPart;
#endif

#if defined(AJA_MAC)
    return (int64_t) mach_absolute_time();
#endif

#if defined(AJALinux) || defined(AJA_LINUX)
#ifdef AJA_USE_CLOCK_GETTIME
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * ((int64_t)1000000)) + (ts.tv_nsec / (int64_t)1000);
#else
    struct timeval tv;
    struct timezone tz;

    gettimeofday( &tv, &tz );
    return (int64_t)((int64_t)tv.tv_sec * (int64_t)1000000 + tv.tv_usec);
#endif
#endif
}


int64_t CNTV2MailBox::getSystemFrequency()
{
#if defined(MSWindows)
    if (!s_bPerformanceInit)
    {
        QueryPerformanceFrequency(&s_PerformanceFrequency);
        s_bPerformanceInit = true;
    }

    return (int64_t)s_PerformanceFrequency.QuadPart;
#endif

#if defined(AJA_MAC)
    if (!s_bPerformanceInit)
    {
        // 1 billion ticks approximately equals 1 sec on a Mac
        static mach_timebase_info_data_t    sTimebaseInfo;
        uint64_t ticks = 1000000000;

        if ( sTimebaseInfo.denom == 0 )
        {
            (void) mach_timebase_info(&sTimebaseInfo);
        }

        // Do the maths. We hope that the multiplication doesn't
        // overflow; the price you pay for working in fixed point.
        int64_t nanoSeconds = ticks * sTimebaseInfo.numer / sTimebaseInfo.denom;

        // system frequency - ticks per second units
        s_PerformanceFrequency = ticks * 1000000000 / nanoSeconds;
        s_bPerformanceInit = true;
    }

    return s_PerformanceFrequency;
#endif

#if defined(AJALinux) || defined(AJA_LINUX)
    return 1000000;
#endif
}

uint64_t CNTV2MailBox::getSystemMilliseconds()
{
    uint64_t ticks          = getSystemCounter();
    uint64_t ticksPerSecond = getSystemFrequency();
    uint64_t ms             = 0;
    if (ticksPerSecond)
    {
        // floats are being used here to avoid the issue of overflow
        // or inaccuracy when chosing where to apply the '1000' correction
        ms = uint64_t((double(ticks) / double(ticksPerSecond)) * 1000.);
    }
    return ms;
}


uint32_t CNTV2MailBox::getFeatures()
{
    uint32_t val;
    mDevice.ReadRegister(SAREK_REGS + kRegSarekFwCfg, val);
    return val;
}


