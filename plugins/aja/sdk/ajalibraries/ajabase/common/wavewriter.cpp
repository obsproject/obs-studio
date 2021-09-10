/* SPDX-License-Identifier: MIT */
/**
	@file		wavewriter.cpp
	@brief		Implements the AJAWavWriter class.
	@copyright	(C) 2015-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#include "wavewriter.h"

#include "ajabase/common/timebase.h"
#include "ajabase/common/timecode.h"
#include <time.h>
#include <assert.h>

#ifdef AJA_LINUX
#include <string.h>
#include <stdint.h>
#ifndef UINT32_C
#define UINT32_C(x) x##U
#endif
#endif

const int sizeOf_riff   = 12;
const int sizeOf_bext_v1= 610;
const int sizeOf_fmt    = 24;
const int sizeOf_data   = 8;

#if defined(AJA_LITTLE_ENDIAN)
#define AjaWavLittleEndianHw
#else
#define AjaWavBigEndianHw
#endif

#define AjaWavSwap16(x) (	((uint16_t(x) & 0xFF00) >> 8)	|	\
							((uint16_t(x) & 0x00FF) << 8)	)

#define AjaWavSwap32(x) (	((uint32_t(x) & 0xFF000000) >> 24)	|	\
							((uint32_t(x) & 0x00FF0000) >>  8)	|	\
							((uint32_t(x) & 0x0000FF00) <<  8)	|	\
							((uint32_t(x) & 0x000000FF) << 24)	)

#define AjaWavSwap64(x) (	((uint64_t(x) & 0xFF00000000000000ULL) >> 56)	|	\
							((uint64_t(x) & 0x00FF000000000000ULL) >> 40)	|	\
							((uint64_t(x) & 0x0000FF0000000000ULL) >> 24)	|	\
							((uint64_t(x) & 0x000000FF00000000ULL) >>  8)	|	\
							((uint64_t(x) & 0x00000000FF000000ULL) <<  8)	|	\
							((uint64_t(x) & 0x0000000000FF0000ULL) << 24)	|	\
							((uint64_t(x) & 0x000000000000FF00ULL) << 40)	|	\
							((uint64_t(x) & 0x00000000000000FFULL) << 56)	)

#ifdef  AjaWavBigEndianHw
	#define AjaWavBigEndian16(x)      (x)
	#define AjaWavLittleEndian16(x)   AjaWavSwap16(x)
	#define AjaWavBigEndian32(x)      (x)
	#define AjaWavLittleEndian32(x)   AjaWavSwap32(x)
	#define AjaWavBigEndian64(x)      (x)
	#define AjaWavLittleEndian64(x)   AjaWavSwap64(x)
#else
	#define AjaWavLittleEndian16(x)   (x)
	#define AjaWavBigEndian16(x)      AjaWavSwap16(x)
	#define AjaWavLittleEndian32(x)   (x)
	#define AjaWavBigEndian32(x)      AjaWavSwap32(x)
	#define AjaWavLittleEndian64(x)   (x)
	#define AjaWavBigEndian64(x)      AjaWavSwap64(x)
#endif

static void getDataAndTimeInBextFormat(std::string& formattedDate, std::string& formattedTime)
{
    char tmp[16];
    time_t now = time(NULL);
	struct tm lcl = *localtime(&now);
    ::strftime(tmp,16,"%Y:%m:%d",&lcl); //"yyyy:mm:dd"
    formattedDate.assign(tmp);
    
    ::strftime(tmp,16,"%H:%M:%S",&lcl); //"hh:mm:ss"
    formattedTime.assign(tmp);
}

AJAWavWriter::AJAWavWriter(const std::string & name, const AJAWavWriterAudioFormat & audioFormat, const AJAWavWriterVideoFormat & videoFormat,
						   const std::string & startTimecode, AJAWavWriterChunkFlag flags,
						   bool useFloatNotPCM)
: AJAFileIO(), mFileName(name), mAudioFormat(audioFormat), mVideoFormat(videoFormat), mStartTimecode(startTimecode), mFlags(flags), mLittleEndian(true),
  mUseFloatData(useFloatNotPCM)
{
    mSizeOfHeader = sizeOf_riff + sizeOf_fmt + sizeOf_data;
    
    if (mFlags & AJAWavWriterChunkFlagBextV1)
    {
        mSizeOfHeader += sizeOf_bext_v1;
    }
}

// making this call not public
AJAStatus AJAWavWriter::Open(const std::string& fileName, int flags, AJAFileProperties properties)
{
    return AJAFileIO::Open(fileName,flags,properties);
}

// making this call not public
AJAStatus AJAWavWriter::Close(void)
{
    return AJAFileIO::Close();
}

bool AJAWavWriter::open()
{
    bool retVal = false;
    AJAStatus result = Open(mFileName,eAJACreateAlways | eAJAWriteOnly,eAJABuffered);
    if(result == AJA_STATUS_SUCCESS)
    {
        writeHeader();
        retVal = true;
    }
    
    return retVal;
}

uint32_t AJAWavWriter::write(const char* data, uint32_t len)
{
    return writeRawData(data,len);
}

uint32_t AJAWavWriter::writeRawData(const char* data,uint32_t len)
{
    return writeRawData((char*)data,len);
}

uint32_t AJAWavWriter::writeRawData(char* data,uint32_t len)
{
    return Write((uint8_t*)data,len);
}

uint32_t AJAWavWriter::writeRaw_uint8_t(uint8_t value, uint32_t count)
{
    uint32_t bytesWritten = 0;
    
    for(uint32_t i=0;i<count;i++)
    {
        bytesWritten += Write(&value,1);
    }
    
    return bytesWritten;
}

uint32_t AJAWavWriter::writeRaw_uint16_t(uint16_t value, uint32_t count)
{
    uint32_t bytesWritten = 0;
    
    if(mLittleEndian)
        value = AjaWavLittleEndian16(value);
    else
        value = AjaWavBigEndian16(value);
    
    for(uint32_t i=0;i<count;i++)
    {
        bytesWritten += Write((uint8_t*)&value, sizeof(uint16_t));
    }
    
    return bytesWritten;
}

uint32_t AJAWavWriter::writeRaw_uint32_t(uint32_t value, uint32_t count)
{
    uint32_t bytesWritten = 0;
    
    if(mLittleEndian)
        value = AjaWavLittleEndian32(value);
    else
        value = AjaWavBigEndian32(value);
    
    for(uint32_t i=0;i<count;i++)
    {
        bytesWritten += Write((uint8_t*)&value, sizeof(uint32_t));
    }
    
    return bytesWritten;
}

void AJAWavWriter::writeHeader()
{
    mLittleEndian = true;
        
    uint32_t wtn = 0;
    
	// RIFF chunk
    wtn += writeRawData("RIFF", 4);
	wtn += writeRaw_uint32_t(0);                           // Placeholder for the RIFF chunk size (filled by close())
	wtn += writeRawData("WAVE", 4);
    
    if (mFlags & AJAWavWriterChunkFlagBextV1)
    {
        // bext chunk (version 1)
        // Calculate the sequence timecode start values
        // These are calculated as:
        // Start timecode converted to seconds * audio sample rate
        // then split into a High and Low
        
        uint32_t            scale    = mVideoFormat.rateScale;
        uint32_t            duration = mVideoFormat.rateDuration;
        //according to Canon they always use 1000 for 23.98, just go with it
        if(scale == 24000 && duration == 1001)
            duration = 1000;
        
        AJATimeBase         tb(scale,duration);
        tb.SetAudioRate(mAudioFormat.sampleRate);
        
        AJATimeCode         tc(mStartTimecode.c_str(),    tb);
        
        uint64_t frames                   = tc.QueryFrame();
        uint64_t numAudioSamplesPerSecond = tb.FramesToSamples(frames);
        
        uint32_t startSequenceTcLow  = numAudioSamplesPerSecond & 0x00000000FFFFFFFF;
        uint32_t startSequenceTcHigh = numAudioSamplesPerSecond >> 32;
        
        std::string formattedDate;
        std::string formattedTime;
        getDataAndTimeInBextFormat(formattedDate, formattedTime);
        
        char emptyBuf[256];
        memset(emptyBuf,0,256);
        
        wtn += writeRawData("bext",4);
        wtn += writeRaw_uint32_t(602);                         //size of extension chunk
        wtn += writeRawData(emptyBuf,256);                     //description of sound sequence
        wtn += writeRawData(emptyBuf,32);                      //name of originator
        wtn += writeRawData(emptyBuf,32);                      //reference of originator
        wtn += writeRawData(formattedDate.c_str(),10);         //origination date <<yyyy:mm:dd>>
        wtn += writeRawData(formattedTime.c_str(),8);          //origination time <<hh:mm:ss>>
        wtn += writeRaw_uint32_t(startSequenceTcLow);          //timecode of sequence (low)
        wtn += writeRaw_uint32_t(startSequenceTcHigh);         //timecode of sequence (high)
        wtn += writeRaw_uint16_t(1);                           //BWF version
        wtn += writeRaw_uint8_t(0,64);                         //U8 * 64, From UMID_0 to UMID_63
        wtn += writeRaw_uint8_t(0,190);                        //U8 * 190 reserved
    }
    
	// Format description chunk
	wtn += writeRawData("fmt ", 4);
	wtn += writeRaw_uint32_t(16);                              // "fmt " chunk size (always 16 for PCM)
	int audioFormat = 1; //PCM
	if (mUseFloatData)
	{
		audioFormat = 3; //IEEE Float
	}
	wtn += writeRaw_uint16_t(audioFormat);                     // data format (1 => PCM), (3 => IEEE Float)
	wtn += writeRaw_uint16_t(mAudioFormat.channelCount);
	wtn += writeRaw_uint32_t(mAudioFormat.sampleRate);
	wtn += writeRaw_uint32_t(mAudioFormat.sampleRate * mAudioFormat.channelCount * mAudioFormat.sampleSize / 8 ); // bytes per second
	wtn += writeRaw_uint16_t(mAudioFormat.channelCount * mAudioFormat.sampleSize / 8); // Block align
	wtn += writeRaw_uint16_t(mAudioFormat.sampleSize);         // Significant Bits Per Sample
    
	// data chunk
	wtn += writeRawData("data", 4);
	wtn += writeRaw_uint32_t(0);                               // Placeholder for the data chunk size (filled by close())
    
    assert(Tell() == mSizeOfHeader);
}

void AJAWavWriter::close()
{
	// Fill the header size placeholders
	uint32_t fileSize = (uint32_t)Tell();
    
    mLittleEndian = true;
    
	// RIFF chunk size
	Seek(4,eAJASeekSet);
	writeRaw_uint32_t(fileSize - sizeOf_data);
    
	// data chunk size
	Seek(mSizeOfHeader-4,eAJASeekSet);
	writeRaw_uint32_t(fileSize - mSizeOfHeader);
    
    Close();
}
