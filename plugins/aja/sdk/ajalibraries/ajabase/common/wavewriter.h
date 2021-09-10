/* SPDX-License-Identifier: MIT */
/**
	@file		wavewriter.h
	@brief		Declares the AJAWavWriter class.
	@copyright	(C) 2010-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJAWAVEWRITER_H
#define AJAWAVEWRITER_H

#include "public.h"
#include "ajabase/system/file_io.h"


class AJA_EXPORT AJAWavWriterAudioFormat
{
public:
    AJAWavWriterAudioFormat(const int inChannelCount = 2, const int inSampleRate = 48000, const int inSampleSizeBits = 16)
		:	channelCount (inChannelCount),
			sampleRate   (inSampleRate),
			sampleSize   (inSampleSizeBits)
		{
		}
    int channelCount;
    int sampleRate;
    int sampleSize;
};


class AJA_EXPORT AJAWavWriterVideoFormat
{
public:
    AJAWavWriterVideoFormat() { rateDuration = 30000; rateScale = 1001; }
    uint32_t rateDuration;
    uint32_t rateScale;
};


enum AJAWavWriterChunkFlag
{
    AJAWavWriterChunkFlagStandard = 1 << 0,
    AJAWavWriterChunkFlagBextV1   = 1 << 1
};


class AJA_EXPORT AJAWavWriter : public AJAFileIO
{
    
public:
	AJAWavWriter(const std::string & name,
                 const AJAWavWriterAudioFormat & audioFormat = AJAWavWriterAudioFormat(),
                 const AJAWavWriterVideoFormat & videoFormat = AJAWavWriterVideoFormat(),
				 const std::string & startTimecode = "00:00:00;00", AJAWavWriterChunkFlag flags = AJAWavWriterChunkFlagStandard,
				 bool useFloatNotPCM = false);
	bool open();
	void close();
    
    uint32_t write(const char* data, uint32_t len);
    
protected:
	AJAStatus Open(const std::string& fileName, int flags, AJAFileProperties properties);    
	AJAStatus Close(void);
    
private:
    uint32_t writeRawData(const char* data,uint32_t len);
    uint32_t writeRawData(char* data,uint32_t len);
    uint32_t writeRaw_uint8_t(uint8_t value,   uint32_t count=1);
    uint32_t writeRaw_uint16_t(uint16_t value, uint32_t count=1);
    uint32_t writeRaw_uint32_t(uint32_t value, uint32_t count=1);
    
	void writeHeader();
    
    std::string                 mFileName;
	AJAWavWriterAudioFormat     mAudioFormat;
    AJAWavWriterVideoFormat     mVideoFormat;
    std::string                 mStartTimecode;
    AJAWavWriterChunkFlag       mFlags;
    bool                        mLittleEndian;
    int32_t                     mSizeOfHeader;
	bool						mUseFloatData;
};

#endif	//	AJAWAVEWRITER_H
