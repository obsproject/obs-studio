/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2mailbox.h
	@brief		Declares the CNTV2MailBox class.
	@copyright	(C) 2014-2021 AJA Video Systems, Inc.    All rights reserved.
**/

#ifndef CNTV2MAILBOX_H
#define CNTV2MAILBOX_H

#include "ntv2card.h"
#include "ntv2enums.h"
#include "ntv2registers2022.h"
#include <stdint.h>

#define MB_tWRDATA      0                   // 0x00
#define MB_tRDDATA      2                   // 0x08
#define MB_tSTATUS      4                   // 0x10
#define MB_tERROR       5                   // 0x14
#define MB_tSIT         6                   // 0x18    // send interrupt threshold
#define MB_tRIT         6                   // 0x18    // receive interrupt threshold
#define MB_tIS          8                   // 0x20    // interrupt status register
#define MB_tIE          9                   // 0x24    // interrupt enable register
#define MB_tIP          10                  // 0x28    // interrupt pending register

#define MBS_RX_EMPTY    BIT(0)
#define MBS_TX_FULL     BIT(1)
#define MBS_SEND_LT_TA  BIT(2)    // send level <= SIT
#define MBS_RCV_GT_TA   BIT(3)    // receive level > RIT

#define MBE_RX_EMPTY    BIT(1)
#define MBE_TX_FULL     BIT(2)

#define MB_RX_INT       BIT(1)

#define FIFO_SIZE       1024    // 32-bit words
#define MB_TIMEOUT      50      // milliseconds

#define SEQNUM_MIN      1
#define SEQNUM_MAX      500

class AJAExport CNTV2MailBox
{
public:
    CNTV2MailBox(CNTV2Card & device);
    ~CNTV2MailBox();

    bool        sendMsg(char * msg, uint32_t timeout); // returns response
    bool        sendMsg(uint32_t timeout);

    void        getError(std::string & error);
    void        getResponse(std::string & response);

    bool        WriteChannelRegister (ULWord reg, ULWord value,  ULWord mask = 0xFFFFFFFF, ULWord shift = 0x0);

    bool        ReadChannelRegister  (const ULWord inReg, ULWord & outValue, const ULWord inMask = 0xFFFFFFFF, const ULWord inShift = 0x0);
    inline bool ReadChannelRegister  (ULWord inReg, ULWord *pOutValue, ULWord mask = 0xFFFFFFFF, ULWord shift = 0x0)	{return pOutValue ? ReadChannelRegister (inReg, *pOutValue, mask, shift) : false;}

    void        SetChannel(ULWord channelOffset, ULWord channelNumber);

    bool        AcquireMailbox();
    void        ReleaseMailbox();

protected:
    bool        rcvMsg(uint32_t timeout);

    bool        writeMB(uint32_t val,  uint32_t timeout = MB_TIMEOUT);
    bool        readMB(uint32_t & val, uint32_t timeout = MB_TIMEOUT);

    bool        waitSOM(uint32_t timeout);
    bool        waitRxReady(uint32_t timeout);
    bool        waitTxReady(uint32_t timeout);

    bool        rxReady();

    uint32_t    getStatus();
    uint32_t    getFeatures();

    CNTV2Card   &mDevice;

    NTV2IpError mIpErrorCode;
    std::string mIpInternalErrorString;

private:
    void        startTimer();
    uint64_t    getElapsedTime();
    int64_t     getSystemCounter();
    int64_t     getSystemFrequency();
    uint64_t    getSystemMilliseconds();


    uint32_t    nextSeqNum() {if (++_seqNum > SEQNUM_MAX) return SEQNUM_MIN; else return _seqNum;}
    uint32_t    currentSeqNum() {return _seqNum;}

    uint32_t    chanOffset;
    uint32_t    chanNumber;

    uint32_t    bOffset;             // base offset
protected:
    uint32_t    txBuf[FIFO_SIZE+1];	//	CNTV2MBController needs access to this
private:
    uint32_t    rxBuf[FIFO_SIZE+1];

    uint64_t    _startTime;
    uint32_t    _seqNum;
};

#endif // CNTV2MAILBOX_H
