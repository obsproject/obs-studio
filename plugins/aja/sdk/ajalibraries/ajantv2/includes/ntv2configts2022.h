/* SPDX-License-Identifier: MIT */
/**
    @file		ntv2configts2022.h
    @brief		Declares the CNTV2ConfigTs2022 class.
	@copyright	(C) 2014-2021 AJA Video Systems, Inc.
**/

#ifndef NTV2_2022CONFIGTS_H
#define NTV2_2022CONFIGTS_H

#include "ntv2card.h"
#include "ntv2enums.h"
#include "ntv2registers2022.h"
#include "ntv2mbcontroller.h"
#include "ntv2tshelper.h"
#include "ntv2config2022.h"
#include <string.h>

// Encoder part numbers
#define     ENCODE_TS_TIMER                 0
#define     ENCODE_TS_J2K_ENCODER           1
#define     ENCODE_TS_MPEG_J2K_ENCAP        2
#define     ENCODE_TS_AES_ENCAP             3
#define     ENCODE_TS_MPEG_AES_ENCAP        4
#define     ENCODE_TS_MPEG_ANC_ENCAP        6
#define     ENCODE_TS_MPEG_PCR_ENCAP        7

// Decoder part numbers
#define     DECODE_TS_MPEG_J2K_DECAP        0
#define     DECODE_TS_J2K_DECODER           1
#define     DECODE_TS_MPEG_AES_DECAP        2
#define     DECODE_TS_AES_DECAP             3
#define     DECODE_TS_MPEG_ANC_DECAP        5


#define     PES_HDR_LOOKUP                  0x0
#define     PES_HDR_LEN                     0xc0
#define     PTS_OFFSET                      0xc1
#define     J2K_TS_OFFSET                   0xc2
#define     AUF1_OFFSET                     0xc3
#define     AUF2_OFFSET                     0xc4
#define     PACKET_RATE                     0xca
#define     HOST_EN                         0xe0
#define     INTERLACED_VIDEO                0xe1
#define     PAYLOAD_PARAMS                  0xe2
#define     LOOPBACK                        0xe3
#define     PAT_TABLE_LOOKUP                0x100
#define     PAT_PMT_PERIOD                  0x1f0
#define     PMT_TABLE_LOOKUP                0x200
#define     ADAPTATION_LOOKUP               0x300
#define     ADAPTATION_HDR_LENGTH           0x3f0

#define     MODE_STOP                       0x00
#define     MODE_RECORD                     0x10

/**
    @brief	The CNTV2ConfigTs2022 class is the interface to Kona-IP SMPTE 2022 J2K encoder and TS chips
**/

class AJAExport CNTV2ConfigTs2022 : public CNTV2MBController
{
public:
    CNTV2ConfigTs2022 (CNTV2Card & device);

    // Setup the J2K encoder
    bool    SetupJ2KEncoder(const NTV2Channel channel, const j2kEncoderConfig &config);
    bool    ReadbackJ2KEncoder(const NTV2Channel channel, j2kEncoderConfig &config);

    // Setup the J2K decoder
    bool    SetupJ2KDecoder(const  j2kDecoderConfig & config);
    bool    ReadbackJ2KDecoder(j2kDecoderConfig &config);
    bool    GetJ2KDecoderStatus(j2kDecoderStatus &status);

    // If method returns false call this to get the error code
    NTV2IpError getLastErrorCode();

 private:

    // Setup the J2K encoder and TS
    bool    SetupJ2KForEncode(const NTV2Channel channel);
    bool    SetupTsForEncode(const NTV2Channel channel);

    // Setup individual TS encode parts
    bool    SetupEncodeTsTimer(const NTV2Channel channel);
    bool    SetupEncodeTsJ2KEncoder(const NTV2Channel channel);
    bool    SetupEncodeTsMpegJ2kEncap(const NTV2Channel channel);
    bool    SetupEncodeTsMpegPcrEncap(const NTV2Channel channel);
    bool    SetupEncodeTsMpegAesEncap(const NTV2Channel channel);
    bool    SetupEncodeTsAesEncap(const NTV2Channel channel);

    // Routines to talk to the J2K part
    bool                J2kCanAcceptCmd(const NTV2Channel channel);
    bool                J2KGetNextT0Status(const NTV2Channel channel, uint32_t *pStatus);
    bool                GetT0CmdStatus( const NTV2Channel channel, const uint32_t cmdId, uint32_t *pStatus );
    void                J2kSetParam(const NTV2Channel channel, uint32_t config, uint32_t param, uint32_t value);
    void                J2kSetMode(const NTV2Channel channel, uint32_t tier, uint32_t mode);
    uint32_t            J2kGetFrameCounter(const NTV2Channel channel, uint32_t tier);
    void                J2kSetConfig(const NTV2Channel channel, uint32_t config);

    uint32_t            GetFeatures();

    void                GenerateTableForMpegJ2kEncap(const NTV2Channel channel);
    void                GenerateTableForMpegPcrEncap(const NTV2Channel channel);
    void                GenerateTableForMpegAesEncap(const NTV2Channel channel);
    uint32_t            GetIpxJ2KAddr(const NTV2Channel channel);
    uint32_t            GetIpxTsAddr(const NTV2Channel channel);

    bool                WriteJ2KConfigReg(const NTV2Channel channel, const uint32_t reg, const uint32_t value);
    bool                ReadJ2KConfigReg(const NTV2Channel channel, const uint32_t reg,  uint32_t * value);

    void                SetEncoderInputEnable(const NTV2Channel channel, bool bEnable, bool bMDEnable );
    void                SetEncoderReset(const NTV2Channel channel, bool bReset );
    int32_t             CalculateTsGen(const NTV2Channel channel);

    bool                _is2022_6;
    bool                _is2022_2;

    int32_t             _transactionTable[1024][2];
    int32_t             _transactionCount;

};	//	CNTV2ConfigTs2022

#endif // NTV2_2022CONFIGTS_H
