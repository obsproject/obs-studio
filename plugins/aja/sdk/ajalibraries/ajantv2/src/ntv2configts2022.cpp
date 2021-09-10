/* SPDX-License-Identifier: MIT */
/**
    @file		ntv2config2022.cpp
    @brief		Implements the CNTV2ConfigTs2022 class.
	@copyright	(C) 2014-2021 AJA Video Systems, Inc.
**/

#include "ntv2configts2022.h"
#include "ntv2endian.h"
#include "ntv2card.h"
#include "ntv2utils.h"
#include "ntv2formatdescriptor.h"

#include <sstream>

#if defined (AJALinux) || defined (AJAMac)
	#include <stdlib.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#else
#pragma warning(disable:4800)
#endif

using namespace std;

CNTV2ConfigTs2022::CNTV2ConfigTs2022(CNTV2Card & device) : CNTV2MBController(device)
{
    uint32_t features    = GetFeatures();

    _is2022_6   = (bool)(features & SAREK_2022_6);
    _is2022_2   = (bool)(features & SAREK_2022_2);
}

bool CNTV2ConfigTs2022::SetupJ2KEncoder(const NTV2Channel channel, const j2kEncoderConfig &config)
{
#define WAIT_RESET_MS   800

    kipdprintf("CNTV2ConfigTs2022::SetupJ2KEncoder channel = %d\n", channel);

    // Check for a proper channnel (we only configure NTV2_CHANNEL1 and NTV2_CHANNEL2)
    if ((channel != NTV2_CHANNEL2) && (channel != NTV2_CHANNEL1))
    {
        mIpErrorCode = NTV2IpErrInvalidChannel;
        return false;
    }

    // Before we do anything lets make sure video format and bitdepth are in valid ranges
    if (!IsVideoFormatJ2KSupported(config.videoFormat))
    {
        mIpErrorCode = NTV2IpErrInvalidFormat;
        return false;
    }
    
    if ((config.bitDepth != 8) && (config.bitDepth != 10))
    {
        mIpErrorCode = NTV2IpErrInvalidBitdepth;
        return false;
    }

    // Set T2 to mode stop
    J2kSetMode(channel, 2, MODE_STOP);

    // Now wait until T2 has stopped
    uint32_t lastFrameCount = J2kGetFrameCounter(channel, 2);
    uint32_t currentFrameCount = 0;
    uint32_t tries = 20;

    while (tries)
    {
        // Wait
        #if defined(AJAWindows) || defined(MSWindows)
            ::Sleep (50);
        #else
            usleep (50 * 1000);
        #endif

        currentFrameCount = J2kGetFrameCounter(channel, 2);
        // See if t2 is still running
        if (lastFrameCount != currentFrameCount)
        {
            // Yep wait some more
            lastFrameCount = currentFrameCount;
            tries--;
        }
        else
        {
            // Nope end the wait
            tries = 0;
        }
    }

    // Disable encoder inputs
    SetEncoderInputEnable( channel, false, false );

    // Assert reset
	SetEncoderReset( channel, true );

    // Wait
    #if defined(AJAWindows) || defined(MSWindows)
        ::Sleep (WAIT_RESET_MS/2);
    #else
        usleep (WAIT_RESET_MS/2 * 1000);
    #endif

    // De-assert reset
	SetEncoderReset( channel, false );

    // Wait
    #if defined(AJAWindows) || defined(MSWindows)
        ::Sleep (WAIT_RESET_MS);
    #else
        usleep (WAIT_RESET_MS * 1000);
    #endif

    // Now proceed to configure the device     
    WriteJ2KConfigReg(channel, kRegSarekEncodeVideoFormat1, (uint32_t) config.videoFormat);
    WriteJ2KConfigReg(channel, kRegSarekEncodeUllMode1, config.ullMode);
    WriteJ2KConfigReg(channel, kRegSarekEncodeBitDepth1, config.bitDepth);
    WriteJ2KConfigReg(channel, kRegSarekEncodeChromaSubSamp1, (uint32_t) config.chromaSubsamp);
    WriteJ2KConfigReg(channel, kRegSarekEncodeMbps1, config.mbps);
    WriteJ2KConfigReg(channel, kRegSarekEncodeStreamType1, (uint32_t) config.streamType);
    WriteJ2KConfigReg(channel, kRegSarekEncodeAudioChannels1, (uint32_t) config.audioChannels);
    WriteJ2KConfigReg(channel, kRegSarekEncodeProgramPid1, config.pmtPid);
    WriteJ2KConfigReg(channel, kRegSarekEncodeVideoPid1, config.videoPid);
    WriteJ2KConfigReg(channel, kRegSarekEncodePcrPid1, config.pcrPid);
    WriteJ2KConfigReg(channel, kRegSarekEncodeAudio1Pid1, config.audio1Pid);

    // setup the TS
    if (!SetupTsForEncode(channel))
        return false;

    // setup the J2K encoder
    if (!SetupJ2KForEncode(channel))
        return false;

    // Need to set 20 or 24 bit audio in NTV2 audio control reg.  For now we are doing this on
    // a global basis and setting all audio engines to the same value.
    J2KStreamType       streamType;
    ReadJ2KConfigReg(channel, kRegSarekEncodeStreamType1, (uint32_t *) &streamType);
    bool    do20Bit = true;

    // Set PTS_MUX depending on streamtype
    if (streamType == kJ2KStreamTypeNonElsm)
    {
        do20Bit = false;
    }

    for (uint32_t i = NTV2_AUDIOSYSTEM_1; i<NTV2_MAX_NUM_AudioSystemEnums; i++)
    {
        mDevice.SetAudio20BitMode ((NTV2AudioSystem)i, do20Bit);
    }

    // Set T2 to mode record
    J2kSetMode(channel, 2, MODE_RECORD);
	// We don't need to do this because the firwmare does it
    // J2kSetMode(channel, 0, MODE_RECORD);

    // Turn on input to the encoder
	SetEncoderInputEnable( channel, true, (config.streamType == kJ2KStreamTypeNonElsm));

    return true;
}

bool CNTV2ConfigTs2022::ReadbackJ2KEncoder(const NTV2Channel channel, j2kEncoderConfig &config)
{
    if ((channel == NTV2_CHANNEL1) || (channel == NTV2_CHANNEL2))
    {
        ReadJ2KConfigReg(channel, kRegSarekEncodeVideoFormat1, (uint32_t*) &config.videoFormat);
        ReadJ2KConfigReg(channel, kRegSarekEncodeUllMode1, &config.ullMode);
        ReadJ2KConfigReg(channel, kRegSarekEncodeBitDepth1, &config.bitDepth);
        ReadJ2KConfigReg(channel, kRegSarekEncodeChromaSubSamp1, (uint32_t *) &config.chromaSubsamp);
        ReadJ2KConfigReg(channel, kRegSarekEncodeMbps1, &config.mbps);
        ReadJ2KConfigReg(channel, kRegSarekEncodeStreamType1, (uint32_t *) &config.streamType);
        ReadJ2KConfigReg(channel, kRegSarekEncodeAudioChannels1, (uint32_t *) &config.audioChannels);
        ReadJ2KConfigReg(channel, kRegSarekEncodeProgramPid1, &config.pmtPid);
        ReadJ2KConfigReg(channel, kRegSarekEncodeVideoPid1, &config.videoPid);
        ReadJ2KConfigReg(channel, kRegSarekEncodePcrPid1, &config.pcrPid);
        ReadJ2KConfigReg(channel, kRegSarekEncodeAudio1Pid1, &config.audio1Pid);
        return true;
    }
    else
    {
        mIpErrorCode = NTV2IpErrInvalidChannel;
        return false;
    }
}

bool CNTV2ConfigTs2022::SetupJ2KForEncode(const NTV2Channel channel)
{
    NTV2VideoFormat         videoFormat;
    uint32_t                ullMode;
    uint32_t                bitDepth;
    J2KChromaSubSampling    subSamp;
    uint32_t                mbps;

    uint32_t                bitRateMsb;
    uint32_t                bitRateLsb;
    uint32_t                rtConstant;
    uint32_t                ull;

    // Subband removal Component 0, 1 and 2
    uint32_t                sb_rmv_c0 = 0;
    uint32_t                sb_rmv_c1 = 0;
    uint32_t                sb_rmv_c2 = 0;
    J2KCodeBlocksize        codeBlocksize = kJ2KCodeBlocksize_32x32;

    uint32_t                regulator_type = 0;
    uint32_t                Rsiz = 258;
    uint32_t                reg_cap = 0;
    uint32_t                clk_fx_freq = 200;
    uint32_t                marker = 0x02;
    uint32_t                guard_bit = 1;
    uint32_t                prog_order = 0;
    uint32_t                fdwt_type = 1;
    uint32_t                mct = 0;
    uint32_t                num_comp = 3;
    uint32_t                num_levels = 5;

    uint32_t                GOB[8] =        {03, 03, 03, 03, 03, 03, 03, 03};
    uint32_t                QS_C0[8] =      {12, 13, 14, 15, 16, 16, 16, 16};
    uint32_t                QS_C1[8] =      {12, 13, 14, 15, 16, 16, 16, 16};
    uint32_t                QS_C2[8] =      {12, 13, 14, 15, 16, 16, 16, 16};

    // Get our variable user params
    ReadJ2KConfigReg(channel, kRegSarekEncodeVideoFormat1, (uint32_t*) &videoFormat);
    ReadJ2KConfigReg(channel, kRegSarekEncodeUllMode1, &ullMode);
    ReadJ2KConfigReg(channel, kRegSarekEncodeBitDepth1, &bitDepth);
    ReadJ2KConfigReg(channel, kRegSarekEncodeChromaSubSamp1, (uint32_t *) &subSamp);
    ReadJ2KConfigReg(channel, kRegSarekEncodeMbps1, &mbps);

    // Calculate height and width based on video format
    NTV2Standard standard = GetNTV2StandardFromVideoFormat(videoFormat);
    NTV2FormatDescriptor fd (standard, NTV2_FBF_10BIT_YCBCR);
    uint32_t    width = fd.GetRasterWidth();
    uint32_t    height = fd.GetVisibleRasterHeight();

    // Calculate framerate based on video format
    NTV2FrameRate   frameRate = GetNTV2FrameRateFromVideoFormat(videoFormat);
    uint32_t    framesPerSecNum;
    uint32_t    framesPerSecDen;
    uint32_t    fieldsPerSec;

    GetFramesPerSecond (frameRate, framesPerSecNum, framesPerSecDen);
    if (framesPerSecDen == 1001)
        framesPerSecDen = 1000;
    fieldsPerSec = framesPerSecNum/framesPerSecDen;
    if (! NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE(videoFormat))
        fieldsPerSec*=2;

    kipdprintf("CNTV2ConfigTs2022::SetupJ2KForEncode width=%d, height=%d fpsNum=%d,\n", width, height, fieldsPerSec);

    if (ullMode)
    {
        ull = 1;
        bitRateMsb = ((((mbps*1000000)/fieldsPerSec)/8)/9) >> 16;
        bitRateLsb = ((((mbps*1000000)/fieldsPerSec)/8)/9) & 0xFFFF;
        rtConstant = (((clk_fx_freq*149000)/200)/fieldsPerSec)/9;
    }
    else
    {
        ull = 0;
        bitRateMsb = (((mbps*1000000)/fieldsPerSec)/8) >> 16;
        bitRateLsb = (((mbps*1000000)/fieldsPerSec)/8) & 0xFFFF;
        rtConstant = (((clk_fx_freq*149000)/200)/fieldsPerSec);
    }


    for (uint32_t config=0; config < 3; config++)
    {
        J2kSetParam(channel, config, 0x00, width);
        J2kSetParam(channel, config, 0x01, height);
        J2kSetParam(channel, config, 0x02, bitDepth);
        J2kSetParam(channel, config, 0x03, guard_bit);
        J2kSetParam(channel, config, 0x04, subSamp);
        J2kSetParam(channel, config, 0x05, num_comp);
        J2kSetParam(channel, config, 0x06, mct);
        J2kSetParam(channel, config, 0x07, fdwt_type);
        J2kSetParam(channel, config, 0x08, num_levels);
        J2kSetParam(channel, config, 0x09, codeBlocksize);

        J2kSetParam(channel, config, 0x0A, prog_order);
        J2kSetParam(channel, config, 0x0B, bitRateMsb);
        J2kSetParam(channel, config, 0x0C, bitRateLsb);
        J2kSetParam(channel, config, 0x0D, rtConstant);
        J2kSetParam(channel, config, 0x0E, marker);
        J2kSetParam(channel, config, 0x0F, ull);
        J2kSetParam(channel, config, 0x10, sb_rmv_c0);
        J2kSetParam(channel, config, 0x11, sb_rmv_c1);
        J2kSetParam(channel, config, 0x12, sb_rmv_c2);
        J2kSetParam(channel, config, 0x13, regulator_type);
        J2kSetParam(channel, config, 0x14, Rsiz);
        J2kSetParam(channel, config, 0x15, reg_cap);
    }

    if (ullMode)
    {
        // Sanity checks
        if ((height != 1080) && (height != 540))
        {
            mIpErrorCode = NTV2IpErrInvalidUllHeight;
            return false;
        }

        if ((height == 1080) && (num_levels > 4))
        {
            mIpErrorCode = NTV2IpErrInvalidUllLevels;
            return false;
        }

        if ((height == 540) && (num_levels > 3))
        {
            mIpErrorCode = NTV2IpErrInvalidUllLevels;
            return false;
        }

        mIpErrorCode = NTV2IpErrUllNotSupported;
        return false;
    }

#if 0
    if {$ull} {

        #Set ULL in active mode
        opb_write $env [expr 0x10000*$enc+0x1006] 1

        #Set img height for each kind of tile
        if {$im_height == 1080} {
            ipx_jp2k_e_set_param $env $enc 0 0x1 128
            ipx_jp2k_e_set_param $env $enc 1 0x1 128
            ipx_jp2k_e_set_param $env $enc 2 0x1 56
        }
        if {$im_height == 540} {
            ipx_jp2k_e_set_param $env $enc 0 0x1 64
            ipx_jp2k_e_set_param $env $enc 1 0x1 64
            ipx_jp2k_e_set_param $env $enc 2 0x1 28
        }

        #Specify all kind of tile
        ipx_jp2k_e_set_param $env $enc 0 0xF 1
        ipx_jp2k_e_set_param $env $enc 1 0xF 2
        ipx_jp2k_e_set_param $env $enc 2 0xF 3
    }

    #ipx_jp2k_e_get_all_status $env $enc 0
    for {set config 0} {$config<3} {incr config} {
        for {set lvl 0} {$lvl<7} {incr lvl} {
            ipx_jp2k_e_set_param $env $enc $config [expr 0x80+$lvl] [lindex $GOB 	$lvl]
            ipx_jp2k_e_set_param $env $enc $config [expr 0x88+$lvl] [expr [lindex $QS_C0 $lvl]*0x800]
            ipx_jp2k_e_set_param $env $enc $config [expr 0x90+$lvl] [expr [lindex $QS_C1 $lvl]*0x800]
            ipx_jp2k_e_set_param $env $enc $config [expr 0x98+$lvl] [expr [lindex $QS_C2 $lvl]*0x800]
        }
    }

#endif


    for (uint32_t config=0; config < 3; config++)
    {
        for (uint32_t lvl=0; lvl < 7; lvl++)
        {
            J2kSetParam(channel, config, 0x80+lvl, GOB[lvl]);
            J2kSetParam(channel, config, 0x88+lvl, QS_C0[lvl]*0x800);
            J2kSetParam(channel, config, 0x90+lvl, QS_C1[lvl]*0x800);
            J2kSetParam(channel, config, 0x98+lvl, QS_C2[lvl]*0x800);
        }
    }

	//J2kSetConfig( channel, 0 /*select config 0*/ );

    return true;
}


bool CNTV2ConfigTs2022::SetupJ2KDecoder(const j2kDecoderConfig &config)
{
    mDevice.WriteRegister(SAREK_IPX_J2K_DECODER_1 + kRegJ2kPrpMainCsr, 0x10);   // prp mode play
    mDevice.WriteRegister(SAREK_IPX_J2K_DECODER_1 + kRegJ2kPopMainCsr, 0x12);   // pop mode play once

    mDevice.WriteRegister(SAREK_REGS2 + kRegSarekModeSelect,     uint32_t(config.selectionMode));
    mDevice.WriteRegister(SAREK_REGS2 + kRegSarekProgNumSelect,  config.programNumber);
    mDevice.WriteRegister(SAREK_REGS2 + kRegSarekProgPIDSelect,  config.programPID);
    mDevice.WriteRegister(SAREK_REGS2 + kRegSarekAudioNumSelect, config.audioNumber);

    uint32_t seqNum;
    mDevice.ReadRegister( SAREK_REGS2 + kRegSarekHostSeqNum, seqNum);
    mDevice.WriteRegister(SAREK_REGS2 + kRegSarekHostSeqNum, ++seqNum);

    return true;
}

bool CNTV2ConfigTs2022::ReadbackJ2KDecoder(j2kDecoderConfig &config)
{
    mDevice.ReadRegister(SAREK_REGS2 + kRegSarekModeSelect,     (uint32_t&)config.selectionMode);
    mDevice.ReadRegister(SAREK_REGS2 + kRegSarekProgNumSelect,  config.programNumber);
    mDevice.ReadRegister(SAREK_REGS2 + kRegSarekProgPIDSelect,  config.programPID);
    mDevice.ReadRegister(SAREK_REGS2 + kRegSarekAudioNumSelect, config.audioNumber);

    return true;
}

bool CNTV2ConfigTs2022::GetJ2KDecoderStatus(j2kDecoderStatus &status)
{
    status.init();

    mDevice.ReadRegister(SAREK_REGS2 + kRegSarekNumPGMs,   status.numAvailablePrograms);
    mDevice.ReadRegister(SAREK_REGS2 + kRegSarekNumAudios, status.numAvailableAudios);
    for (uint32_t i=0; i < status.numAvailablePrograms; i++)
    {
        uint32_t val;
        mDevice.ReadRegister(SAREK_REGS2 + kRegSarekPGMNums + i, val);
        status.availableProgramNumbers.push_back(val);
        mDevice.ReadRegister(SAREK_REGS2 + kRegSarekPGMPIDs + i, val);
        status.availableProgramPIDs.push_back(val);
    }
    for (uint32_t i=0; i < status.numAvailableAudios; i++)
    {
        uint32_t val;
        mDevice.ReadRegister(SAREK_REGS2 + kRegSarekAudioPIDs + i, val);
        status.availableAudioPIDs.push_back(val);
    }

    return true;
}

bool CNTV2ConfigTs2022::SetupTsForEncode(const NTV2Channel channel)
{
    // program TS timer
    if (!SetupEncodeTsTimer(channel))
        return false;

    // program TS for mpeg j2k encapsulator
    if (!SetupEncodeTsMpegJ2kEncap(channel))
        return false;

    // program TS for mpeg pcr encapsulator
    if (!SetupEncodeTsMpegPcrEncap(channel))
        return false;

    // program TS for mpeg aes encapsulator
    if (!SetupEncodeTsMpegAesEncap(channel))
        return false;

    // program TS for aes encapsulator
    if (!SetupEncodeTsAesEncap(channel))
        return false;

    // finally setup TS for the encoder
    if (!SetupEncodeTsJ2KEncoder(channel))
        return false;

    return true;
}


// Private functions

// Setup individual TS encode parts
bool CNTV2ConfigTs2022::SetupEncodeTsTimer(const NTV2Channel channel)
{
    uint32_t addr = GetIpxTsAddr(channel);
    int32_t tsGen = 0;

    kipdprintf("CNTV2ConfigTs2022::SetupEncodeTsTimer\n");

    mDevice.WriteRegister(addr + (0x800*ENCODE_TS_TIMER) + kRegTsTimerJ2kTsLoad, (0x103110));

    // Calculate TS Gen based on total bitrate and system clock
    tsGen = CalculateTsGen(channel);
    mDevice.WriteRegister(addr + (0x800*ENCODE_TS_TIMER) + kRegTsTimerJ2kTsGenTc, tsGen);

    J2KStreamType       streamType;
    ReadJ2KConfigReg(channel, kRegSarekEncodeStreamType1, (uint32_t *) &streamType);

    // Set PTS_MUX depending on streamtype
    if (streamType == kJ2KStreamTypeNonElsm)
    {
        mDevice.WriteRegister(addr + (0x800*ENCODE_TS_TIMER) + kRegTsTimerJ2kTsPtsMux, (0x2));
    }
    else
    {
        mDevice.WriteRegister(addr + (0x800*ENCODE_TS_TIMER) + kRegTsTimerJ2kTsPtsMux, (0x1));
    }

    return true;
}


bool CNTV2ConfigTs2022::SetupEncodeTsJ2KEncoder(const NTV2Channel channel)
{
    NTV2VideoFormat videoFormat;
    uint32_t addr = GetIpxTsAddr(channel);

    kipdprintf("CNTV2ConfigTs2022::SetupEncodeTsJ2KEncoder\n");

    ReadJ2KConfigReg(channel, kRegSarekEncodeVideoFormat1, (uint32_t*) &videoFormat);
    if (NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE(videoFormat))
    {
        // progressive format
        mDevice.WriteRegister(addr + (0x800*ENCODE_TS_J2K_ENCODER) + kRegTsJ2kEncoderInterlacedVideo, (0x0));
    }
    else
    {
        // interlaced format
        mDevice.WriteRegister(addr + (0x800*ENCODE_TS_J2K_ENCODER) + kRegTsJ2kEncoderInterlacedVideo, (0x1));
    }

    mDevice.WriteRegister(addr + (0x800*ENCODE_TS_J2K_ENCODER) + kRegTsJ2kEncoderFlushTimeout, (0x0));
    mDevice.WriteRegister(addr + (0x800*ENCODE_TS_J2K_ENCODER) + kRegTsJ2kEncoderHostEn, (0x6));

    // Wait 60 ms
    #if defined(AJAWindows) || defined(MSWindows)
        ::Sleep (60);
    #else
        usleep (60 * 1000);
    #endif

    mDevice.WriteRegister(addr + (0x800*ENCODE_TS_J2K_ENCODER) + kRegTsJ2kEncoderHostEn, (0x1));

    return true;
}


bool CNTV2ConfigTs2022::SetupEncodeTsMpegJ2kEncap(const NTV2Channel channel)
{
    uint32_t addr = GetIpxTsAddr(channel);

    kipdprintf("CNTV2ConfigTs2022::SetupEncodeTsMpegJ2kEncap\n");

    GenerateTableForMpegJ2kEncap(channel);

    // Program Transaction Table
    for (int32_t index=0; index < _transactionCount; index++)
    {
        mDevice.WriteRegister(addr + (0x800*ENCODE_TS_MPEG_J2K_ENCAP) + (_transactionTable[index][0]), _transactionTable[index][1]);
        //printf("SetupEncodeTsMpegJ2kEncap - addr=%08x, val=%08x\n",
        //       addr + (0x800*ENCODE_TS_MPEG_J2K_ENCAP) + (_transactionTable[index][0]),  _transactionTable[index][1]);
    }
    return true;
}


bool CNTV2ConfigTs2022::SetupEncodeTsMpegPcrEncap(const NTV2Channel channel)
{
    uint32_t addr = GetIpxTsAddr(channel);

    kipdprintf("CNTV2ConfigTs2022::SetupEncodeTsMpegPcrEncap\n");

    GenerateTableForMpegPcrEncap(channel);

    // Program Transaction Table
    for (int32_t index=0; index < _transactionCount; index++)
    {
        mDevice.WriteRegister(addr + (0x800*ENCODE_TS_MPEG_PCR_ENCAP) + (_transactionTable[index][0]), _transactionTable[index][1]);
        //printf("SetupEncodeTsMpegPcrEncap - addr=%08x, val=%08x\n",
        //       addr + (0x800*ENCODE_TS_MPEG_PCR_ENCAP) + (_transactionTable[index][0]),  _transactionTable[index][1]);
    }
    return true;
}


bool CNTV2ConfigTs2022::SetupEncodeTsMpegAesEncap(const NTV2Channel channel)
{
    uint32_t addr = GetIpxTsAddr(channel);

    kipdprintf("CNTV2ConfigTs2022::SetupEncodeTsMpegAesEncap\n");

    GenerateTableForMpegAesEncap(channel);

    // Program Transaction Table
    for (int32_t index=0; index < _transactionCount; index++)
    {
        mDevice.WriteRegister(addr + (0x800*ENCODE_TS_MPEG_AES_ENCAP) + (_transactionTable[index][0]), _transactionTable[index][1]);
        //printf("SetupEncodeTsMpegAesEncap - addr=%08x, val=%08x\n",
        //       addr + (0x800*ENCODE_TS_MPEG_AES_ENCAP) + (_transactionTable[index][0]),  _transactionTable[index][1]);
    }
    return true;
}


bool CNTV2ConfigTs2022::SetupEncodeTsAesEncap(const NTV2Channel channel)
{
    uint32_t addr = GetIpxTsAddr(channel);

    J2KStreamType   streamType;
    uint32_t        numAudioChannels = 0;
    uint32_t        audioChannels = 0;

    ReadJ2KConfigReg(channel, kRegSarekEncodeStreamType1, (uint32_t *) &streamType);
    ReadJ2KConfigReg(channel, kRegSarekEncodeAudioChannels1, (uint32_t *) &numAudioChannels);

    // Need to figure out how many stereo pairs we have (0 is actually 1 stereo pair)
    if (numAudioChannels)
        audioChannels = (numAudioChannels/2) - 1;

    // Write number of audio channels and set bit 4 for non elsm streams to indicate 24 bit audio
    if (streamType == kJ2KStreamTypeNonElsm)
    {
        mDevice.WriteRegister(addr + (0x800*ENCODE_TS_AES_ENCAP) + kRegTsAesEncapNumChannels, audioChannels | 0x10);
    }
    else
    {
        mDevice.WriteRegister(addr + (0x800*ENCODE_TS_AES_ENCAP) + kRegTsAesEncapNumChannels, audioChannels);
    }

    // Enable the AES encapsulator if there is audio
    mDevice.WriteRegister(addr + (0x800*ENCODE_TS_AES_ENCAP) + kRegTsAesEncapHostEn, numAudioChannels?1:0);

    return true;
}


uint32_t CNTV2ConfigTs2022::GetFeatures()
{
    uint32_t val;
    mDevice.ReadRegister(SAREK_REGS + kRegSarekFwCfg, val);
    return val;
}


NTV2IpError CNTV2ConfigTs2022::getLastErrorCode()
{
    NTV2IpError error = mIpErrorCode;
    mIpErrorCode = NTV2IpErrNone;
    return error;
}


bool CNTV2ConfigTs2022::J2kCanAcceptCmd(const NTV2Channel channel)
{
    uint32_t val;
    uint32_t addr = GetIpxJ2KAddr(channel);

    // Read T0 Main CSR Register
    mDevice.ReadRegister(addr + kRegJ2kT0FIFOCsr, val);

    // Check CF bit note this is bit reversed from documentation
    // so we check 6 instead of 25
    if(val & BIT(6))
        return false;
    else
        return true;
}

bool CNTV2ConfigTs2022::J2KGetNextT0Status(const NTV2Channel channel, uint32_t *pStatus)
{
    uint32_t val;
    uint32_t addr = GetIpxJ2KAddr(channel);
	if (!pStatus)
		return false;

    // Read T0 Main CSR Register
    mDevice.ReadRegister(addr + kRegJ2kT0FIFOCsr, val);

	// Clear SF Status Full & SO Status Overflow bits
	if (val & (BIT(10) | BIT(9))) {
		printf("Overflow / status full 0x%08x\n", val);
		mDevice.WriteRegister( addr + kRegJ2kT0FIFOCsr, BIT(10) | BIT(9) );
	}

    // Check SE Status Empty bit
    if(val & BIT(11))
		return false;

	return mDevice.ReadRegister( addr + kRegJ2kT0StatusFIFO, *pStatus );
}


bool CNTV2ConfigTs2022::GetT0CmdStatus( const NTV2Channel channel, const uint32_t cmdId, uint32_t *pStatus ) {
	uint32_t val;
	static const int MAX_STATUSES_TO_WAIT = 16;
	int count = 0;
    while( J2KGetNextT0Status(channel, &val) )
    {
        if ( ((val >> 16)& 0xff) == cmdId )
        {
			*pStatus = val;
			return true;
        }
        else
        {
			count++;
			if (count == MAX_STATUSES_TO_WAIT)
				return false;
		}
	}
	return false;
}


void CNTV2ConfigTs2022::J2kSetMode(const NTV2Channel channel, uint32_t tier, uint32_t mode)
{
    uint32_t addr = GetIpxJ2KAddr(channel);

    mDevice.WriteRegister(addr + (tier*0x40) + kRegJ2kT0MainCsr, mode);
    //printf("J2kSetMode - %d wrote 0x%08x to MAIN CSR in tier %d\n", channel, mode, tier);
}


void CNTV2ConfigTs2022::J2kSetConfig(const NTV2Channel channel, uint32_t config)
{
    uint32_t val;
    uint32_t addr = GetIpxJ2KAddr(channel);

    mDevice.WriteRegister(addr + kRegJ2kT0CmdFIFO, 0x73010000 | (config & 0xffff));

    if (!GetT0CmdStatus( channel, 0x01, &val))
    {
		printf("No status received for setconfig\n");
		return;
	}

    if (val >> 24 != 0xf3)
    {
		printf("J2KSetConfig: Expected status 0xf3...... received 0x%08x\n", val);
	}
}


uint32_t CNTV2ConfigTs2022::J2kGetFrameCounter(const NTV2Channel channel, uint32_t tier)
{
    uint32_t addr = GetIpxJ2KAddr(channel);
    uint32_t val = 0;

    mDevice.ReadRegister(addr + (tier*0x40) + kRegJ2kT0Framecount, val);
    //printf("J2kGetFrameCounter - %d read 0x%08x to MAIN CSR in tier %d\n", channel, val, tier);
    return val;
}


void CNTV2ConfigTs2022::J2kSetParam (const NTV2Channel channel, uint32_t config, uint32_t param, uint32_t value)
{
    uint32_t val;
    uint32_t addr = GetIpxJ2KAddr(channel);

    //printf("J2kSetParam - ch=%d config=0x%08x param=0x%08x value=0x%08x\n", channel, config, param, value);

    while(!J2kCanAcceptCmd(channel))
    {
         printf("J2kSetParam - command fifo full\n");
    }

	// we use param as cmd id
    val = 0x70000000 + (param<<16) + (config&0x7)*0x2000 + param;
    mDevice.WriteRegister(addr + kRegJ2kT0CmdFIFO, val);
    //printf("J2kSetParam - wrote 0x%08x to CMD FIFO\n", val);

    while(!J2kCanAcceptCmd(channel))
    {
         printf("J2kSetParam - command fifo full\n");
    }

    val = 0x7f000000 + (param<<16) + value;
    mDevice.WriteRegister(addr + kRegJ2kT0CmdFIFO, val);
    //printf("J2kSetParam - wrote 0x%08x to CMD FIFO\n", val);

    if (!GetT0CmdStatus( channel, param /* doubles as cmd id*/, &val))
    {
		printf("No status received for SetParam\n");
		return;
	}

    if (val >> 24 != 0xf0)
    {
		printf("J2KSetConfig: Expected status 0xf0...... received 0x%08x\n", val);
	}
}


void CNTV2ConfigTs2022::GenerateTableForMpegJ2kEncap(const NTV2Channel channel)
{
    NTV2VideoFormat     videoFormat;
    int32_t             w1;
    TsEncapStreamData   streamData;

    kipdprintf("CNTV2ConfigTs2022::GenerateTransactionTableForMpegJ2kEncap\n");

    // Get our variable user params
    ReadJ2KConfigReg(channel, kRegSarekEncodeVideoFormat1, (uint32_t*) &videoFormat);
    ReadJ2KConfigReg(channel, kRegSarekEncodeStreamType1, (uint32_t *) &streamData.j2kStreamType);
    ReadJ2KConfigReg(channel, kRegSarekEncodeAudioChannels1, (uint32_t *) &streamData.numAudioChannels);

    streamData.interlaced = !NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE(videoFormat);

    // Calculate height and width based on video format
    NTV2Standard standard = GetNTV2StandardFromVideoFormat(videoFormat);
    NTV2FormatDescriptor fd (standard, NTV2_FBF_10BIT_YCBCR);
    streamData.width = fd.GetRasterWidth();
    streamData.height = fd.GetVisibleRasterHeight();

    // Calculate framerate based on video format
    NTV2FrameRate   frameRate = GetNTV2FrameRateFromVideoFormat(videoFormat);
    GetFramesPerSecond (frameRate, streamData.numFrameRate, streamData.denFrameRate);

    // set the PIDs for all streams
    ReadJ2KConfigReg(channel, kRegSarekEncodeProgramPid1, &streamData.programPid);
    ReadJ2KConfigReg(channel, kRegSarekEncodeVideoPid1, &streamData.videoPid);
    ReadJ2KConfigReg(channel, kRegSarekEncodePcrPid1, &streamData.pcrPid);
    ReadJ2KConfigReg(channel, kRegSarekEncodeAudio1Pid1, &streamData.audio1Pid);

    kipdprintf("Program PID     = 0x%02x\n", streamData.programPid);
    kipdprintf("Video PID       = 0x%02x\n", streamData.videoPid);
    kipdprintf("PCR PID         = 0x%02x\n", streamData.pcrPid);
    kipdprintf("Audio 1 PID     = 0x%02x\n\n", streamData.audio1Pid);

    _transactionCount = 0;

    // Start with enable off
    _transactionTable[_transactionCount][0] = HOST_EN;
    _transactionTable[_transactionCount++][1] = 0;

    PESGen pes;
    pes._tsEncapType = kTsEncapTypeJ2k;
    pes._elemNumToPID[1] = streamData.videoPid;
    pes._videoStreamData.j2kStreamType = streamData.j2kStreamType;
    pes._videoStreamData.width = streamData.width;
    pes._videoStreamData.height = streamData.height;
    pes._videoStreamData.denFrameRate = streamData.denFrameRate;
    pes._videoStreamData.numFrameRate = streamData.numFrameRate;
    pes._videoStreamData.numAudioChannels = streamData.numAudioChannels;
    pes._videoStreamData.interlaced = streamData.interlaced;

    kipdprintf("Host Register Settings:\n\n");
    _transactionTable[_transactionCount][0] = PAYLOAD_PARAMS;
    _transactionTable[_transactionCount++][1] = streamData.videoPid;
    kipdprintf("Payload Parameters = 0x%x\n", _transactionTable[_transactionCount-1][1]);

    _transactionTable[_transactionCount][0] = INTERLACED_VIDEO;
    _transactionTable[_transactionCount++][1] = streamData.interlaced;
    kipdprintf("Interlaced Video = %i\n", _transactionTable[_transactionCount-1][1]);

    _transactionTable[_transactionCount][0] = PAT_PMT_PERIOD;
    _transactionTable[_transactionCount++][1] = pes.calcPatPmtPeriod() | 0x01000000;
    kipdprintf("PAT/PMT Transmission Period = %i (0x%x)\n", _transactionTable[_transactionCount-1][1], _transactionTable[_transactionCount-1][1]);

    _transactionTable[_transactionCount][0] = PACKET_RATE;
    _transactionTable[_transactionCount++][1] = 0;
    kipdprintf("Packet Rate = %i (0x%x)\n", _transactionTable[_transactionCount-1][1], _transactionTable[_transactionCount-1][1]);

    int length = pes.makePacket();

    kipdprintf("PTS Offset = 0x%02x J2K TS Offset = 0x%02x auf1 offset = 0x%02x auf2 offset = 0x%02x\n\n",
               pes._ptsOffset, pes._j2kTsOffset, pes._auf1Offset, pes._auf2Offset);
    _transactionTable[_transactionCount][0] = PTS_OFFSET;
    _transactionTable[_transactionCount++][1] = pes._ptsOffset;
    _transactionTable[_transactionCount][0] = J2K_TS_OFFSET;
    _transactionTable[_transactionCount++][1] = pes._j2kTsOffset;
    _transactionTable[_transactionCount][0] = AUF1_OFFSET;
    _transactionTable[_transactionCount++][1] = pes._auf1Offset;
    _transactionTable[_transactionCount][0] = AUF2_OFFSET;
    _transactionTable[_transactionCount++][1] = pes._auf2Offset;

    kipdprintf("PES Template Length = %i, Data:\n", length);
    _transactionTable[_transactionCount][0] = PES_HDR_LEN;
    _transactionTable[_transactionCount++][1] = length;

    for (w1 = 0; w1 < 188; w1++)
    {
        _transactionTable[_transactionCount][0] = PES_HDR_LOOKUP + w1;
        _transactionTable[_transactionCount++][1] = pes._pkt8[w1];
    }             
    pes.dump8();

    ADPGen adp;
    adp._tsEncapType = kTsEncapTypeJ2k;
    adp._elemNumToPID[1] = streamData.videoPid;
    length = adp.makePacket();

    kipdprintf("Adaptation Template Length = %i, Data:\n", length);
    _transactionTable[_transactionCount][0] = ADAPTATION_HDR_LENGTH;
    _transactionTable[_transactionCount++][1] = length;
    for (w1 = 0; w1 < 188; w1++)
    {
        _transactionTable[_transactionCount][0] = ADAPTATION_LOOKUP + w1;
        _transactionTable[_transactionCount++][1] = adp._pkt32[w1];
    }
    adp.dump32();

    PATGen pat;
    pat._tsEncapType = kTsEncapTypeJ2k;
    pat._progNumToPID[1] = streamData.programPid;
    length = pat.makePacket();

    kipdprintf("PAT Template Length = %i, Data:\n", length);
    for (w1 = 0; w1 < 188; w1++)
    {
        _transactionTable[_transactionCount][0] = PAT_TABLE_LOOKUP + w1;
        _transactionTable[_transactionCount++][1] = pat._pkt8[w1];
    }
    pat.dump8();

    PMTGen pmt;
    pmt._tsEncapType = kTsEncapTypeJ2k;
    pmt._progNumToPID[1] = streamData.programPid;
    pmt._videoNumToPID[1] = streamData.videoPid;
    pmt._pcrNumToPID[1] = streamData.pcrPid;
    pmt._audioNumToPID[1] = streamData.audio1Pid;
    pmt._videoStreamData.j2kStreamType = streamData.j2kStreamType;
    pmt._videoStreamData.width = streamData.width;
    pmt._videoStreamData.height = streamData.height;
    pmt._videoStreamData.denFrameRate = streamData.denFrameRate;
    pmt._videoStreamData.numFrameRate = streamData.numFrameRate;
    pmt._videoStreamData.numAudioChannels = streamData.numAudioChannels;
    pmt._videoStreamData.interlaced = streamData.interlaced;
    length = pmt.makePacket();

    kipdprintf("PMT Template Length = %i, Data:\n", length);
    for (w1 = 0; w1 < 188; w1++)
    {
        _transactionTable[_transactionCount][0] = PMT_TABLE_LOOKUP + w1;
        _transactionTable[_transactionCount++][1] = pmt._pkt8[w1];
    }
    pmt.dump8();

    // On last entry enable is on
    _transactionTable[_transactionCount][0] = HOST_EN;
    _transactionTable[_transactionCount++][1] = 1;
}


void CNTV2ConfigTs2022::GenerateTableForMpegPcrEncap(const NTV2Channel channel)
{
    int32_t             w1;
    uint32_t            pcrPid;

    kipdprintf("CNTV2ConfigTs2022::GenerateTableForMpegPcrEncap\n");

    // Get the PCR pid
    ReadJ2KConfigReg(channel, kRegSarekEncodePcrPid1, &pcrPid);
    kipdprintf("PCR PID         = 0x%02x\n", pcrPid);

    _transactionCount = 0;

    // Start with enable off
    _transactionTable[_transactionCount][0] = HOST_EN;
    _transactionTable[_transactionCount++][1] = 0;

    _transactionTable[_transactionCount][0] = PAT_PMT_PERIOD;
    _transactionTable[_transactionCount++][1] = 0;
    kipdprintf("PAT/PMT Transmission Period = %i (0x%x)\n", _transactionTable[_transactionCount-1][1], _transactionTable[_transactionCount-1][1]);

    _transactionTable[_transactionCount][0] = PACKET_RATE;
    _transactionTable[_transactionCount++][1] = 0;
    kipdprintf("Packet Rate = %i (0x%x)\n", _transactionTable[_transactionCount-1][1], _transactionTable[_transactionCount-1][1]);

    ADPGen adp;
    adp._tsEncapType = kTsEncapTypePcr;
    adp._elemNumToPID[1] = pcrPid;
    uint32_t length = adp.makePacket();

    kipdprintf("Adaptation Template Length = %i, Data:\n", length);
    _transactionTable[_transactionCount][0] = ADAPTATION_HDR_LENGTH;
    _transactionTable[_transactionCount++][1] = length;
    for (w1 = 0; w1 < 188; w1++)
    {
        _transactionTable[_transactionCount][0] = ADAPTATION_LOOKUP + w1;
        _transactionTable[_transactionCount++][1] = adp._pkt32[w1];
    }
    adp.dump32();

    // On last entry enable is on
    _transactionTable[_transactionCount][0] = HOST_EN;
    _transactionTable[_transactionCount++][1] = 1;
}


void CNTV2ConfigTs2022::GenerateTableForMpegAesEncap(const NTV2Channel channel)
{
    int32_t             w1;
    uint32_t            audioPid;
    J2KStreamType       j2kStreamType;

    kipdprintf("CNTV2ConfigTs2022::GenerateTableForMpegAesEncap\n");

    // Get the Audio pid and stream type
    ReadJ2KConfigReg(channel, kRegSarekEncodeAudio1Pid1, &audioPid);
    ReadJ2KConfigReg(channel, kRegSarekEncodeStreamType1, (uint32_t *) &j2kStreamType);
    kipdprintf("Audio 1 PID     = 0x%02x\n\n", audioPid);

    _transactionCount = 0;

    // Start with enable off
    _transactionTable[_transactionCount][0] = HOST_EN;
    _transactionTable[_transactionCount++][1] = 0;

    // First we must set this to zero while disabled later we will set this same register to the proper value.
    _transactionTable[_transactionCount][0] = PACKET_RATE;
    _transactionTable[_transactionCount++][1] = 0;

    PESGen pes;
    pes._tsEncapType = kTsEncapTypeAes;
    pes._elemNumToPID[1] = audioPid;
    // For Aes PES generator the streamType is the only thing we need to pass in the videoStreamData struct
    pes._videoStreamData.j2kStreamType = j2kStreamType;

    kipdprintf("Host Register Settings:\n\n");

    _transactionTable[_transactionCount][0] = PAYLOAD_PARAMS;
    _transactionTable[_transactionCount++][1] = audioPid;
    kipdprintf("Payload Parameters = 0x%x\n", _transactionTable[_transactionCount-1][1]);

    _transactionTable[_transactionCount][0] = PACKET_RATE;
    _transactionTable[_transactionCount++][1] = 0x10000;
    kipdprintf("Packet Rate = %i (0x%x)\n", _transactionTable[_transactionCount-1][1], _transactionTable[_transactionCount-1][1]);

    int length = pes.makePacket();

    kipdprintf("PTS Offset = 0x%02x auf1 offset = 0x%02x auf2 offset = 0x%02x\n\n",
               pes._ptsOffset, pes._auf1Offset, pes._auf2Offset);
    _transactionTable[_transactionCount][0] = PTS_OFFSET;
    _transactionTable[_transactionCount++][1] = pes._ptsOffset;
    _transactionTable[_transactionCount][0] = AUF1_OFFSET;
    _transactionTable[_transactionCount++][1] = pes._auf1Offset;
    _transactionTable[_transactionCount][0] = AUF2_OFFSET;
    _transactionTable[_transactionCount++][1] = pes._auf2Offset;

    kipdprintf("PES Template Length = %i, Data:\n", length);
    _transactionTable[_transactionCount][0] = PES_HDR_LEN;
    _transactionTable[_transactionCount++][1] = length;

    for (w1 = 0; w1 < 188; w1++)
    {
        _transactionTable[_transactionCount][0] = PES_HDR_LOOKUP + w1;
        _transactionTable[_transactionCount++][1] = pes._pkt8[w1];
    }
    pes.dump8();

    ADPGen adp;
    adp._tsEncapType = kTsEncapTypeAes;
    adp._elemNumToPID[1] = audioPid;
    length = adp.makePacket();

    kipdprintf("Adaptation Template Length = %i, Data:\n", length);
    _transactionTable[_transactionCount][0] = ADAPTATION_HDR_LENGTH;
    _transactionTable[_transactionCount++][1] = length;
    for (w1 = 0; w1 < 188; w1++)
    {
        _transactionTable[_transactionCount][0] = ADAPTATION_LOOKUP + w1;
        _transactionTable[_transactionCount++][1] = adp._pkt32[w1];
    }
    adp.dump32();

    // On last entry enable is on
    _transactionTable[_transactionCount][0] = HOST_EN;
    _transactionTable[_transactionCount++][1] = 1;
}


uint32_t CNTV2ConfigTs2022::GetIpxJ2KAddr(const NTV2Channel channel)
{
    // Only NTV2_CHANNEL1 and NTV2_CHANNEL2 are valid.
    uint32_t addr = SAREK_J2K_ENCODER_1;
    
    if (channel == NTV2_CHANNEL2)
    {
        addr = SAREK_J2K_ENCODER_2;
    }
    return addr;
}


uint32_t CNTV2ConfigTs2022::GetIpxTsAddr(const NTV2Channel channel)
{
    // Only NTV2_CHANNEL1 and NTV2_CHANNEL2 are valid.
    uint32_t addr = SAREK_TS_ENCODER_1;
    
    if (channel == NTV2_CHANNEL2)
    {
        addr = SAREK_TS_ENCODER_2;
    }
    return addr;
}

bool CNTV2ConfigTs2022::WriteJ2KConfigReg(const NTV2Channel channel, const uint32_t reg, const uint32_t value)
{
    bool rv = false;

    // Only NTV2_CHANNEL1 and NTV2_CHANNEL2 are valid.
    if ((channel == NTV2_CHANNEL1) || (channel == NTV2_CHANNEL2))
    {
        rv = mDevice.WriteRegister(SAREK_REGS2 + reg + ((kRegSarekEncodeAudio1Pid1-kRegSarekEncodeVideoFormat1+1) * channel), value);
    }
    else
        mIpErrorCode = NTV2IpErrInvalidChannel;

    //printf("CNTV2ConfigTs2022::WriteJ2KConfigReg reg = %08x %d\n", SAREK_REGS2 + reg + ((kRegSarekEncodeAudio1Pid1-kRegSarekEncodeVideoFormat1+1) * channel), value);
    return rv;
}

bool CNTV2ConfigTs2022::ReadJ2KConfigReg(const NTV2Channel channel, const uint32_t reg,  uint32_t * value)
{
    bool rv = false;

    // Only NTV2_CHANNEL1 and NTV2_CHANNEL2 are valid.
    if ((channel == NTV2_CHANNEL1) || (channel == NTV2_CHANNEL2))
    {
        rv = mDevice.ReadRegister(SAREK_REGS2 + reg + ((kRegSarekEncodeAudio1Pid1-kRegSarekEncodeVideoFormat1+1) * channel), *value);
    }
    else
        mIpErrorCode = NTV2IpErrInvalidChannel;

    //printf("CNTV2ConfigTs2022::ReadJ2KConfigVReg reg = %08x %d\n", SAREK_REGS2 + reg + ((kRegSarekEncodeAudio1Pid1-kRegSarekEncodeVideoFormat1+1) * channel), *value);
    return rv;
}

void CNTV2ConfigTs2022::SetEncoderInputEnable(const NTV2Channel channel, bool bEnable, bool bMDEnable )
{
#ifdef COCHRANE
    mDevice.WriteRegister( 0x20000, (bEnable?BIT(16):0)|(bMDEnable?BIT(17):0)|(bMDEnable?BIT(18):0), BIT(16)|BIT(17)|BIT(18));
#else
	uint32_t encoderBit(0), mdBit(0);
    if (channel == NTV2_CHANNEL2)
    {
		encoderBit = ENCODER_2_ENABLE;
		mdBit = ENCODER_2_MD_ENABLE;    // Bits 24 and 25
    } else if (channel == NTV2_CHANNEL1)
    {
		encoderBit = ENCODER_1_ENABLE;
		mdBit = ENCODER_1_MD_ENABLE;    // Bits 24 and 25
    }

	uint32_t val;
	uint32_t tmp = (bEnable?encoderBit:0) | (bMDEnable?mdBit:0);
    mDevice.ReadRegister(SAREK_REGS + kRegSarekControl, val);
    val &= ~(encoderBit|mdBit);
	val |= tmp;
	mDevice.WriteRegister(SAREK_REGS + kRegSarekControl, val);
#endif
}


void CNTV2ConfigTs2022::SetEncoderReset(const NTV2Channel channel, bool bReset )
{
#ifdef COCHRANE
	mDevice.WriteRegister( 0x20000, (bReset?BIT(12):0), BIT(12));
#else
	uint32_t resetBit = (channel == NTV2_CHANNEL2)?ENCODER_2_RESET:ENCODER_1_RESET;
	uint32_t val;
	mDevice.ReadRegister( SAREK_REGS + kRegSarekControl, val);
	if (bReset)
		val |= (resetBit);
	else
		val &= (~resetBit);
	mDevice.WriteRegister( SAREK_REGS + kRegSarekControl, val);
#endif
}


int32_t CNTV2ConfigTs2022::CalculateTsGen(const NTV2Channel channel)
{
    uint32_t    mbps;
    uint32_t    audioChannels1;

    ReadJ2KConfigReg(channel, kRegSarekEncodeMbps1, &mbps);
    ReadJ2KConfigReg(channel, kRegSarekEncodeAudioChannels1, &audioChannels1);

    // Calculate bitrate, allow 1.2mbps per audio channel, then add an additional 20%
    double ts_bitrate = (((double) mbps + ((double) audioChannels1 * 1.6)) *  1.2) * 1000000;
    double sys_clk = 125e6;
    double d1, d2;

    // First packet rate
    d1 = ts_bitrate / 8.0 / 188.0;		// Packet Rate
    d1 = 1.0 / d1;						// Packet Period
    d2 = 1.0 / sys_clk;					// Clock Period
    d1 = d1 / d2 - 1.0;					// One less as it counts from 0

    return (int32_t) d1;
}

